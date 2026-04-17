#ifndef MEM_ALLOC_MODULE_H
#define MEM_ALLOC_MODULE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    SemaphoreHandle_t alloc_mutex; // Distributed Mutex per Sec 4
    size_t psram_usage;
    size_t sram_usage;
} mem_alloc_ctx;

void mem_alloc_init(mem_alloc_ctx *self);
void mem_alloc_run(mem_alloc_ctx *self); // Could monitor and log heap health
void mem_alloc_deinit(mem_alloc_ctx *self);

// Custom allocator wrappers
void* mem_alloc_malloc(mem_alloc_ctx *self, size_t size, bool prefer_psram);
void mem_alloc_free(mem_alloc_ctx *self, void *ptr);

#endif // MEM_ALLOC_MODULE_H
