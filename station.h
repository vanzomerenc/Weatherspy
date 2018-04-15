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

struct weather_station_status status;
struct bme280_dev sensor_atmospheric;

// This will be used to store time data received from the web
struct rtc_time received_time;
bool received_time_valid = false;

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
}
#endif /* STATION_H_ */
