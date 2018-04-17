/*
 * -------------------------------------------
 *    MSP432 DriverLib - v3_21_00_05 
 * -------------------------------------------
 *
 * --COPYRIGHT--,BSD,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/******************************************************************************
 * MSP432 Empty Project
 *
 * Description: An empty project that uses DriverLib
 *
 *                MSP432P401
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST               |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 * Author: 
*******************************************************************************/
/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/timing.h>
#include <drv/esp8266.h>
#include <drv/rtc.h>
#include <sensors/atmospheric.h>

#include <generated/status.h>
#include "station.h"



int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    init_clocks();
    rtc_init();

    struct bme280_dev atmospheric_sensor;
    sensor_atmospheric_init(&atmospheric_sensor);

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    uart_use_stdio_support();

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

    AtInterface wifi = at_init(wifi_uart.tx, wifi_uart.rx);

    int err = at_check_alive(wifi);
    err = esp8266_set_wifi_mode(wifi, WIFI_MODE_SOFT_AP_AND_STATION);
    enum esp8266_wifi_error wifi_err = 0;
    err = esp8266_connect_to_ap(
            wifi,
            (struct wifi_ap) {
                .ssid="G6_5137",
                .passphrase="TomatoSandwich"},
            &wifi_err,
            1000);
    do
    {
        err = esp8266_set_hosted_ap(
                wifi,
                (struct wifi_ap) {
                    .ssid="Your Advertisement Here",
                    .passphrase="sendmoneyplease",
                    .channel=6},
                1000);
    } while(err == AT_BUSY);

    err = esp8266_server_start(wifi, 1000);

    struct webpage_status status = {0};

    set_time_nist();
    rtc_settime(&received_time);

    int64_t last_iter_time;
    while(1)
    {
        last_iter_time = now();

        struct rtc_time ct;
        rtc_gettime(&ct);
        status.year = ct.year;
        status.month = ct.month;
        status.day = ct.date;
        status.hour = ct.hour;
        status.minute = ct.min;
        status.second = ct.sec;

        struct sensor_atmospheric_result current_reading;
        sensor_atmospheric_read(&atmospheric_sensor, &current_reading);
        status.temperature = current_reading.temperature;
        status.humidity = current_reading.humidity;
        status.pressure = current_reading.pressure;

        esp8266_server_periodic(wifi, &print_webpage_status, &status);
        clearerr(wifi_uart.rx);
        fseek(wifi_uart.rx, 0, SEEK_END);
        P1->OUT ^= 0x1;

        while(last_iter_time + 10 > now())
        {
            PCM_gotoLPM0();
        }
    }
}
