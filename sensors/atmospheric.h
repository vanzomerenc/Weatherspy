/*
 * press_temp_hum.h
 *
 *  Created on: Feb 19, 2018
 *      Author: Chris
 */

#ifndef SENSORS_ATMOSPHERIC_H_
#define SENSORS_ATMOSPHERIC_H_

#define PRESSURE_ALTITUDE_CORRECTION 0.64 // @ 600 ft above sea level

#include "drv/BME280/bme280.h"

struct sensor_atmospheric_result
{
    float pressure;
    float temperature;
    float humidity;
};

int sensor_atmospheric_init(struct bme280_dev *out_dev);

int sensor_atmospheric_read(struct bme280_dev *dev, struct sensor_atmospheric_result *out_result);

#endif /* SENSORS_ATMOSPHERIC_H_ */
