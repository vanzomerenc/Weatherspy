#pragma once

// timing.h
// AUTHOR: Chris van Zomeren
// This module provides functions to set
// clock frequencies and implement delays
// of various lengths.

#include "driverlib.h"
#include <stdint.h>

typedef uint32_t ClockSignalID;
typedef uint32_t ClockFrequency;



#define kHz ((uint32_t)    1000)
#define MHz ((uint32_t) 1000000)



// Initializes all clocks used by the program.
// TODO: make the clock speeds of the various clocks configurable
void init_clocks(void);

// Expects the clock given by clock to have the frequency freq.
// This function returns only if this expectation is met.
// If the frequency of clock is not freq, this function does not return.
ClockFrequency get_frequency(ClockSignalID clock);

// Delays for n milliseconds by entering a tight loop.
void delay_spin_ms(uint32_t n);

// Delays for n milliseconds.
void delay_ms(uint32_t n);
