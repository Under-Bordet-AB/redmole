#ifndef SDCARD_LOG_H
#define SDCARD_LOG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize logging system (optionally creates log file)
 */
esp_err_t sdcard_log_init(const char *log_path);

/**
 * Append a log line (adds newline automatically)
 */
esp_err_t sdcard_log_write(const char *message);

/**
 * Flush or close logging (optional depending on design)
 */
void sdcard_log_dispose(void);

#ifdef __cplusplus
}
#endif

#endif // SDCARD_LOG_H