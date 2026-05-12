#ifndef SDCARD_H
#define SDCARD_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SD card (SPI mode)
 */
esp_err_t sdcard_init(void);

/**
 * Unmount and release SD card
 */
void sdcard_dispose(void);

/**
 * Write text data to a file
 * @param path Path relative to mount point (e.g. "/sdcard/file.txt")
 */
esp_err_t sdcard_write_file(const char *path, const char *data);

/**
 * Append text data to a file (creates file if it doesn't exist)
 * @param path Path to file
 */
esp_err_t sdcard_append_file(const char *path, const char *data);

/**
 * Read entire file into buffer
 * @param path Path to file
 * @param out_buffer Caller-provided buffer
 * @param max_len Buffer size
 */
esp_err_t sdcard_read_file(const char *path, char *out_buffer, size_t max_len);

/**
 * Check if SD card is mounted
 */
bool sdcard_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // SDCARD_H