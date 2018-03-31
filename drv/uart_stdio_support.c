/*
 * uart_io_support.c
 *
 *  Created on: Mar 25, 2018
 *      Author: Chris van Zomeren
 *
 * TODO: set errno on errors
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <file.h>
#include <string.h>

#include <drv/uart.h>
#include <drv/uart_stdio_support.h>
#include <drv/line_buf.h>

#define UART_NUM_CHANNELS 4
#define UART_RECEIVE_BUFFER_SIZE BUFSIZ


struct _uart_channel
{
    int write_fd;                          // Writes to channel must know this fd
    int read_fd;                           // Reads from channel must know this fd
    UartInterface interface;               // UART interface associated with the channel
    bool echo;                             // Enable or disable receive echo
    LineBuf line_buf;                      // Optional, used for tty-like line buffering
    uint16_t rx_buf_trailing;              // -,
    uint16_t rx_buf_leading;               //  |- ring buffer holding received data
    bool rx_data_was_lost;                 //  |
    char rx_buf[UART_RECEIVE_BUFFER_SIZE]; // -'
};

static struct _uart_channel _uart_channel[UART_NUM_CHANNELS] =
{
 (struct _uart_channel) {-1, -1, NULL, 0, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0, 0},
 (struct _uart_channel) {-1, -1, NULL, 0, 0, 0}
};



// This is set as the function called by the UART interface when a byte is received.
// Received data is placed in the ring buffer for the receiving channel.
// If the leading edge of the buffer reaches the trailing edge, the program has
// fallen too far behind and must discard data to catch up.
// Once any data is lost, the entire receive stream is considered compromised,
// including data which has already been received and is sitting in the receive
// buffer. The only way to recover from data loss is to seek to the end of the
// receive stream, or to close and reopen the stream.
void _uart_buffer_fill_callback(struct _uart_channel *context, char data)
{
    context->rx_buf[context->rx_buf_leading] = data;
    context->rx_buf_leading = (context->rx_buf_leading + 1) % UART_RECEIVE_BUFFER_SIZE;
    if(context->rx_buf_leading == context->rx_buf_trailing)
    {
        context->rx_data_was_lost = true;
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
        line_buf_free(dev->line_buf);
        dev->line_buf = NULL;
    }

    return 0;
}



static int _uart_read_unbuffered_unsafe(struct _uart_channel *dev, char *buf, unsigned count)
{
    int num_read = 0;
    while(
            dev->rx_buf_trailing != dev->rx_buf_leading
            && num_read < count)
    {
        char c = dev->rx_buf[dev->rx_buf_trailing];
        if(dev->echo) { uart_send_byte(dev->interface, c); }
        buf[num_read] = c;
        dev->rx_buf_trailing = (dev->rx_buf_trailing + 1) % UART_RECEIVE_BUFFER_SIZE;
        num_read++;
    }
    return num_read;
}

static int _uart_read_line_buffered_unsafe(struct _uart_channel *dev, char *buf, unsigned count)
{
    while(
            dev->rx_buf_trailing != dev->rx_buf_leading
            && line_get_read_state(dev->line_buf) == LINE_FILLING)
    {
        char c = dev->rx_buf[dev->rx_buf_trailing];
        if(dev->echo) { uart_send_byte(dev->interface, c); }
        line_buf_put_char(dev->line_buf, c);
        dev->rx_buf_trailing = (dev->rx_buf_trailing + 1) % UART_RECEIVE_BUFFER_SIZE;
    }
    if(line_get_read_state(dev->line_buf) == LINE_DRAINING)
    {
        return line_buf_get_line(dev->line_buf, buf, count);
    }
    else
    {
        return 0;
    }
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
    if(dev->line_buf)
    {
        num_read = _uart_read_line_buffered_unsafe(dev, buf, count);
    }
    else
    {
        num_read = _uart_read_unbuffered_unsafe(dev, buf, count);
    }

    // This check has to happen _after_ we've already read the data,
    // because the receive interrupt might have overwritten it while
    // we were reading.
    // We check once at the end, instead of on each byte, for a few reasons:
    //   1) We want a read to be as fast as possible while there's data to read.
    //   2) Data loss means we either stopped caring about the stream for a while,
    //      or we have already severely failed to keep up with it.
    //   3) The data is coming from an external source which is already not
    //      perfectly predictable. It makes little difference whether we lose data
    //      from time t or time t - x, for sufficiently small x.
    // See _uart_buffer_fill_callback.
    if(dev->rx_data_was_lost)
    {
        return -1;
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

// Seeks to the given offset in dev_fd relative to origin.
// Seeking to the end of the stream (0, SEEK_END) will always
// succeed, and will recover from data loss if used on a receiving
// stream. Seeking to any other point will always fail.
// Called by lseek(), fseek(), &c.
static off_t _uart_lseek(int dev_fd, off_t offset, int origin)
{
    struct _uart_channel *dev = _uart_get_fd_channel(dev_fd);

    if(offset == 0 && origin == SEEK_END)
    {
        if(dev_fd == dev->read_fd)
        {
            if(dev->line_buf)
            {
                line_buf_reset(dev->line_buf);
            }
            dev->rx_buf_trailing = dev->rx_buf_leading;
            dev->rx_data_was_lost = false;
        }

        return 0;
    }
    else
    {
        return -1;
    }
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



struct uart_channel uart_open(
    struct uart_config config,
    struct uart_input_config input_config,
    struct uart_output_config output_config)
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

    if(input_config.complete_lines)
    {
        if(dev->line_buf) { line_buf_reset(dev->line_buf); }
        else { dev->line_buf = line_buf_alloc(); }
    }
    else
    {
        line_buf_free(dev->line_buf);
    }

    dev->echo = input_config.echo;

    uart_enable(dev->interface);

    // Open the tx and rx streams, letting the runtime support library
    // take over.
    char dev_path[7] = "uart:_";
    dev_path[5] = config.id + '0';
    result.tx = fopen(dev_path, "w");
    result.rx = fopen(dev_path, "r");

    // The buffering done by the standard library will always give us
    // whatever it has. If we want line buffering that always gives full lines,
    // we have to do it ourselves, so we won't need the standard buffering anyway.
    setvbuf(result.tx, NULL, _IONBF, 0);
    setvbuf(result.rx, NULL, _IONBF, 0);

    return result;
}
