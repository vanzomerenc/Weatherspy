#pragma once
/*
 * esp8266.h
 *
 *  Created on: Mar 24, 2018
 *      Author: Chris van Zomeren
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum at_status
{
    AT_OK = 0,
    AT_FAIL = -1,
    AT_TIMEOUT_FAIL = -2,
    AT_INVALID_COMMAND = -3,
    AT_UNEXPECTED_RESPONSE = -4,
    AT_STREAM_ERROR = -5,
    AT_BUSY = -6
};

enum esp8266_wifi_mode
{
    WIFI_MODE_STATION = 1,
    WIFI_MODE_SOFT_AP = 2,
    WIFI_MODE_SOFT_AP_AND_STATION = 3
};

enum esp8266_wifi_error
{
    WIFI_ERROR_TIMEOUT = 1,
    WIFI_ERROR_PASSWORD = 2,
    WIFI_ERROR_SSID = 3,
    WIFI_ERROR_CONNECTION = 4
};

enum esp8266_wifi_encryption_method
{
    WIFI_OPEN,
    WIFI_WPA_PSK,
    WIFI_WPA2_PSK,
    WIFI_WPA_WPA2_PSK
};

struct wifi_ap
{
    char *ssid;
    int ssid_buf_size;
    char *passphrase;
    int passphrase_buf_size;
    char *bssid;
    int bssid_buf_size;
    enum esp8266_wifi_encryption_method encryption;
    int channel;
    int max_connections;
    bool hidden;
};

struct _at_interface;
typedef struct _at_interface *AtInterface;

AtInterface at_init(FILE *tx, FILE *rx);
void at_free(AtInterface iface);

void at_set_response_timeout(AtInterface iface, int ms);

enum at_status at_check_alive(AtInterface iface);
enum at_status at_set_echo(AtInterface a, bool enabled);

enum at_status esp8266_set_wifi_mode(AtInterface a, enum esp8266_wifi_mode mode);

enum at_status esp8266_connect_to_ap(AtInterface a, struct wifi_ap ap, enum esp8266_wifi_error *resp_error, int32_t timeout);
enum at_status esp8266_disconnect_from_ap(AtInterface a);

enum at_status esp8266_set_hosted_ap(AtInterface a, struct wifi_ap ap, int32_t timeout);

typedef int(*PageGen)(void *, char *, int);
typedef void *PageState;
enum at_status esp8266_server_start(AtInterface a, int32_t timeout);
void esp8266_server_periodic(AtInterface a, PageGen gen, PageState state);
