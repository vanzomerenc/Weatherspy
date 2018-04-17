/*
 * esp8266.c
 *
 *  Created on: Mar 24, 2018
 *      Author: Chris van Zomeren
 */


#include "esp8266.h"

#include "driverlib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drv/timing.h>

#include <generated/status.h>



enum at_interface_state
{
    AT_IDLE = 0,
    AT_SENT_COMMAND_START,
    AT_SENT_PARAMETER_1,
    AT_SENT_PARAMETER_N,

    AT_SENT_COMPLETE_TEST,
    AT_SENT_COMPLETE_QUERY,
    AT_SENT_COMPLETE_SET,
    AT_SENT_COMPLETE_EXECUTE
};



struct _at_interface
{
    FILE *tx;
    FILE *rx;
    int timeout;
};



AtInterface at_init(FILE *tx, FILE *rx)
{
    struct _at_interface *iface = malloc(sizeof(struct _at_interface));
    iface->tx = tx;
    iface->rx = rx;
    at_set_response_timeout(iface, 10);
    at_set_echo(iface, false);
    return iface;
}

void at_free(AtInterface iface)
{
    free(iface);
}

void at_set_response_timeout(AtInterface iface, int ms)
{
    iface->timeout = ms;
}



static int _at_get_response(AtInterface iface, char *response, int n, bool quiet)
{
    int64_t start = now();
    while(start + iface->timeout >= now())
    {
        if(fgets(response, n, iface->rx) && strcmp(response, "\r\n") != 0)
        {
            if(!quiet) printf("Received: %s\r\n", response);
            return 0;
        }
        else if(ferror(iface->rx))
        {
            clearerr(iface->rx);
            fseek(iface->rx, 0, SEEK_END);
            printf("Stream error... giving up\r\n");
            return AT_STREAM_ERROR;
        }
        else
        {
            clearerr(iface->rx);
            PCM_gotoLPM0();
        }
    }
    if(!quiet) printf("Timeout reached... giving up\r\n");
    return AT_TIMEOUT_FAIL;
}

static enum at_status _at_check_ok(AtInterface iface)
{
    char response[256] = {0};
    int err = 0;
    do
    {
        err = _at_get_response(iface, response, 256, false);
        if(strstr(response, "OK")) { return AT_OK; }
        if(strstr(response, "ERROR")) { return AT_INVALID_COMMAND; }
        if(strstr(response, "busy p...")) { return AT_BUSY; }
    }
    while(!err);
    return (enum at_status) err;
}



enum at_status at_check_alive(AtInterface iface)
{
    printf("Checking that AT interface is alive...\r\n");
    fputs("AT\r\n", iface->tx);
    return _at_check_ok(iface);
}



enum at_status at_set_echo(AtInterface iface, bool enabled)
{
    if(enabled)
    {
        printf("AT echo on\r\n");
        fputs("ATE1\r\n", iface->tx);
    }
    else
    {
        printf("AT echo off\r\n");
        fputs("ATE0\r\n", iface->tx);
    }
    fseek(iface->rx, 0, SEEK_END);
    return AT_OK;
}



enum at_status esp8266_set_wifi_mode(AtInterface a, enum esp8266_wifi_mode mode)
{
    printf("Setting WiFi mode\r\n");
    fprintf(a->tx, "AT+CWMODE_CUR=%d\r\n", mode);
    return _at_check_ok(a);
}



enum at_status esp8266_connect_to_ap(AtInterface a, struct wifi_ap ap, enum esp8266_wifi_error *resp_error, int32_t timeout)
{
    printf("Connecting to external AP\r\n");
    int32_t old_timeout = a->timeout;
    a->timeout = timeout;
    fprintf(a->tx, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", ap.ssid, ap.passphrase);

    char response[256] = {0};
    int err = 0;
    do
    {
        err = _at_get_response(a, response, 256, false);
        if(strstr(response, "OK")) { err = AT_OK; break; }
        if(strstr(response, "ERROR")) { err = AT_INVALID_COMMAND; }
        if(sscanf(response, "+CWJAP_CUR:%d", &resp_error)) { err = AT_FAIL; }
        if(strstr(response, "FAIL")) { err = AT_FAIL; }
        if(strstr(response, "busy p...")) { err = AT_BUSY; }
    }
    while(!err);
    a->timeout = old_timeout;
    return (enum at_status) err;
}



enum at_status esp8266_disconnect_from_ap(AtInterface a)
{
    printf("Disconnecting from external AP\r\n");
    fprintf(a->tx, "AT+CWQAP\r\n");
    return _at_check_ok(a);
}



enum at_status esp8266_set_hosted_ap(AtInterface a, struct wifi_ap ap, int32_t timeout)
{
    printf("Setting up AP\r\n");
    int32_t old_timeout = a->timeout;
    a->timeout = timeout;
    fprintf(a->tx, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,3\r\n", ap.ssid, ap.passphrase, ap.channel);
    enum at_status err = _at_check_ok(a);
    a->timeout = old_timeout;
    return err;
}



enum at_status esp8266_server_start(AtInterface a, int32_t timeout)
{
    int32_t old_timeout = a->timeout;
    a->timeout = timeout;

    printf("Multiplexing\r\n");
    fputs("AT+CIPMUX=1\r\n", a->tx);
    enum at_status err = _at_check_ok(a);

    printf("Starting server\r\n");
    fputs("AT+CIPSERVER=1,80\r\n", a->tx);
    enum at_status err2 = _at_check_ok(a);
    a->timeout = old_timeout;

    if(err != AT_OK) {return err;}
    if(err2 != AT_OK) {return err2;}
    return AT_OK;
}



static enum at_status _esp8266_check_send_ok(AtInterface a)
{
    char response[256] = {0};
    int err = 0;
    do
    {
        delay_ms(100);
        err = _at_get_response(a, response, 256, false);
        if(strstr(response, "SEND OK")) { return AT_OK; }
        if(strstr(response, "SEND ERROR")) { return AT_INVALID_COMMAND; }
    }
    while(!err);
    return (enum at_status)err;
}

static enum at_status _esp8266_serve_page(AtInterface a, int connection, char const *page)
{
    printf("Starting serve page\r\n");
    fprintf(a->tx, "AT+CIPSEND=%d,%d\r\n", connection, strlen(page));
    enum at_status err = _at_check_ok(a);

    if(err == AT_OK)
    {
        printf("Sending data\r\n");
        fputs(page, a->tx);
        err = _esp8266_check_send_ok(a);
    }

    printf("Closing connection\r\n");
    fprintf(a->tx, "AT+CIPCLOSE=%d\r\n", connection);
    enum at_status err2 = _at_check_ok(a);

    if(err != AT_OK) {return err;}
    if(err2 != AT_OK) {return err2;}
    return AT_OK;
}

void esp8266_server_periodic(AtInterface a, PageGen gen, PageState state)
{
    bool received_request = false;
    char line[256];

    int connection;
    int message_len;
    char message_type[32];
    char message_first_arg[256];
    int at_err = 0;
    while(at_err == 0)
    {
        at_err = _at_get_response(a, line, 256, true);
        int c = sscanf(line, " +IPD,%d,%d:%s %s", &connection, &message_len, &message_type[0], &message_first_arg[0]);
        if(c == 4)
        {
            printf("+++Received a request!+++\r\n");
            received_request = true;
            break;
        }
    }
    if(at_err == AT_STREAM_ERROR)
    {
        fseek(a->rx, 0, SEEK_END);
        clearerr(a->rx);
    }
    if(!received_request) {return;}
    while(_at_get_response(a, line, 256, false) == 0)
    {
        // eat the rest of the request, we don't care
    }

    if(strcmp(message_first_arg, "/") == 0)
    {
        char page[1024];
        gen(state, page, 1024);

        _esp8266_serve_page(a, connection, page);
    }
    else
    {
        _esp8266_serve_page(a, connection, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
    }
}
