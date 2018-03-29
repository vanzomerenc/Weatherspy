// AUTHOR: Chris van Zomeren

#include "light_level.h"

#include <drv/adc/adc.h>



static int adc_channel_id = 0;

#define LUX_APPROXIMATION_N_DATA_POINTS 6

static float volt_points[LUX_APPROXIMATION_N_DATA_POINTS] =
{
 0.032673267326732675,
 0.175,
 0.7878197320341047,
 2.102681992337165,
 2.995417304136713,
 3.241148332567477
};

static float lux_points[LUX_APPROXIMATION_N_DATA_POINTS] =
{
 10,
 100,
 1000,
 10000,
 100000,
 1000000
};



int sensor_light_level_init()
{
    return 0;
}

int sensor_light_level_read(float *out_light_level)
{
    float voltage;
    enum adc_error err = adc_get_single(adc_channel_id, &voltage);
    if(err) return err;

    int approximation_interval = 0;
    while(approximation_interval < LUX_APPROXIMATION_N_DATA_POINTS - 1)
    {
        if(volt_points[approximation_interval] < voltage) approximation_interval++;
        else break;
    }

    if(approximation_interval == 0)
    {
        *out_light_level = 0;
    }
    else
    {
        int upper = approximation_interval;
        int lower = upper - 1;
        *out_light_level =
                ((voltage - volt_points[lower]) / (volt_points[upper] - volt_points[lower]))
                * (lux_points[upper] - lux_points[lower])
                + lux_points[lower];
    }
    return 0;
}
