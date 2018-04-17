#include "weather_station_ui.h"
#include "gui_layout.h"
#include "embedded_gui.h"

static struct weather_station_status short_run_avg = {0};
static float const short_run_alpha = 2/(SHORT_RUN_AVG_N_SAMPLES + 1.0);

static struct weather_station_status long_run_avg = {0};
static float const long_run_alpha = 2/(LONG_RUN_AVG_N_SAMPLES + 1.0);


void draw_trend(struct text_placement location, float new, float *short_run_avg, float *long_run_avg, float eps)
{
    if(new == 0) {return;}
    if(*short_run_avg == 0) {*short_run_avg = new;}
    if(*long_run_avg == 0) {*long_run_avg = new;}

    *short_run_avg = *short_run_avg * (1 - short_run_alpha) + new * short_run_alpha;
    *long_run_avg = *long_run_avg * (1 - long_run_alpha) + new * long_run_alpha;

    if(*short_run_avg - *long_run_avg > eps)
    {
        gui_print(location, "\x1E");
    }
    else if(*short_run_avg - *long_run_avg < -eps)
    {
        gui_print(location, "\x1F");
    }
    else
    {
        gui_print(location, " ");
    }
}

int draw_weather_station_ui(struct weather_station_status status)
{
    //gui_draw(LIGHTING_ICON_POS, images[status.lighting]);
    if(show_colon)
    {
        GUI_PRINT_FORMATTED(TIME_TEXT_POS, "%02d:%02d", status.time.hour, status.time.min);
    }
    else
    {
        GUI_PRINT_FORMATTED(TIME_TEXT_POS, "%02d %02d", status.time.hour, status.time.min);
    }
    GUI_PRINT_FORMATTED(DATE_TEXT_POS, "%02d/%02d/%04d", status.time.month, status.time.date, status.time.year);

    gui_print(LOCAL_CONDITION_TEXT_POS, status.local_condition);
    gui_print(OUTSIDE_HEADER_TEXT_POS, "OUTSIDE");
    gui_print(INSIDE_HEADER_TEXT_POS, "INSIDE");

    GUI_PRINT_FORMATTED(OUTSIDE_TEMPERATURE_VALUE_POS, "%2.f", status.outdoor_temperature);
    gui_print(OUTSIDE_TEMPERATURE_LABEL_POS, "\xF7""F");
    draw_trend(OUTSIDE_TEMPERATURE_TREND_POS, status.outdoor_temperature, &short_run_avg.outdoor_temperature, &long_run_avg.outdoor_temperature, 0.5f);

    GUI_PRINT_FORMATTED(INSIDE_TEMPERATURE_VALUE_POS, "%2.f", status.indoor_temperature);
    gui_print(INSIDE_TEMPERATURE_LABEL_POS, "\xF7""F");
    draw_trend(INSIDE_TEMPERATURE_TREND_POS, status.indoor_temperature, &short_run_avg.indoor_temperature, &long_run_avg.indoor_temperature, 0.5f);

    GUI_PRINT_FORMATTED(OUTSIDE_HUMIDITY_VALUE_POS, "%2.f", status.outdoor_humidity);
    gui_print(OUTSIDE_HUMIDITY_LABEL_POS, "%");
    draw_trend(OUTSIDE_HUMIDITY_TREND_POS, status.outdoor_humidity, &short_run_avg.outdoor_humidity, &long_run_avg.outdoor_humidity, 0.5f);

    GUI_PRINT_FORMATTED(INSIDE_HUMIDITY_VALUE_POS, "%2.f", status.indoor_humidity);
    gui_print(INSIDE_HUMIDITY_LABEL_POS, "%");
    draw_trend(INSIDE_HUMIDITY_TREND_POS, status.indoor_humidity, &short_run_avg.indoor_humidity, &long_run_avg.indoor_humidity, 0.5f);

    GUI_PRINT_FORMATTED(BAROMETER_VALUE_POS, "%2.2f", status.pressure);
    gui_print(BAROMETER_LABEL_POS, "inHg");
    draw_trend(BAROMETER_TREND_POS, status.pressure, &short_run_avg.pressure, &long_run_avg.pressure, 0.01f);

    return 0;
}
