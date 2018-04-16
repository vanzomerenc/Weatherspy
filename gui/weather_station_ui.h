/*
 * info_ui.h
 *
 *  Created on: Jan 20, 2018
 *      Author: chris
 */

#ifndef GUI_WEATHER_STATION_UI_H_
#define GUI_WEATHER_STATION_UI_H_

#include "../weather_station_status.h"

#define DEMO_MODE

#if defined(DEMO_MODE)
# define SHORT_RUN_AVG_N_SAMPLES 15
# define LONG_RUN_AVG_N_SAMPLES 30
#else
# define SHORT_RUN_AVG_N_SAMPLES 3600
# define LONG_RUN_AVG_N_SAMPLES 7200
#endif

int show_colon;

int draw_weather_station_ui(struct weather_station_status status);

#endif /* GUI_WEATHER_STATION_UI_H_ */
