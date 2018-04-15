/*
 * gui_layout.h
 *
 *  Created on: Jan 20, 2018
 *      Author: chris
 */

#ifndef GUI_LAYOUT_H_
#define GUI_LAYOUT_H_

//
// STATUS DISPLAY
//

#define LIGHTING_ICON_POS            (struct bitmap_placement) {0, 56, 128, 32}

#define OUTSIDE_VALUE_COL 2
#define OUTSIDE_HEADER_COL OUTSIDE_VALUE_COL
#define OUTSIDE_LABEL_COL (OUTSIDE_VALUE_COL + 4)

#define INSIDE_VALUE_COL 12
#define INSIDE_HEADER_COL INSIDE_VALUE_COL
#define INSIDE_LABEL_COL (INSIDE_VALUE_COL + 4)

#define HEADER_ROW 7
#define TEMPERATURE_ROW (HEADER_ROW + 2)
#define HUMIDITY_ROW (TEMPERATURE_ROW + 2)
#define BAROMETER_ROW (HUMIDITY_ROW + 3)

#define TIME_TEXT_POS                (struct text_placement) {1, 0, 2}
#define DATE_TEXT_POS                (struct text_placement) {15, 0, 1}

#define OUTSIDE_HEADER_TEXT_POS      (struct text_placement) {OUTSIDE_HEADER_COL, HEADER_ROW, 1}
#define INSIDE_HEADER_TEXT_POS       (struct text_placement) {INSIDE_HEADER_COL, HEADER_ROW, 1}

#define OUTSIDE_TEMPERATURE_LABEL_POS (struct text_placement) {OUTSIDE_LABEL_COL, TEMPERATURE_ROW, 1}
#define OUTSIDE_TEMPERATURE_VALUE_POS (struct text_placement) {OUTSIDE_VALUE_COL, TEMPERATURE_ROW, 2}
#define OUTSIDE_TEMPERATURE_TREND_POS (struct text_placement) {OUTSIDE_LABEL_COL + 2, TEMPERATURE_ROW, 2}

#define INSIDE_TEMPERATURE_LABEL_POS  (struct text_placement) {INSIDE_LABEL_COL, TEMPERATURE_ROW, 1}
#define INSIDE_TEMPERATURE_VALUE_POS  (struct text_placement) {INSIDE_VALUE_COL, TEMPERATURE_ROW, 2}
#define INSIDE_TEMPERATURE_TREND_POS  (struct text_placement) {INSIDE_LABEL_COL + 2, TEMPERATURE_ROW, 2}

#define OUTSIDE_HUMIDITY_LABEL_POS    (struct text_placement) {OUTSIDE_LABEL_COL, HUMIDITY_ROW, 1}
#define OUTSIDE_HUMIDITY_VALUE_POS    (struct text_placement) {OUTSIDE_VALUE_COL, HUMIDITY_ROW, 2}
#define OUTSIDE_HUMIDITY_TREND_POS    (struct text_placement) {OUTSIDE_LABEL_COL + 2, HUMIDITY_ROW, 2}

#define INSIDE_HUMIDITY_LABEL_POS     (struct text_placement) {INSIDE_LABEL_COL, HUMIDITY_ROW, 1}
#define INSIDE_HUMIDITY_VALUE_POS     (struct text_placement) {INSIDE_VALUE_COL, HUMIDITY_ROW, 2}
#define INSIDE_HUMIDITY_TREND_POS     (struct text_placement) {INSIDE_LABEL_COL + 2, HUMIDITY_ROW, 2}

#define BAROMETER_LABEL_POS           (struct text_placement) {16, BAROMETER_ROW, 1}
#define BAROMETER_VALUE_POS           (struct text_placement) {5, BAROMETER_ROW, 2}
#define BAROMETER_TREND_POS           (struct text_placement) {2, BAROMETER_ROW, 2}


#endif /* GUI_LAYOUT_H_ */
