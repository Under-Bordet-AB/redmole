#ifndef APP_MODULE_H
#define APP_MODULE_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// Bring in the component objects (Composition)
/*
#include "mem_alloc_module.h"
#include "uart_diag_module.h"
#include "nvs_module.h"
#include "bme280_module.h"
#include "server_module.h"
#include "gui_module.h"
#include "health_module.h"
*/
#include "wifi_module.h"

// Global Inter-task Signaling Bits (Sec 3.2)
#define BIT_WIFI_CONNECTED   (1 << 0)
#define BIT_SENSOR_READY     (1 << 1)
#define BIT_SYSTEM_FAULT     (1 << 2)

typedef struct {
    EventGroupHandle_t group; // Central signaling mechanism
} app_signals_t;

// The complex object (Mediator)
typedef struct {
    //app_signals_t   signals;
    //mem_alloc_ctx_t mem;
    //uart_diag_ctx_t diag;
    //nvs_ctx_t       nvs;
    //bme280_ctx_t    sensor;
    wifi_ctx_t      wifi;
    //server_ctx_t    server;
    //gui_ctx_t       gui;
    //health_ctx_t    health;
} app_ctx_t;

// Lifecycle for the entire mediator
app_ctx_t *app_init(void);
//void app_run(app_ctx_t *self); // The conductor/dispatcher loop
void app_deinit(app_ctx_t *self);

/*
void app_task_wifi(void *pvParameters);
void app_task_sensor(void *pvParameters);
void app_task_server(void *pvParameters);
void app_task_gui(void *pvParameters);
void app_task_health(void *pvParameters);
*/

#endif // APP_MODULE_H
