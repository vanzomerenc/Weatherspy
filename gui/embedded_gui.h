/*
 * embedded_gui.h
 *
 *  Created on: Jan 16, 2018
 *      Author: chris
 */

#ifndef EMBEDDED_GUI_H_
#define EMBEDDED_GUI_H_

#include <stdio.h>

#include "../drv/ST7735/ST7735.h"

struct text_placement
{
    int column;
    int line;
    int scale;
};

struct bitmap_placement
{
    int x;
    int y;
    int w;
    int h;
};

inline void gui_draw(
    struct bitmap_placement bitmap_placement,
    uint16_t const *bitmap_data)
{
    ST7735_DrawBitmap(
            bitmap_placement.x,
            bitmap_placement.y,
            bitmap_data,
            bitmap_placement.w,
            bitmap_placement.h);
}

inline void gui_print(
    struct text_placement text_placement,
    char const *text)
{
    ST7735_DrawStringS(
            text_placement.column,
            text_placement.line,
            text,
            ST7735_WHITE,
            ST7735_BLACK,
            text_placement.scale);
}

inline void gui_print_selected(
    struct text_placement text_placement,
    char const* text)
{
    ST7735_DrawStringS(
            text_placement.column,
            text_placement.line,
            text,
            ST7735_BLACK,
            ST7735_WHITE,
            text_placement.scale);
}

#define MAX_TEXT_LENGTH 64

#define GUI_PRINT_FORMATTED(PLACEMENT, TEXT, ...)\
    do {\
        char _formatted_text[MAX_TEXT_LENGTH];\
        snprintf(_formatted_text, MAX_TEXT_LENGTH, TEXT, __VA_ARGS__);\
        gui_print(PLACEMENT, _formatted_text);\
    } while(0)

#endif /* EMBEDDED_GUI_H_ */
