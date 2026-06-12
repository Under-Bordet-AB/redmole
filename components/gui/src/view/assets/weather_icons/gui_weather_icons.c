#include "gui_weather_icons.h"

const char *gui_weather_icons_get_symbol(gui_weather_icon_t icon)
{
    switch (icon) {
        case GUI_WEATHER_ICON_CLEAR:
            return GUI_WEATHER_ICON_SYMBOL_CLEAR;
        case GUI_WEATHER_ICON_PARTLY_CLOUDY:
            return GUI_WEATHER_ICON_SYMBOL_PARTLY_CLOUDY;
        case GUI_WEATHER_ICON_CLOUDY:
            return GUI_WEATHER_ICON_SYMBOL_CLOUDY;
        case GUI_WEATHER_ICON_FOG:
            return GUI_WEATHER_ICON_SYMBOL_FOG;
        case GUI_WEATHER_ICON_DRIZZLE:
            return GUI_WEATHER_ICON_SYMBOL_DRIZZLE;
        case GUI_WEATHER_ICON_RAIN:
            return GUI_WEATHER_ICON_SYMBOL_RAIN;
        case GUI_WEATHER_ICON_SNOW:
            return GUI_WEATHER_ICON_SYMBOL_SNOW;
        case GUI_WEATHER_ICON_THUNDERSTORM:
            return GUI_WEATHER_ICON_SYMBOL_THUNDERSTORM;
        default:
            return GUI_WEATHER_ICON_SYMBOL_CLOUDY;
    }
}
