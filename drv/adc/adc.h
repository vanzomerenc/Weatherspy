/*
 * adc.h
 *
 *  Created on: Feb 6, 2018
 *      Author: chris
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdbool.h>
#include <stdint.h>

#define ADC_MAX_NUM_CHANNELS 32
#define ADC_MAX_INPUT_ID 24

#define ADC_MAX_READING (1 << 14)
#define ADC_HIGH_RANGE_MAX_VOLTAGE 3.3f
#define ADC_LOW_RANGE_MAX_VOLTAGE 1.2f

struct adc_channel_config
{
    int input_id;
    bool is_high_range;
};

enum adc_error
{
    ADC_TOO_MANY_CHANNELS = 1,
    ADC_INVALID_CHANNEL = 2,
    ADC_INVALID_INPUT_ID = 3
};

int adc_init(int num_channels, struct adc_channel_config *channels);

int adc_get_all_raw(int16_t *results);
int adc_get_single_raw(int channel, int16_t *result);

float adc_convert(int16_t raw, bool is_high_range);

int adc_get_single(int channel, float *result);

#endif /* ADC_H_ */
