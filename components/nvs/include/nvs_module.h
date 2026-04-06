#ifndef NVS_MODULE_H
#define NVS_MODULE_H

#include <stdbool.h>

typedef struct {
    bool is_initialized;
    // NVS handle would go here
} nvs_ctx_t;

void nvs_init(nvs_ctx_t *self);
void nvs_run(nvs_ctx_t *self); // Flushes deferred writes or handles background wear-leveling
void nvs_deinit(nvs_ctx_t *self);

#endif // NVS_MODULE_H
