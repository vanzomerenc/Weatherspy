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
#include <string.h>
#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/timing.h>
#include "htmlbody.h"
int clear_if_found(char* buffer, char* term);
int lookforchannel(char* buffer, char* term);

int main(void)
{
    enum state {SET_MULTIPLE, SET_SERVER, WAIT_CONNECT, INIT_HEADER, SEND_HEADER, INIT_BODY, SEND_BODY, CLOSE_CONNECT, WAIT_RECEIVE, RECEIVE, CONNECT};
    enum state currState = SET_MULTIPLE;
    int red = 0;
    int blue = 0;
    int green = 0;
    int temp = 75;
    int humidity = 30;
    float pressure = 29.145;

    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    init_clocks();

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
    int err = uart_use_stdio_support();

    Interrupt_enableMaster();

    struct uart_config config = {.id = 0, .baud_rate = 115200, .flags = 0};
    struct uart_config config2 = {.id = 2, .baud_rate = 115200, .flags = 0};
    struct uart_channel channel = uart_open(config);
    struct uart_channel channel2 = uart_open(config2);

    err = fputs("AT+CIPMUX=1\r\n", channel2.tx);

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    int c1 = EOF;
    int c2 = EOF;
    char receiveBuffer[1200] = {'\0'};
    char charstr[2] = {'\0'};
    char sendLine[1200] = {'\0'};
    char sendBody[1200] = {'\0'};
    char receiveExpect[100] = {'\0'};
    char htmlHeader[] = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
    int ipdchannel = 0;
    while(1)
    {
        c2 = fgetc(channel2.rx);
        if (c2 != EOF) {
            fputc(c2, channel.tx);
            sprintf(charstr, "%c", c2);
            strcat(receiveBuffer, charstr);
        }
        else {
            switch(currState)
            {
                case SET_MULTIPLE:
                    if(clear_if_found(receiveBuffer, "OK"))
                    {
                        currState = SET_SERVER;
                        fputs("AT+CIPSERVER=1,80\r\n", channel2.tx);
                        //delay_ms(50);
                    }
                    break;
                case SET_SERVER:
                    if(strstr(receiveBuffer, ",CONNECT"))
                    {
                        currState = WAIT_CONNECT;
                        //delay_ms(50);
                    }
                    break;
                case WAIT_CONNECT:
                    ipdchannel = lookforchannel(receiveBuffer, "+IPD,");
                    if(ipdchannel != -1)
                    {
                        //delay_ms(50);
                        currState = CONNECT;
                    }
                    break;
                case CONNECT:
                    if(strstr(receiveBuffer, "Host"))
                    {
                        if(strstr(receiveBuffer, "/?"))
                        {
                            red = strstr(receiveBuffer, "red=on") ? 1 : 0;
                            green = strstr(receiveBuffer, "green=on") ? 1 : 0;
                            blue = strstr(receiveBuffer, "blue=on") ? 1 : 0;

                            if(strstr(receiveBuffer, "OnOff=On"))
                            {
                                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
                                if(red) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
                                if(green) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
                                if(blue) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
                            }
                            if(strstr(receiveBuffer, "OnOff=Off"))
                            {
                                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
                            }
                            //delay_ms(50);
                            receiveBuffer[0] = '\0';
                            currState = WAIT_CONNECT;
                        }
                        else
                        {
                            currState = INIT_HEADER;
                            sprintf(sendLine, "AT+CIPSEND=%d,%d\r\n", ipdchannel, strlen(htmlHeader));
                            receiveBuffer[0] = '\0';
                            //delay_ms(50);
                            fputs(sendLine, channel2.tx);
                            //delay_ms(50);
                        }
                    }
                    break;
                case INIT_HEADER:
                    if(clear_if_found(receiveBuffer, ">"))
                    {
                        currState = SEND_HEADER;
                        fputs(htmlHeader, channel2.tx);
                        //delay_ms(50);
                    }
                    break;
                case SEND_HEADER:
                    if(clear_if_found(receiveBuffer, "SEND OK"))
                    {
                        currState = INIT_BODY;
                        sprintf(sendBody, htmlBody, temp, humidity, pressure);
                        sprintf(sendLine, "AT+CIPSEND=%d,%d\r\n", ipdchannel, strlen(sendBody));
                        fputs(sendLine, channel2.tx);
                        //delay_ms(50);
                    }
                    break;
                case INIT_BODY:
                    if(clear_if_found(receiveBuffer, ">"))
                    {
                        currState = SEND_BODY;
                        fputs(sendBody, channel2.tx);
                        //delay_ms(50);
                    }
                    break;
                case SEND_BODY:
                    if(clear_if_found(receiveBuffer, "SEND OK"))
                    {
                        currState = CLOSE_CONNECT;
                        sprintf(sendLine, "AT+CIPCLOSE=%d\r\n", ipdchannel);
                        fputs(sendLine, channel2.tx);
                        //delay_ms(50);
                    }
                    break;
                case CLOSE_CONNECT:
                    sprintf(receiveExpect, "%d,CLOSED", ipdchannel);
                    if(clear_if_found(receiveBuffer, receiveExpect))
                    {
                        currState = WAIT_CONNECT;
                    }
                    break;
                case WAIT_RECEIVE:
                    if(strstr(receiveBuffer, "/?"))
                    {
                        red = strstr(receiveBuffer, "red=on") ? 1 : 0;
                        green = strstr(receiveBuffer, "green=on") ? 1 : 0;
                        blue = strstr(receiveBuffer, "blue=on") ? 1 : 0;

                        if(strstr(receiveBuffer, "OnOff=On"))
                        {
                            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
                            if(red) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
                            if(green) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
                            if(blue) MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
                        }
                        if(strstr(receiveBuffer, "OnOff=Off"))
                        {
                            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
                        }
                        delay_ms(50);
                        currState = WAIT_CONNECT;
                    }
                    break;
                default:
                    break;
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

int clear_if_found(char* buffer, char* term)
{
    if(strstr(buffer, term))
    {
        buffer[0] = '\0';
        return 1;
    }
    return 0;
}

int lookforchannel(char* buffer, char* term)
{
    char* sptr = strstr(buffer, term);
    if(sptr)
    {
        sptr = sptr + strlen(term);
        if(sptr != NULL && *sptr != '\0')
        {
            return ((*sptr) - '0');
        }
    }
    return -1;
}
