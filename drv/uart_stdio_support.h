#pragma once
/*
 * uart_io_support.h
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris van Zomeren
 */

#include <drv/uart.h>

enum uart_buffer_mode
{
    UART_BUFFER_NONE = 0,
    UART_BUFFER_LINE = 1,
    UART_BUFFER_FULL = 3
};

struct uart_input_config
{
    bool complete_lines;
    bool echo;
};

struct uart_output_config
{
};

struct uart_channel
{
    FILE *tx;
    FILE *rx;
};

int uart_use_stdio_support();

// Use this function to open a UART interface.
// Do not use open() or fopen(): they do not initialize the
// interface before trying to open it, and will fail.
struct uart_channel uart_open(
    struct uart_config config,
    struct uart_input_config input_config,
    struct uart_output_config output_config);
