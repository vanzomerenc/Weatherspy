/*
 * station.h
 *
 *  Created on: Apr 14, 2018
 *      Author: Sam
 */

#ifndef STATION_H_
#define STATION_H_

/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/timing.h>
#include <drv/ST7735/ST7735.h>
#include <drv/rtc.h>
#include <drv/adc/adc.h>
#include <drv/BME280/bme280.h>
#include <drv/i2c/i2c.h>
#include <sensors/atmospheric.h>
#include "weather_station_status.h"
#include "gui/weather_station_ui.h"
#include "gui/gui_layout.h"
#include "gui/embedded_gui.h"

#define READ_LINE_BUFSIZE (CHAR_MAX + 1)
#define READ_LINE_LAST_POSITION CHAR_MAX

struct weather_station_status status;
struct bme280_dev sensor_atmospheric;

// This will be used to store time data received from the web
struct rtc_time received_time;
bool received_time_valid = false;
struct uart_channel wifi_uart;
int time_set = 0;
char receive_buff[READ_LINE_BUFSIZE] = {'\0'};

// Returns 1 if expected found
int receive(char* expected)
{
    delay_ms(50);
    char* ptr = NULL;
    do
    {
        ptr = fgets(receive_buff, READ_LINE_BUFSIZE, wifi_uart.rx);
        if(NULL != ptr)
        {
            puts(receive_buff);
        }
        if(strstr(receive_buff, expected))
        {
            return 1;
        }
    } while(NULL != ptr);

    return 0;
}

void send(char* command)
{
    fseek(wifi_uart.rx, 0, SEEK_END);
    fputs(command, wifi_uart.tx);
}

// Returns 1 if expected found
int send_receive(char* command, char* expected)
{
    send(command);
    return receive(expected);
}

void send_sensor_data()
{
    char post_sensor_data[500] = {'\0'};
    char request_post[50] = {'\0'};
    send("AT+CIPSTART=\"TCP\",\"api.pushingbox.com\",80\r\n");
    delay_ms(3000);
    sprintf(post_sensor_data,"GET /pushingbox?devid=v24CBC064ECA935A&humidityData=%.1f&tempData=%.1f&"
     "pressData=%.2f&lightData=77 HTTP/1.1\r\nHost: api.pushingbox.com\r\n"
     "User-Agent: ESP8266/1.0\r\nConnection: close\r\n\r\n"
     ,status.indoor_humidity,status.indoor_temperature,status.pressure);
    sprintf(request_post, "AT+CIPSEND=%d\r\n", strlen(post_sensor_data));
    send(request_post);
    delay_ms(500);
    send(post_sensor_data);
    delay_ms(2000);
}

void set_time_nist()
{
    char date[10] = {'\0'};
    char time[10] = {'\0'};
    int month = 0;
    int day = 0;
    int year = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;

    send("AT\r\n");
    while(!receive("OK"));
    receive_buff[0] = '\0';

    send("AT+CWMODE=3\r\n");
    while(!receive("OK"));
    receive_buff[0] = '\0';

    send("AT+CWJAP=\"Samdroid\",\"859e21178c35\"\r\n");
    delay_ms(5000);
    while(!receive("OK"));
    receive_buff[0] = '\0';

    send("AT+CIPSTART=\"TCP\",\"time.nist.gov\",13\r\n");
    delay_ms(5000);
    while(!receive("UTC(NIST)"));
    char* token = strtok(receive_buff, " ");
    while(token != NULL)
    {
        if(strstr(token, "-"))
        {
            strcpy(date, token);
            sscanf(date, "%d-%d-%d", &year, &month, &day);
            year += 2000;
        }
        if(strstr(token, ":"))
        {
            strcpy(time, token);
            sscanf(time, "%d:%d:%d", &hour, &min, &sec);
        }
        token = strtok(NULL, " ");
    }
    receive_buff[0] = '\0';

    while(!receive("CLOSED"));
    receive_buff[0] = '\0';

    received_time.date = day;
    received_time.hour = hour;
    received_time.min = min;
    received_time.sec = sec;
    received_time.month = month;
    received_time.year = year;
    received_time_valid = true;

    time_set = 1;
}

void init_station_module()
{
    //struct adc_channel_config adc_channels[1];
    //adc_channels[0] = (struct adc_channel_config){.input_id = 0, .is_high_range = true};
    //adc_init(1, adc_channels);

    ST7735_InitR(INITR_REDTAB); // initialize LCD controller IC

    sensor_atmospheric_init(&sensor_atmospheric);

    status = (struct weather_station_status) {
         .lighting = lighting_dark,
         .outdoor_temperature = 0.0f,
         .indoor_temperature = 0.0f,
         .outdoor_humidity = 0.0f,
         .indoor_humidity = 0.0f,
         .pressure = 0.0f,
         .time = (struct rtc_time) {
                 .sec = 0,
                 .min = 0,
                 .hour = 12,
                 .date = 15,
                 .month = 1,
                 .year = 2018
             }
    };

    show_colon = 0;

    ST7735_FillScreen(0);
}

void run_station_module()
{
    struct sensor_atmospheric_result sensor_atmospheric_result = {0};
    sensor_atmospheric_read(&sensor_atmospheric, &sensor_atmospheric_result);
    status.indoor_humidity = sensor_atmospheric_result.humidity;
    status.indoor_temperature = sensor_atmospheric_result.temperature;
    status.pressure = sensor_atmospheric_result.pressure;

    rtc_gettime(&status.time);

    draw_weather_station_ui(status);

    if(received_time_valid)
    {
        if(received_time.min != status.time.min
            || received_time.hour != status.time.hour
            || received_time.date != status.time.date
            || received_time.month != status.time.month
            || received_time.year != status.time.year)
        {
            rtc_settime(&received_time);
        }
        received_time_valid = false;
    }
    if(rtc_minute_passed)
    {
        rtc_minute_passed = false;
        // only post on even minutes (every 2 min)
        if(status.time.min % 2 == 0)
        {
            send_sensor_data();
        }
    }
}
#endif /* STATION_H_ */
