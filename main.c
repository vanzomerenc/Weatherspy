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

int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    init_clocks();

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    int err = uart_use_stdio_support();

    Interrupt_enableMaster();

    struct uart_config config = {.id = 0, .baud_rate = 115200, .flags = 0};
    struct uart_config config2 = {.id = 2, .baud_rate = 115200, .flags = 0};
    struct uart_channel channel = uart_open(config);
    struct uart_channel channel2 = uart_open(config2);

    err = fputs("AT\r\n", channel2.tx);

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    int c1 = EOF;
    int c2 = EOF;
    char receiveBuffer[1000] = {'\0'};
    char charstr[2] = {'\0'};
    while(1)
    {
        c2 = fgetc(channel2.rx);
        if (c2 != EOF) {
            fputc(c2, channel.tx);
            sprintf(charstr, "%c", c2);
            strcat(receiveBuffer, charstr);
        }
        else {
            if(strstr(receiveBuffer, "OK"))
            {
                receiveBuffer[0] = '\0';
                fputs("HI\r\n", channel2.tx);
            }
        }
        c1 = fgetc(channel.rx);
        if (c1 != EOF) {
            fputc(c1, channel2.tx);
        }
    }
    fclose(channel.rx);
    fclose(channel.tx);
    fclose(channel2.rx);
    fclose(channel2.tx);
}
