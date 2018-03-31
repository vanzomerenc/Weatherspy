/*
 * tty_buf.c
 *
 *  Created on: Mar 31, 2018
 *      Author: Chris van Zomeren
 */

#include "line_buf.h"
#include <stdlib.h>

#define LINE_BUF_SIZE 256

struct _line_buf
{
    enum line_read_state state;
    int start;
    int end;
    char buf[LINE_BUF_SIZE];
};

LineBuf line_buf_alloc()
{
    struct _line_buf *result = malloc(sizeof(struct _line_buf));
    result->state = LINE_FILLING;
    result->start = 0;
    result->end = 0;
    return result;
}

void line_buf_free(LineBuf buf)
{
    free(buf);
}

enum line_read_state line_get_read_state(LineBuf buf)
{
    return buf->state;
}

int line_buf_put_char(LineBuf buf, int c)
{
    if(buf->state != LINE_FILLING) { return -1; }
    if(c < 0) { return -1; }

    buf->buf[buf->end] = c;
    buf->end++;
    if(c == '\n' || buf->end == LINE_BUF_SIZE - 1)
    {
        buf->start = 0;
        buf->state = LINE_DRAINING;
    }
    return 0;
}

int line_buf_get_line(LineBuf buf, char *out, int n)
{
    if(buf->state != LINE_DRAINING) { return -1; }

    char *start = buf->buf + buf->start;
    int len = buf->end - buf->start;
    if(len > n) { len = n; }
    memcpy(out, start, len);

    buf->start += len;
    if(buf->start == buf->end)
    {
        line_buf_reset(buf);
    }
    return len;
}

void line_buf_reset(LineBuf buf)
{
    buf->end = 0;
    buf->state = LINE_FILLING;
}
