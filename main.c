/*

 * Author: Sam
*******************************************************************************/
/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/timing.h>
#include <drv/esp8266.h>
#include <station.h>

#define READ_LINE_BUFSIZE (CHAR_MAX + 1)
#define READ_LINE_LAST_POSITION CHAR_MAX

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
            sscanf(date, "%d-%d-%d", &day, &month, &year);
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

int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    init_clocks();
    uart_use_stdio_support();
    init_station_module();
    rtc_init();

    Interrupt_disableSleepOnIsrExit();
    Interrupt_enableMaster();

    uart_replace_standard_streams(
        (struct uart_config) {.id = 0, .baud_rate = 115200, .flags = 0},
        (struct uart_input_config) {.complete_lines = true, .echo = true},
        (struct uart_output_config) {});

    wifi_uart = uart_open(
            (struct uart_config) {.id = 2, .baud_rate = 115200, .flags = 0},
            (struct uart_input_config) {.complete_lines = true, .echo = false},
            (struct uart_output_config) {});

    puts("What up!\r\n");
    puts("Test\r\n");

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    delay_ms(1000);

    while(1)
    {
        if(!time_set)
        {
            set_time_nist();
        }
        run_station_module();
        //PCM_gotoLPM0();
        while(!rtc_second_passed)
        {
            MAP_PCM_gotoLPM0();
        }
        rtc_second_passed = false;
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }
}
