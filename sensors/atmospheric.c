/*
 * atmospheric.c
 *
 *  Created on: Feb 19, 2018
 *      Author: Chris
 */


#include "atmospheric.h"

#include "driverlib.h"
#include "drv/BME280/bme280.h"
#include "drv/i2c/i2c.h"
#include "drv/timing.h"



int sensor_atmospheric_init(struct bme280_dev *out_dev)
{
    I2C_Init();

    *out_dev = (struct bme280_dev)
    {
        .dev_id = BME280_I2C_ADDR_PRIM,
        .intf = BME280_I2C_INTF,
        .read = &I2C_WRITE_READ_STRING,
        .write = &I2C_WRITE_STRING,
        .delay_ms = &delay_ms,
        .settings.osr_h = BME280_OVERSAMPLING_1X,
        .settings.osr_p = BME280_OVERSAMPLING_1X,
        .settings.osr_t = BME280_OVERSAMPLING_1X,
        .settings.filter = BME280_FILTER_COEFF_OFF
    };
    bme280_init(out_dev);
    bme280_set_sensor_settings(BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL, out_dev);

    return 0;
}

int sensor_atmospheric_read(struct bme280_dev *dev, struct sensor_atmospheric_result *out_result)
{

    bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    struct bme280_data sensor_data = {0};
    int8_t err = bme280_get_sensor_data(BME280_ALL, &sensor_data, dev);

    out_result->pressure = sensor_data.pressure * 0.000002953 + PRESSURE_ALTITUDE_CORRECTION;
    out_result->temperature = (sensor_data.temperature * 0.01) * (9.0 / 5.0) + 32.0;
    out_result->humidity = sensor_data.humidity * (1.0 / 1024.0);

    return 0;
}
