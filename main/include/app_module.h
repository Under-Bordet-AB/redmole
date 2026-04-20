#ifndef APP_MODULE_H
#define APP_MODULE_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nac.h"

/*
 * Future components (uncomment as modules are added):
 * #include "mem_alloc_module.h"
 * #include "uart_diag_module.h"
 * #include "nvs_module.h"
 * #include "bme280_module.h"
 * #include "server_module.h"
 * #include "gui_module.h"
 * #include "health_module.h"
 */

/* Global inter-task signalling bits */
#define BIT_WIFI_CONNECTED  (1 << 0)
#define BIT_SENSOR_READY    (1 << 1)
#define BIT_SYSTEM_FAULT    (1 << 2)

/*
 * app_ctx_t — the mediator / top-level owner of all subsystems.
 *
 * nac is a pointer because nac_init() allocates it.  Every other
 * module that allocates should follow the same pattern.
 */
typedef struct
{
    nac_ctx_t *nac;
    /*
     * mem_alloc_ctx_t *mem;
     * uart_diag_ctx_t *diag;
     * nvs_ctx_t       *nvs;
     * bme280_ctx_t    *sensor;
     * server_ctx_t    *server;
     * gui_ctx_t       *gui;
     * health_ctx_t    *health;
     */
} app_ctx_t;

app_ctx_t *app_init(void);
void       app_deinit(app_ctx_t *self);

#endif /* APP_MODULE_H */
