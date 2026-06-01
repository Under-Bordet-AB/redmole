/**
 * @file app_gui_bindings.c
 * @brief Synchronize application services, persisted settings, and GUI events.
 *
 * This module owns the public GUI binding API and delegates private
 * persistence, Wi-Fi, sync, and scheduled fetch work to focused helpers.
 */

#include "app_gui_bindings.h"

#include <string.h>

#include "app_gui_bindings/app_gui_bindings_internal.h"
#include "freertos/event_groups.h"
#include "uart_mole.h"

static app_gui_bindings_ctx_t s_bindings;

bool app_gui_bindings_load_saved_appearance(gui_init_config_t *config)
{
    return app_gui_settings_load_saved_appearance(config);
}

esp_err_t app_gui_bindings_init(gui_ctx_t *gui, EventGroupHandle_t *event_group)
{
    gui_module_bindings_t bindings = { 0 };

    if ((gui == NULL) || (event_group == NULL) || (*event_group == NULL)) {
        if ((event_group != NULL) && (*event_group != NULL)) {
            xEventGroupClearBits(*event_group, UART_MOLE_GUI_ONLINE_BIT);
        }
        return ESP_ERR_INVALID_ARG;
    }

    memset(&s_bindings, 0, sizeof(s_bindings));
    s_bindings.gui = gui;
    s_bindings.event_group = event_group;

    app_gui_wifi_fill_bindings(&bindings, &s_bindings);
    gui_set_bindings(gui, &bindings);

    app_gui_settings_cache_current_appearance(&s_bindings, gui);
    app_gui_settings_cache_current_location(&s_bindings, gui);
    (void)app_gui_settings_load_saved_location(&s_bindings, gui);
    (void)app_gui_wifi_load_saved_metadata(&s_bindings, gui);
    app_gui_bindings_sync(gui);
    (void)app_gui_wifi_queue_saved_autoconnect(&s_bindings, gui);

    app_gui_forecast_register_task(&s_bindings);
    app_gui_leop_register_task(&s_bindings);
    app_gui_sync_register_sensor_task(&s_bindings);

    return ESP_OK;
}

void app_gui_bindings_sync(gui_ctx_t *gui)
{
    app_gui_sync_runtime(&s_bindings, gui);

    if (gui == NULL) {
        return;
    }

    (void)app_gui_settings_save_appearance_if_changed(&s_bindings, gui);
    (void)app_gui_settings_save_location_if_changed(&s_bindings, gui);
}
