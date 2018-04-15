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
    iface->timeout = 1;
    fputs("ATE0\r\n", iface->tx);
    fseek(rx, 0, SEEK_END);
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



int _at_get_response(AtInterface iface, char *response, int n)
{
    int64_t start = now();
    while(start + iface->timeout >= now())
    {
        if(fgets(response, n, iface->rx) && strcmp(response, "\r\n") != 0)
        {
            return 0;
        }
        else if(ferror(iface->rx))
        {
            fseek(iface->rx, 0, SEEK_END);
            return AT_STREAM_ERROR;
        }
        else
        {
            clearerr(iface->rx);
            PCM_gotoLPM0();
        }
    }
    return AT_TIMEOUT_FAIL;
}

enum at_status at_check_alive(AtInterface iface)
{
    fputs("AT\r\n", iface->tx);
    char response[256] = {0};
    int err = 0;
    do
    {
        err = _at_get_response(iface, response, 256);
        if(strstr(response, "OK")) { return AT_OK; }
        if(strstr(response, "ERROR")) { return AT_INVALID_COMMAND; }
    }
    while(!err);
    return err;
}
