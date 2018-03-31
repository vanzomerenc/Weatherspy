#pragma once
/*
 * tty_buf.h
 *
 *  Created on: Mar 31, 2018
 *      Author: Chris van Zomeren
 */

enum line_read_state
{
    LINE_FILLING,
    LINE_DRAINING
};

struct _line_buf;
typedef struct _line_buf *LineBuf;

LineBuf line_buf_alloc();
void line_buf_free(LineBuf buf);

enum line_read_state line_get_read_state(LineBuf buf);

int line_buf_put_char(LineBuf buf, int c);

int line_buf_get_line(LineBuf buf, char *out, int n);

void line_buf_reset(LineBuf buf);
