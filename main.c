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
#include <limits.h>

#include <drv/timing.h>
#include <drv/simpleuart.h>
#include <drv/simpleuart_stdio_support.h>

bool on;
bool r;
bool g;
bool b;

#define READ_LINE_BUFSIZE (CHAR_MAX + 1)
#define READ_LINE_LAST_POSITION CHAR_MAX
bool read_line(char *b, FILE *f)
{
    int progress = b[READ_LINE_LAST_POSITION];
    for(int c = fgetc(f); c != EOF; c = fgetc(f))
    {
        b[progress] = c;
        progress++;
        if(progress == READ_LINE_LAST_POSITION || c == '\n')
        {
            b[progress] = '\0';
            b[READ_LINE_LAST_POSITION] = '\0';
            return true;
        }
    }
    b[READ_LINE_LAST_POSITION] = progress;
    return false;
}


void log(FILE *dest, char *context, char *message)
{
    fprintf(dest, "[%s]:\t%s", context, message);
    if(message[strlen(message) - 1] != '\n') {fputs("\r\n", dest);}
}

void wait_for_response(FILE *source, char *line, char *response, char *log_message, FILE *log_f)
{
    bool got_line;
    do
    {
        got_line = false;
        if(read_line(line, source))
        {
            got_line = true;
            log(log_f, log_message, line);
        }
        if(strncmp(line, "+IPD", 4) == 0)
        {
            return;
        }
        if(strstr(line, "FAIL") != NULL)
        {
            return;
        }
    }
    while(!got_line || (response != NULL && strcmp(line, response) != 0) || (response == NULL && strcmp(line, "\r\n") == 0));
}

void handle_request(char *const line, struct uart_channel channel, FILE *log_f)
{
    int link;
    int message_len;
    char message_type[32];
    char message_first_arg[256];
    int c = sscanf(line, " +IPD,%d,%d:%s %s", &link, &message_len, &message_type[0], &message_first_arg[0]);
    log(log_f, "--==RECEIVED REQUEST==--", line);
    if(c != 4) {fprintf(log_f, "%d\n", c); log(log_f, "WTF", "WTF"); return;}
    log(log_f, "REQUEST TYPE", message_type);
    log(log_f, "REQUESTED", message_first_arg);
    int i = 0;
    while(strcmp(line, "\r\n") != 0)
    {
        read_line(line, channel.rx);
        log(log_f, "REMAINDER", line);
        i++;
        if(i > 20) break;
    }
    fseek(channel.rx, 0, SEEK_END);

    char *message;

    if(message_first_arg[1] == '\0' || message_first_arg[1] == '?')
    {
        message = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n"
                    "<!DOCTYPE html>"
                    "<html> <body> <center>"
                    "<h1>Set the color of LED light:<br></h1>"
                    "<form action=\"\" method=\"get\">"
                    "<p style=\"color:black;font-size:28px\"> LED color:<br> </p>"
                    "<p style=\"color:red;font-size:24px\">"
                    "<label for=\"red\">Red</label>"
                    "<input type=\"checkbox\" name=\"red\"> </p>"
                    "<p style=\"color:green;font-size:24px\">"
                    "<label for=\"green\">Green</label>"
                    "<input type=\"checkbox\" name=\"green\"> </p>"
                    "<p style=\"color:blue;font-size:24px\">"
                    "<label for=\"blue\">Blue</label>"
                    "<input type=\"checkbox\" name=\"blue\"> </p>"
                    "<p style=\"color:black;font-size:28px\">Turn Light:"
                    "<input type=\"radio\" name=\"OnOff\" value=\"On\"> On"
                    "<input type=\"radio\" name=\"OnOff\" value=\"Off\" checked> Off<br></p>"
                    "<fieldset>"
                    "<legend style=\"color:black;font-size:28px\">Environmental variables: </legend>"
                    "<p style=\"color:black;font-size:28px\">"
                    "Temperature: 72<br>"
                    "Humidity: 20%<br>"
                    "Pressure: 30.1 in Hg<br></p>"
                    "</fieldset> <br>"
                    "<input type=\"submit\" value=\"Submit\">"
                    "</form> </center> </body> </html>";
        if(strstr(message_first_arg, "red=on") != NULL) {r=true;}
        else {r=false;}
        if(strstr(message_first_arg, "green=on") != NULL) {g=true;}
        else {g=false;}
        if(strstr(message_first_arg, "blue=on") != NULL) {b=true;}
        else {b=false;}
        if(strstr(message_first_arg, "OnOff=On") != NULL) {on=true;}
        if(strstr(message_first_arg, "OnOff=Off") != NULL) {on=false;}
    }
    else
    {
        message = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
    }

    fprintf(channel.tx, "AT+CIPSEND=%d,%d\r\n", link, strlen(message));
    fputs(message, channel.tx);

    if(message_first_arg[1] == '\0' || message_first_arg[1] == '?')
    {
        wait_for_response(channel.rx, line, "SEND OK\r\n", "WAITING FOR ACKNOWLEDGEMENT...", log_f);
    }
    else
    {
        wait_for_response(channel.rx, line, "OK\r\n", "WAITING FOR ACKNOWLEDGEMENT...", log_f);
    }
    fprintf(channel.tx, "AT+CIPCLOSE=%d\r\n", link);
    wait_for_response(channel.rx, line, "OK\r\n", "WAITING FOR CLOSE...", log_f);
}



int main(void)
{
    /* Stop Watchdog  */
    MAP_WDT_A_holdTimer();

    init_clocks();

    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);

    MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    int err = uart_use_stdio_support();

    Interrupt_enableMaster();
    Interrupt_disableSleepOnIsrExit();

    struct uart_config config = {.id = 0, .baud_rate = 115200, .flags = 0};
    struct uart_channel pc = uart_open(config);
    config.id = 2;
    struct uart_channel wifi = uart_open(config);

    MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN1);

    log(pc.tx, "--==STARTED==--", "\r\n");

    while(fgetc(wifi.rx) < 0) {PCM_gotoLPM0();} // wait for the wi-fi module to wake up
    delay_ms(1000); // too lazy right now to work out what to look for
    fseek(wifi.rx, 0, SEEK_END);
    fseek(pc.rx, 0, SEEK_END);
    char line[READ_LINE_BUFSIZE] = {0};

    fputs("ATE0\r\n", wifi.tx);
    fseek(wifi.rx, 0, SEEK_END);
    wait_for_response(wifi.rx, line, "OK\r\n", "ATE0 success?", pc.tx);

    fputs("AT+CWSAP_CUR=\"Your Advertisement Here\",\"sendmoneyplease\",6,3\r\n", wifi.tx);
    fseek(wifi.rx, 0, SEEK_END);
    wait_for_response(wifi.rx, line, "OK\r\n", "AT+CWSAP_CUR success?", pc.tx);

    fputs("AT+CIPAP_CUR?\r\n", wifi.tx);
    fseek(wifi.rx, 0, SEEK_END);
    wait_for_response(wifi.rx, line, NULL, "", pc.tx);
    log(pc.tx, "AP address", line);
    wait_for_response(wifi.rx, line, NULL, "", pc.tx);
    log(pc.tx, "AP gateway", line);
    wait_for_response(wifi.rx, line, NULL, "", pc.tx);
    log(pc.tx, "AP netmask", line);

    fputs("AT+CIPMUX=1\r\n", wifi.tx);
    fseek(wifi.rx, 0, SEEK_END);
    wait_for_response(wifi.rx, line, NULL, "", pc.tx);
    log(pc.tx, "AT+CIPMUX success?", line);

    fputs("AT+CIPSERVER=1,80\r\n", wifi.tx);
    fseek(wifi.rx, 0, SEEK_END);
    wait_for_response(wifi.rx, line, NULL, "", pc.tx);
    log(pc.tx, "AT+CIPSERVER success?", line);

    while(1)
    {

        if(line[255] == '\0' && strncmp("+IPD", line, 4) == 0)
        {
            handle_request(line, wifi, pc.tx);
        }
        else
        {
            if(read_line(line, wifi.rx))
            {
                log(pc.tx, "WIFI", line);
            }
        }
        if(on)
        {
            GPIO_setOutputHighOnPin(GPIO_PORT_P2,
                                    (r? GPIO_PIN0 : 0)
                                    |(g? GPIO_PIN1 : 0)
                                    |(b? GPIO_PIN2 : 0));
            GPIO_setOutputLowOnPin(GPIO_PORT_P2,
                                    ((!r)? GPIO_PIN0 : 0)
                                    |((!g)? GPIO_PIN1 : 0)
                                    |((!b)? GPIO_PIN2 : 0));
        }
        else
        {
            GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2);
        }
        PCM_gotoLPM0();
    }
}
