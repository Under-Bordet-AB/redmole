/**
 * @file gui_view_common.h
 * @brief Shared LVGL helper functions used by the GUI view implementation.
 */

#ifndef GUI_VIEW_COMMON_H
#define GUI_VIEW_COMMON_H

#include "lvgl.h"

#include "../gui_defs.h"
#include "gui_view.h"

/**
 * @brief Update a label only when its text differs from the current value.
 *
 * @param label Target LVGL label object.
 * @param text Null-terminated text to apply.
 */
void gui_view_set_label_text_if_changed(lv_obj_t *label, const char *text);

/**
 * @brief Update a text area only when its text differs from the current value.
 *
 * @param textarea Target LVGL text area object.
 * @param text Null-terminated text to apply.
 */
void gui_view_set_textarea_text_if_changed(lv_obj_t *textarea, const char *text);

/**
 * @brief Apply theme-aware styling to a scanned Wi-Fi network button.
 *
 * @param button Target button object.
 * @param theme Theme whose colors should be used.
 * @param is_selected True when the network is the currently selected entry.
 * @param is_known True when the network also exists in the known network list.
 * @param is_connected True when the network is currently connected.
 */
void gui_view_style_scanned_wifi_button(lv_obj_t *button, gui_view_theme_t theme,
                                        bool is_selected, bool is_known, bool is_connected);

/**
 * @brief Determine whether a given SSID is the currently connected Wi-Fi network.
 *
 * @param model View model to inspect.
 * @param ssid Null-terminated SSID string to compare.
 * @return True when the model represents the SSID as connected.
 */
bool gui_view_wifi_is_connected(const gui_view_model_t *model, const char *ssid);

/**
 * @brief Find the index of a known Wi-Fi network by SSID.
 *
 * @param model View model to inspect.
 * @param ssid Null-terminated SSID string to search for.
 * @return Matching known network index, or -1 when no match exists.
 */
int8_t gui_view_find_known_network_index(const gui_view_model_t *model, const char *ssid);

/**
 * @brief Compute the absolute value of a signed 32-bit integer.
 *
 * @param value Signed integer value.
 * @return Absolute value of value.
 */
int32_t gui_view_abs_i32(int32_t value);

/**
 * @brief Create a themed action button at a fixed position.
 *
 * @param parent Parent LVGL object.
 * @param x X coordinate relative to parent.
 * @param y Y coordinate relative to parent.
 * @param width Button width.
 * @param text Null-terminated button label.
 * @param event_code LVGL event code to react to.
 * @param event_cb Callback bound to the button.
 * @param event_user_data User data forwarded to the callback.
 * @return Newly created LVGL button object.
 */
lv_obj_t *gui_view_create_action_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                        lv_coord_t width, const char *text,
                                        lv_event_code_t event_code, lv_event_cb_t event_cb,
                                        void *event_user_data);

/**
 * @brief Create a chart legend item composed of a colored dot and text label.
 *
 * @param parent Parent LVGL object.
 * @param x X coordinate relative to parent.
 * @param y Y coordinate relative to parent.
 * @param width Legend item width.
 * @param color Legend marker color.
 * @param text Null-terminated legend label.
 * @return Newly created LVGL container object for the legend item.
 */
lv_obj_t *gui_view_create_legend_item(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      lv_coord_t width, lv_color_t color,
                                      const char *text);

/**
 * @brief Create a metric card with a static label and caller-owned value label pointer.
 *
 * @param parent Parent LVGL object.
 * @param x X coordinate relative to parent.
 * @param y Y coordinate relative to parent.
 * @param label_text Null-terminated metric title.
 * @param value_label Output pointer receiving the created value label.
 * @return Newly created LVGL card container.
 */
lv_obj_t *gui_view_create_metric_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                      const char *label_text, lv_obj_t **value_label);

/**
 * @brief Copy an energy series into an LVGL chart series.
 *
 * @param chart Target chart widget.
 * @param series Chart series to update.
 * @param values Array of GUI_ENERGY_PLAN_POINT_COUNT values.
 */
void gui_view_apply_energy_series(lv_obj_t *chart, lv_chart_series_t *series,
                                  const uint16_t *values);

#endif