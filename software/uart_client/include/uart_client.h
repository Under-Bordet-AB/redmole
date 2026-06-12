/**
 * @file
 * @brief UART client factory interface for sending commands and receiving typed packets.
 *
 * Implements a vtable-based packet factory. Each packet type is a concrete
 * struct with uart_base_t embedded first. uart_client_create_package()
 * allocates the concrete type and returns a uart_package_t handle pointing at
 * its vtable pointer. Callers interact through uart_factory_read_package() and
 * uart_factory_write_package() without knowing the concrete type.
 */

#ifndef UART_CLIENT_H
#define UART_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include "protocol.h"

/**
 * @brief Opaque handle to a UART packet vtable.
 *
 * Points at the package_api field of the embedded uart_base_t. Use
 * container_of to recover the full concrete struct from inside a vtable method.
 */
struct uart_api;
typedef struct uart_api** uart_package_t;

/**
 * @brief Virtual dispatch table shared by all packet types.
 */
struct uart_api
{
    int8_t (*read_package)(uart_package_t self);                              /*!< Receive and decode a packet of this type. */
    int8_t (*write_package)(uart_package_t self, uint8_t *data, size_t len); /*!< Encode and send a packet of this type. */
};

/**
 * @brief Base struct embedded at the top of every concrete packet type.
 *
 * Taking &base->package_api yields a uart_package_t usable with the factory API.
 */
typedef struct uart_base
{
    struct uart_api *package_api; /*!< Pointer to the vtable for this packet type. */
} uart_base_t;

/**
 * @brief UART client instance holding the open serial file descriptor.
 */
typedef struct
{
    int fd; /*!< Serial file descriptor opened by uart_client_init(). */
} uart_client_t;

/**
 * @brief Open the serial port and prepare the client for use.
 *
 * Must be called before uart_client_work() or uart_client_deinit().
 *
 * @param self Client instance; must not be NULL.
 * @return 0 on success, -1 on failure.
 */
int uart_client_init(uart_client_t *self);

/**
 * @brief Run one iteration of the client request/response loop.
 *
 * Sends the next queued command, receives the response packet, and dispatches
 * it to the appropriate protocol_decode_* function.
 *
 * @param self Initialised client instance; must not be NULL.
 */
void uart_client_work(uart_client_t *self);

/**
 * @brief Close the serial port and release all resources.
 *
 * @param self Client instance; must not be NULL.
 */
void uart_client_deinit(uart_client_t *self);

/**
 * @brief Allocate and return a packet handle for the given tag type.
 *
 * The returned handle must be freed with uart_client_destroy_package().
 *
 * @param tag Packet type to create.
 * @return A valid uart_package_t on success, NULL on allocation failure.
 */
uart_package_t uart_client_create_package(uart_pkg_tag_t tag);

/**
 * @brief Free a packet handle previously returned by uart_client_create_package().
 *
 * @param pkg Handle to free; NULL is a no-op.
 */
void uart_client_destroy_package(uart_package_t pkg);

/**
 * @brief Receive and decode one packet via the vtable for pkg.
 *
 * @param pkg Valid packet handle; must not be NULL.
 * @return 0 on success, -1 on protocol or read error.
 */
int8_t uart_factory_read_package(uart_package_t pkg);

/**
 * @brief Encode and send data via the vtable for pkg.
 *
 * @param pkg  Valid packet handle; must not be NULL.
 * @param data Payload bytes to send; must not be NULL.
 * @param len  Number of bytes in data.
 * @return 0 on success, -1 on write error.
 */
int8_t uart_factory_write_package(uart_package_t pkg, uint8_t *data, size_t len);

#endif /* UART_CLIENT_H */
