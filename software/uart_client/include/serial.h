/**
 * @file
 * @brief Thin POSIX serial port wrapper used by the uart_client.
 *
 * Provides blocking open/close, exact-length read, and exact-length write
 * operations over a serial file descriptor.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

#define RECV_TIMEOUT_MS    2000  /*!< Default receive timeout in milliseconds. */
#define MAX_PACKET_PAYLOAD 12288 /*!< Maximum deframed payload size in bytes. */

/**
 * @brief Open and configure a serial port for use with the uart_client protocol.
 *
 * Configures the port for the baud rate and framing expected by the ESP32
 * firmware (8N1, no flow control).
 *
 * @param path Null-terminated device path (e.g. "/dev/ttyUSB0"); must not be NULL.
 * @return A valid file descriptor on success, -1 on error with errno set.
 */
int serial_open(const char *path);

/**
 * @brief Close a serial port file descriptor.
 *
 * @param fd File descriptor returned by serial_open().
 */
void serial_close(int fd);

/**
 * @brief Read exactly len bytes from the serial port, blocking up to timeout_ms.
 *
 * Returns early with -1 if the timeout elapses before all bytes arrive.
 *
 * @param fd         File descriptor returned by serial_open().
 * @param buf        Output buffer; must not be NULL and must be at least len bytes.
 * @param len        Number of bytes to read.
 * @param timeout_ms Maximum time to wait in milliseconds before returning -1.
 * @return 0 on success, -1 on timeout or read error.
 */
int serial_read_exact(int fd, void *buf, size_t len, int timeout_ms);

/**
 * @brief Write exactly len bytes to the serial port.
 *
 * Retries partial writes until all bytes are sent or a write error occurs.
 *
 * @param fd  File descriptor returned by serial_open().
 * @param buf Data to write; must not be NULL.
 * @param len Number of bytes to write.
 * @return 0 on success, -1 on write error.
 */
int serial_write_exact(int fd, const void *buf, size_t len);

#endif /* SERIAL_H */
