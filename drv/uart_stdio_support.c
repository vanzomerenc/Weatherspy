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

#define UART_NUM_DEVICES 4
#define UART_RECEIVE_BUFFER_SIZE BUFSIZ


struct _uart_dev
{
    UartInterface interface;
    unsigned flags;
    int fd;
    uint16_t buf_trailing;
    uint16_t buf_leading;
    char read_buf[UART_RECEIVE_BUFFER_SIZE];
};



static struct _uart_dev _uart_dev[UART_NUM_DEVICES] =
{
 (struct _uart_dev) {NULL, 0, -1, 0, 0, 0},
 (struct _uart_dev) {NULL, 0, -1, 0, 0, 0},
 (struct _uart_dev) {NULL, 0, -1, 0, 0, 0},
 (struct _uart_dev) {NULL, 0, -1, 0, 0, 0}
};



void _uart_buffer_fill_callback(struct _uart_dev *context, char data)
{
    // TODO: is it better to miss reads, or writes? currently we miss writes

    // We want to keep a separation of at least 1 between the trailing
    // and the next leading edge of the buffer.
    if((context->buf_leading + 1) % UART_RECEIVE_BUFFER_SIZE != context->buf_trailing)
    {
        context->read_buf[context->buf_leading] = data;
        context->buf_leading = (context->buf_leading + 1) % UART_RECEIVE_BUFFER_SIZE;
    }
}



int _uart_parse_config(char const *config_string, struct uart_config *result)
{
    int dev_num;
    uint32_t baud_rate;
    int n_found = sscanf(config_string, "%*[d]%*[e]%*[v]%i%*[@]%lu", &dev_num, &baud_rate);
    if(
            n_found == 2 // Should match the number of varargs to sscanf
            && (dev_num >= 0 && dev_num < 4))
    {
        result->id = dev_num;
        result->baud_rate = baud_rate;
        result->flags = 0; // TODO: implement handling of flags
        return 0;
    }
    else
    {
        return -1;
    }
}

static int _uart_get_index_from_fd(int fd)
{
    for(int i = 0; i < UART_NUM_DEVICES; i++)
    {
        if(_uart_dev[i].fd == fd)
        {
            return i;
        }
    }
    return -1;
}

static int _uart_open(char const *path, unsigned flags, int llv_fd)
{
    struct uart_config config;
    int last_error = _uart_parse_config(path, &config);
    if(last_error != 0) { return last_error; }
    if(_uart_dev[config.id].interface != NULL) { return -1; }

    struct _uart_dev *dev = &_uart_dev[config.id];

    dev->interface = uart_init(config);
    dev->flags = flags;
    dev->fd = llv_fd;
    dev->buf_leading = 0;
    dev->buf_trailing = 0;

    struct uart_receive_callback receive_callback;
    receive_callback.callback = _uart_buffer_fill_callback;
    receive_callback.context = dev;

    uart_set_receive_handler(dev->interface, receive_callback);

    uart_enable(dev->interface);

    return dev->fd;
}

static int _uart_close(int dev_fd)
{
    int dev_index = _uart_get_index_from_fd(dev_fd);
    if(dev_index < 0) { return dev_index; }

    uart_disable(_uart_dev[dev_index].interface);
    _uart_dev[dev_index] = (struct _uart_dev) {NULL, 0, 0};
    return 0;
}

static int _uart_read(int dev_fd, char *buf, unsigned count)
{
    int dev_index =_uart_get_index_from_fd(dev_fd);
    if(dev_index < 0) { return dev_index; }

    struct _uart_dev *dev = &_uart_dev[dev_index];

    int num_read = 0;
    while(
            dev->buf_trailing != dev->buf_leading
            && num_read < count)
    {
        buf[num_read] = dev->read_buf[dev->buf_trailing];
        dev->buf_trailing = (dev->buf_trailing + 1) % UART_RECEIVE_BUFFER_SIZE;
        num_read++;
    }

    return num_read;
}

static int _uart_write(int dev_fd, char const *buf, unsigned count)
{
    int dev_index = _uart_get_index_from_fd(dev_fd);
    if(dev_index < 0) { return dev_index; }

    struct _uart_dev *dev = &_uart_dev[dev_index];
    uart_send_bytes(dev->interface, count, buf);
    return count;
}

static off_t _uart_lseek(int dev_fd, off_t offset, int origin)
{
    // Pretend we're seeking to make the standard library happy.
    return 0;
}

static int _uart_unlink(char const *path)
{
    return -1;
}

static int _uart_rename(char const *old_name, char const *new_name)
{
    return -1;
}



int uart_use_stdio_support()
{
    return add_device(
            "uart",
            _MSA,
            _uart_open,
            _uart_close,
            _uart_read,
            _uart_write,
            _uart_lseek,
            _uart_unlink,
            _uart_rename);
}
