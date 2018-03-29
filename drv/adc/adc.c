/*
 * adc.c
 *
 *  Created on: Feb 6, 2018
 *      Author: chris
 */

#include <stdint.h>

#include "driverlib.h"

#include "adc.h"


#define BIT32(N) ((uint32_t)1 << (N))

#define ADC_MEM(N) BIT32(N)
#define ADC_INPUT_A(N) (N)
#define ADC_INT(N) BIT32(N)


struct gpio_pin {uint16_t port; uint16_t pin;};

static struct gpio_pin pins[24] =
{
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN5}, //  0
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN4}, //  1
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN3}, //  2
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN2}, //  3
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN1}, //  4
 {.port = GPIO_PORT_P5, .pin = GPIO_PIN0}, //  5
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN7}, //  6
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN6}, //  7
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN5}, //  8
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN4}, //  9
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN3}, // 10
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN2}, // 11
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN1}, // 12
 {.port = GPIO_PORT_P4, .pin = GPIO_PIN0}, // 13
 {.port = GPIO_PORT_P6, .pin = GPIO_PIN1}, // 14
 {.port = GPIO_PORT_P6, .pin = GPIO_PIN0}, // 15
 {.port = GPIO_PORT_P9, .pin = GPIO_PIN1}, // 16
 {.port = GPIO_PORT_P9, .pin = GPIO_PIN0}, // 17
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN7}, // 18
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN6}, // 19
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN5}, // 20
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN4}, // 21
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN3}, // 22
 {.port = GPIO_PORT_P8, .pin = GPIO_PIN2}, // 23
 /* ... */
};



static int adc_num_channels = {0};
static struct adc_channel_config adc_channels[ADC_MAX_NUM_CHANNELS] = {0};
static int16_t adc_result[ADC_MAX_NUM_CHANNELS] = {0};



int adc_init(int num_channels, struct adc_channel_config *channels)
{
    if(num_channels > ADC_MAX_NUM_CHANNELS) return -ADC_TOO_MANY_CHANNELS;
    for(int i = 0; i < num_channels; i++)
    {
        if(channels[i].input_id < 0) return -ADC_INVALID_INPUT_ID;
        if(channels[i].input_id > ADC_MAX_INPUT_ID) return -ADC_INVALID_INPUT_ID;
    }

    /* Initializing reference voltage */
    MAP_REF_A_setReferenceVoltage(REF_A_VREF1_2V);
    MAP_REF_A_enableReferenceVoltage();

    /* Initializing ADC (MCLK/1/4) */
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0);

    /* GPIO pins */
    for(int i = 0; i < num_channels; i++)
    {
        int input_id = channels[i].input_id;

        MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
                pins[input_id].port,
                pins[input_id].pin,
                GPIO_TERTIARY_MODULE_FUNCTION);
    }

    /* Configuring sample mode */
    MAP_ADC14_configureMultiSequenceMode(ADC_MEM(0), ADC_MEM(num_channels - 1), true);

    /* Configuring conversion modes */
    for(int i = 0; i < num_channels; i++)
    {
        MAP_ADC14_configureConversionMemory(
                ADC_MEM(i),
                channels[i].is_high_range?
                        ADC_VREFPOS_AVCC_VREFNEG_VSS
                        : ADC_VREFPOS_INTBUF_VREFNEG_VSS,
                ADC_INPUT_A(i), false);
    }

    /* Configuring Sample Timer */
    MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

    /* Enabling/Toggling Conversion */
    MAP_ADC14_enableConversion();

    /* Enabling interrupts */
    for(int i = 0; i < num_channels; i++)
    {
        MAP_ADC14_enableInterrupt(ADC_INT(i));
    }

    /* Setting globals needed in interrupt handler */
    adc_num_channels = num_channels;
    for(int i = 0; i < adc_num_channels; i++)
    {
        adc_channels[i] = channels[i];
    }

    /* Enabling master ADC interrupt */
    MAP_Interrupt_enableInterrupt(INT_ADC14);

    return 0;
}



void handle_ADC_interrupt()
{
    uint64_t status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);
    for(int i = 0; i < adc_num_channels; i++)
    {
        uint32_t flag = BIT32(i);
        if(flag & status)
        {
            adc_result[i] = MAP_ADC14_getResult(flag);
        }
    }
}



int adc_get_all_raw(int16_t *results)
{
    for(int i = 0; i < adc_num_channels; i++)
    {
        results[i] = adc_result[i];
    }

    return 0;
}



int adc_get_single_raw(int channel, int16_t *result)
{
    if(channel < 0) return -ADC_INVALID_CHANNEL;
    if(channel > adc_num_channels) return -ADC_INVALID_CHANNEL;
    *result = adc_result[channel];
    return 0;
}



float adc_convert(int16_t raw, bool is_high_range)
{
    return raw * (is_high_range? ADC_HIGH_RANGE_MAX_VOLTAGE : ADC_LOW_RANGE_MAX_VOLTAGE) / ADC_MAX_READING;
}



int adc_get_single(int channel, float *result)
{
    int16_t raw;
    enum adc_error err = adc_get_single_raw(channel, &raw);
    if(err) return err;
    *result = adc_convert(raw, adc_channels[channel].is_high_range);
    return 0;
}
