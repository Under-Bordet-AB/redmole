#ifndef GUI_WEATHER_ICONS_H
#define GUI_WEATHER_ICONS_H

/**
 * @file gui_weather_icons.h
 * @brief Weather icon font declarations and symbol lookup helpers.
 */

#include "gui_types.h"
#include "lvgl.h"

/** Weather Icons font used by the forecast panel. */
LV_FONT_DECLARE(weather_icons_128);

/** Partly cloudy Weather Icons symbol, wi-day-cloudy. */
#define GUI_WEATHER_ICON_SYMBOL_PARTLY_CLOUDY "\xEF\x80\x82"
/** Clear Weather Icons symbol, wi-day-sunny. */
#define GUI_WEATHER_ICON_SYMBOL_CLEAR         "\xEF\x80\x8D"
/** Cloudy Weather Icons symbol, wi-cloudy. */
#define GUI_WEATHER_ICON_SYMBOL_CLOUDY        "\xEF\x80\x93"
/** Fog Weather Icons symbol, wi-fog. */
#define GUI_WEATHER_ICON_SYMBOL_FOG           "\xEF\x80\x94"
/** Rain Weather Icons symbol, wi-rain. */
#define GUI_WEATHER_ICON_SYMBOL_RAIN          "\xEF\x80\x99"
/** Snow Weather Icons symbol, wi-snow. */
#define GUI_WEATHER_ICON_SYMBOL_SNOW          "\xEF\x80\x9B"
/** Drizzle Weather Icons symbol, wi-sprinkle. */
#define GUI_WEATHER_ICON_SYMBOL_DRIZZLE       "\xEF\x80\x9C"
/** Thunderstorm Weather Icons symbol, wi-thunderstorm. */
#define GUI_WEATHER_ICON_SYMBOL_THUNDERSTORM  "\xEF\x80\x9E"

/**
 * @brief Return the LVGL text symbol for a weather icon identifier.
 *
 * @param icon Weather icon identifier to resolve.
 * @return Static null-terminated symbol string; cloudy is used as the fallback.
 */
const char *gui_weather_icons_get_symbol(gui_weather_icon_t icon);

#endif
