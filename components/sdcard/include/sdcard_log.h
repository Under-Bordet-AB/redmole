#ifndef SDCARD_LOG_H
#define SDCARD_LOG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize logging system
 */
esp_err_t sdcard_log_init(const char *dir_path);

/**
 * Flush or close logging
 */
void sdcard_log_dispose(void);

#ifdef __cplusplus
}
#endif

#endif // SDCARD_LOG_H