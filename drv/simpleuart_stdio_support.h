#pragma once
/*
 * uart_io_support.h
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris van Zomeren
 *
 * Support module for treating UART devices as files.
 * This module uses the interface in simple_uart.h
 * to control the hardware UART modules. Using this
 * module and simpleuart on the same interface should
 * be avoided.
 *
 * A device is presented as a pair of one read-only (rx)
 * and one write-only (tx) stream, using uart_open().
 * Reading and writing is done using stream operations
 * from the standard library (<stdio.h>). When finished
 * using a stream, it may be closed as normal using fclose().
 *
 * Writing is unbuffered: written data is transmitted
 * immediately over the UART interface.
 *
 * Reading is single buffered: received data is put into a
 * buffer to be read later by application code, but the application
 * is expected to read this buffer before it is full.
 *
 * The receive buffer can only hold 255 bytes at a time.
 * If more than 255 bytes are stored in the receive buffer,
 * then the buffer is emptied and future reads will fail
 * until an explicit seek to the end of the stream
 * (i.e. fseek(s, 0, SEEK_END)) is performed.
 * This is to prevent silently receiving corrupted data.
 */

#include <stdio.h>

#include "simpleuart.h"

struct uart_channel
{
    FILE *tx;
    FILE *rx;
};

int uart_use_stdio_support();

// Use this function to open a UART interface.
// Do not use open() or fopen(): they do not initialize the
// interface before trying to open it, and will fail.
struct uart_channel uart_open(struct uart_config config);
