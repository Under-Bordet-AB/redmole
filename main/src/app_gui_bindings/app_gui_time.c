#include "app_gui_bindings_internal.h"

#include <stdio.h>
#include <time.h>

void app_gui_time_format_unknown_last_updated(char *text, size_t text_len)
{
    if ((text == NULL) || (text_len == 0U)) {
        return;
    }

    snprintf(text, text_len, "%s", "Last updated: --:--:--");
}

void app_gui_time_format_last_updated_now(char *text, size_t text_len)
{
    time_t now;
    struct tm time_info;

    if ((text == NULL) || (text_len == 0U)) {
        return;
    }

    now = time(NULL);
    if ((now < 1700000000) || (localtime_r(&now, &time_info) == NULL) ||
        (strftime(text, text_len, "Last updated: %H:%M:%S", &time_info) == 0U)) {
        app_gui_time_format_unknown_last_updated(text, text_len);
    }
}
