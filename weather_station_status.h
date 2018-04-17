/*
 * weather_station_status.h
 *
 *  Created on: Jan 20, 2018
 *      Author: chris
 */

#ifndef WEATHER_STATION_STATUS_H_
#define WEATHER_STATION_STATUS_H_

#include <stdint.h>
#include "drv/rtc.h"

enum lighting_condition
{
    lighting_dark,
    lighting_twilight,
    lighting_overcast,
    lighting_partly_sunny,
    lighting_sunny
};

struct weather_station_status
{
    enum lighting_condition lighting;
    char local_condition[20];
    float outdoor_temperature;
    float indoor_temperature;
    float outdoor_humidity;
    float indoor_humidity;
    float pressure;
    float out_voltage;
    float out_current;
    struct rtc_time time;
};

#endif /* WEATHER_STATION_STATUS_H_ */
