/* The UART client sends a command and receives a package as a response from the ESP32S3.
 * Will implement the packages to return as a factory. The client only receives a handle back.
 */

#ifndef UART_CLIENT_H
#define UART_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include "protocol.h"

// Handle to a UART package — points at the package_api field inside uart_base_t so that
// container_of can recover the full object from any vtable call.
struct uart_api;
typedef struct uart_api** uart_package_t;

struct uart_api
{
    int8_t (*read_package)(uart_package_t self);
    int8_t (*write_package)(uart_package_t self, uint8_t *data, size_t len);
};

// Embed at the top of every concrete package type; &base->package_api is the handle.
typedef struct uart_base
{
    struct uart_api *package_api;
} uart_base_t;

typedef struct
{
    int fd;
} uart_client_t;

int  uart_client_init(uart_client_t *self);
void uart_client_work(uart_client_t *self);
void uart_client_deinit(uart_client_t *self);

uart_package_t uart_client_create_package(uart_pkg_tag_t tag);
void           uart_client_destroy_package(uart_package_t pkg);
int8_t         uart_factory_read_package(uart_package_t pkg);
int8_t         uart_factory_write_package(uart_package_t pkg, uint8_t *data, size_t len);

#endif /* UART_CLIENT_H */
