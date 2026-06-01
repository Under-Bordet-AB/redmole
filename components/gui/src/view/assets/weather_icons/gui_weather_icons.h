#ifndef GUI_WEATHER_ICONS_H
#define GUI_WEATHER_ICONS_H

#include "gui_types.h"
#include "lvgl.h"

LV_FONT_DECLARE(weather_icons_128);

#define GUI_WEATHER_ICON_SYMBOL_PARTLY_CLOUDY "\xEF\x80\x82" /* wi-day-cloudy */
#define GUI_WEATHER_ICON_SYMBOL_CLEAR         "\xEF\x80\x8D" /* wi-day-sunny */
#define GUI_WEATHER_ICON_SYMBOL_CLOUDY        "\xEF\x80\x93" /* wi-cloudy */
#define GUI_WEATHER_ICON_SYMBOL_FOG           "\xEF\x80\x94" /* wi-fog */
#define GUI_WEATHER_ICON_SYMBOL_RAIN          "\xEF\x80\x99" /* wi-rain */
#define GUI_WEATHER_ICON_SYMBOL_SNOW          "\xEF\x80\x9B" /* wi-snow */
#define GUI_WEATHER_ICON_SYMBOL_DRIZZLE       "\xEF\x80\x9C" /* wi-sprinkle */
#define GUI_WEATHER_ICON_SYMBOL_THUNDERSTORM  "\xEF\x80\x9E" /* wi-thunderstorm */

const char *gui_weather_icons_get_symbol(gui_weather_icon_t icon);

#endif
