/*
 * uart_io_support.c
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <file.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>

#define UART_NUM_CHANNELS 4
#define UART_RECEIVE_BUFFER_SIZE BUFSIZ


struct _uart_channel
{
    int write_fd;                          // Writes to channel must know this fd
    int read_fd;                           // Reads from channel must know this fd
    UartInterface interface;               // UART interface associated with the channel
    uint16_t rx_buf_trailing;              // -,
    uint16_t rx_buf_leading;               //  |- ring buffer holding received data
    char rx_buf[UART_RECEIVE_BUFFER_SIZE]; // -'
};

static struct _uart_channel _uart_channel[UART_NUM_CHANNELS] =
{
 (struct _uart_channel) {-1, -1, NULL, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0}
};



// This is set as the function called by the UART interface when a byte is received.
// Received data is placed in the ring buffer for the receiving channel.
void _uart_buffer_fill_callback(struct _uart_channel *context, char data)
{
    // TODO: is it better to miss reads, or writes? currently we miss writes

    // We want to keep a separation of at least 1 between the trailing
    // and the next leading edge of the buffer.
    if((context->rx_buf_leading + 1) % UART_RECEIVE_BUFFER_SIZE != context->rx_buf_trailing)
    {
        context->rx_buf[context->rx_buf_leading] = data;
        context->rx_buf_leading = (context->rx_buf_leading + 1) % UART_RECEIVE_BUFFER_SIZE;
    }
}

// Gets the channel which owns fd, or NULL if no channel owns fd.
static struct _uart_channel *_uart_get_fd_channel(int fd)
{
    for(int i = 0; i < UART_NUM_CHANNELS; i++)
    {
        if(
            _uart_channel[i].write_fd == fd
            || _uart_channel[i].read_fd == fd)
        {
            return &_uart_channel[i];
        }
    }
    return NULL;
}

// Opens a single fd for either read or write on a UART channel.
// Will fail if the module for the channel has not been initialized,
// if the given direction (read or write) has already been opened,
// or if attempting to open both read and write with the same fd.
// Called by open(), fopen(), &c.
static int _uart_open_one_way(char const *path, unsigned flags, int llv_fd)
{
    // Check that we have a valid device id
    if(strlen(path) != 1) { return -1; }
    int dev_num = path[0] - '0';
    if(dev_num < 0 || dev_num >= UART_NUM_CHANNELS) { return -1; }

    // Do not allow read and write on the same fd
    int mode = flags & 0x03;
    if(mode != O_RDONLY && mode != O_WRONLY) { return -1; }

    struct _uart_channel *dev = &_uart_channel[dev_num];

    // Do not allow opening the same direction multiple times
    if(mode == O_RDONLY && dev->read_fd >= 0) { return -1; }
    if(mode == O_WRONLY && dev->write_fd >= 0) { return -1; }

    // Do not allow using an interface which hasn't been initialized
    if(dev->interface == NULL) { return -1; }


    if(mode == O_RDONLY)
    {
        dev->read_fd = llv_fd;
        dev->rx_buf_leading = 0;
        dev->rx_buf_trailing = 0;
    }
    if(mode == O_WRONLY)
    {
        dev->write_fd = llv_fd;

    }

    return llv_fd;
}

// Closes a single fd.
// If both read and write are closed, then the interface
// is disabled and the channel reset.
// called by close(), fclose(), &c.
static int _uart_close_one_way(int dev_fd)
{
    struct _uart_channel *dev = _uart_get_fd_channel(dev_fd);
    if(dev == NULL) { return -1; }

    if(dev_fd == dev->read_fd)
    {
        dev->read_fd = -1;
    }
    if(dev_fd == dev->write_fd)
    {
        dev->write_fd = -1;
    }
    if(dev->read_fd < 0 && dev->write_fd < 0)
    {
        uart_disable(dev->interface);
        dev->interface = NULL;
    }

    return 0;
}

// Reads from dev_fd if dev_fd is the read_fd for a channel.
// Fails if dev_fd is not a read_fd or is not owned by a channel.
// Called by read(), fgetc(), fgets(), &c.
static int _uart_read(int dev_fd, char *buf, unsigned count)
{
    struct _uart_channel *dev = _uart_get_fd_channel(dev_fd);
    if(dev == NULL) { return -1; }
    if(dev_fd != dev->read_fd) { return -1; }

    int num_read = 0;
    while(
            dev->rx_buf_trailing != dev->rx_buf_leading
            && num_read < count)
    {
        buf[num_read] = dev->rx_buf[dev->rx_buf_trailing];
        dev->rx_buf_trailing = (dev->rx_buf_trailing + 1) % UART_RECEIVE_BUFFER_SIZE;
        num_read++;
    }

    return num_read;
}

// Writes to dev_fd if dev_fd is the write_fd for a channel.
// Fails if dev_fd is not a write_fd or is not owned by a channel.
// Called by write(), fputc(), fputs(), &c.
static int _uart_write(int dev_fd, char const *buf, unsigned count)
{
    struct _uart_channel *dev = _uart_get_fd_channel(dev_fd);
    if(dev == NULL) { return -1; }
    if(dev_fd != dev->write_fd) { return -1; }

    uart_send_bytes(dev->interface, count, buf);
    return count;
}

// Pretends to seek, to make the standard library happy.
// This is a stream though, so seeking does not make much sense.
// Called by lseek(), fseek(), &c.
static off_t _uart_lseek(int dev_fd, off_t offset, int origin)
{
    return 0;
}

// We have a fixed set of devices, so unlinking does not make sense.
// Called by unlink().
static int _uart_unlink(char const *path)
{
    return -1;
}

// We have a fixed set of devices, so renaming does not make sense.
// Called by rename().
static int _uart_rename(char const *old_name, char const *new_name)
{
    return -1;
}



int uart_use_stdio_support()
{
    return add_device(
            "uart",
            _MSA,
            _uart_open_one_way,
            _uart_close_one_way,
            _uart_read,
            _uart_write,
            _uart_lseek,
            _uart_unlink,
            _uart_rename);
}

struct uart_channel uart_open(struct uart_config config)
{
    struct uart_channel result = {.tx = NULL, .rx = NULL};

    if(config.id < 0 || config.id > UART_NUM_CHANNELS) { return result; }
    struct _uart_channel *dev = &_uart_channel[config.id];

    if(dev->interface != NULL) { return result; }

    dev->interface = uart_init(config);

    struct uart_receive_callback receive_callback;
    receive_callback.callback = _uart_buffer_fill_callback;
    receive_callback.context = dev;
    uart_set_receive_handler(dev->interface, receive_callback);

    uart_enable(dev->interface);

    // Open the tx and rx streams, letting the runtime support library
    // take over.
    // We call setvbuf here because buffering interferes with timely
    // transmission and reception, and because we already have our own
    // receive buffers anyway.
    char dev_path[7] = "uart:_";
    dev_path[5] = config.id + '0';
    result.tx = fopen(dev_path, "w");
    setvbuf(result.tx, NULL, _IONBF, 0);
    result.rx = fopen(dev_path, "r");
    setvbuf(result.rx, NULL, _IONBF, 0);

    return result;
}
