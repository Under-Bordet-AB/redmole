#ifndef SYSTEM_SERVICES_MODULE_H
#define SYSTEM_SERVICES_MODULE_H

#include <stdint.h>

typedef struct {
    uint32_t heartbeat_bitmask; // Stores task check-ins
    int last_reset_reason;
} system_services_ctx;

void system_services_init(system_services_ctx *self);
void system_services_run(system_services_ctx *self); // Periodically evaluates health bits and FSM states
void system_services_deinit(system_services_ctx *self);

#endif // SYSTEM_SERVICES_MODULE_H
