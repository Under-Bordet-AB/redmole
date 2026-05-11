#include "uart_client.h"

#include <glob.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"
#include "serial.h"

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

// --- package structs ---------------------------------------------------------

typedef struct
{
    uart_base_t base;
    // TODO: add STATUS fields once the wire layout is defined
} uart_client_status_pkg_t;

typedef struct
{
    uart_base_t base;
    // TODO: add SERVER fields once the wire layout is defined
} uart_client_server_pkg_t;

typedef struct
{
    uart_base_t base;
    int32_t  temperature;   // temperature_x_100 / 100.0 = degrees C
    int32_t  humidity;      // humidity_x_100    / 100.0 = %RH
    int32_t  pressure;      // pressure_x_100    / 100.0 = hPa
    uint32_t timestamp;     // seconds since epoch
} uart_client_sensor_pkg_t;

typedef struct
{
    uart_base_t base;
    // TODO: add CONFIG fields once the wire layout is defined
} uart_client_config_pkg_t;

typedef struct
{
    uart_base_t base;
    // TODO: add DIAG fields once the wire layout is defined
} uart_client_diag_pkg_t;

// --- forward declarations ----------------------------------------------------

static int8_t uart_client_status_pkg_read (uart_package_t self);
static int8_t uart_client_status_pkg_write(uart_package_t self, uint8_t *data, size_t len);
static int8_t uart_client_server_pkg_read (uart_package_t self);
static int8_t uart_client_server_pkg_write(uart_package_t self, uint8_t *data, size_t len);
static int8_t uart_client_sensor_pkg_read (uart_package_t self);
static int8_t uart_client_sensor_pkg_write(uart_package_t self, uint8_t *data, size_t len);
static int8_t uart_client_config_pkg_read (uart_package_t self);
static int8_t uart_client_config_pkg_write(uart_package_t self, uint8_t *data, size_t len);
static int8_t uart_client_diag_pkg_read   (uart_package_t self);
static int8_t uart_client_diag_pkg_write  (uart_package_t self, uint8_t *data, size_t len);

// --- vtables -----------------------------------------------------------------

static const struct uart_api s_status_api =
{
    .read_package  = uart_client_status_pkg_read,
    .write_package = uart_client_status_pkg_write,
};

static const struct uart_api s_server_api =
{
    .read_package  = uart_client_server_pkg_read,
    .write_package = uart_client_server_pkg_write,
};

static const struct uart_api s_sensor_api =
{
    .read_package  = uart_client_sensor_pkg_read,
    .write_package = uart_client_sensor_pkg_write,
};

static const struct uart_api s_config_api =
{
    .read_package  = uart_client_config_pkg_read,
    .write_package = uart_client_config_pkg_write,
};

static const struct uart_api s_diag_api =
{
    .read_package  = uart_client_diag_pkg_read,
    .write_package = uart_client_diag_pkg_write,
};

// --- STATUS ------------------------------------------------------------------

static void uart_client_status_pkg_init(uart_client_status_pkg_t *self)
{
    *self = (uart_client_status_pkg_t)
    {
        .base =
        {
            .package_api = &s_status_api,
        },
        // TODO: initialise STATUS-specific fields once wire layout is defined
    };
}

static int8_t uart_client_status_pkg_read(uart_package_t self)
{
    uart_client_status_pkg_t *pkg = CONTAINER_OF(self, uart_client_status_pkg_t, base.package_api);
    // TODO: call protocol_recv_packet to receive the response into a local payload buffer
    // TODO: decode each field byte-by-byte from the payload (little-endian, no pointer casts)
    // TODO: store decoded values into pkg fields
    // TODO: return 0 on success, -1 on error
    (void)pkg;
    return 0;
}

static int8_t uart_client_status_pkg_write(uart_package_t self, uint8_t *data, size_t len)
{
    uart_client_status_pkg_t *pkg = CONTAINER_OF(self, uart_client_status_pkg_t, base.package_api);
    // TODO: serialise pkg fields into data byte-by-byte (little-endian)
    // TODO: return 0 on success, -1 on error
    (void)pkg; (void)data; (void)len;
    return 0;
}

// --- SERVER ------------------------------------------------------------------

static void uart_client_server_pkg_init(uart_client_server_pkg_t *self)
{
    *self = (uart_client_server_pkg_t)
    {
        .base =
        {
            .package_api = &s_server_api,
        },
        // TODO: initialise SERVER-specific fields once wire layout is defined
    };
}

static int8_t uart_client_server_pkg_read(uart_package_t self)
{
    uart_client_server_pkg_t *pkg = CONTAINER_OF(self, uart_client_server_pkg_t, base.package_api);
    // TODO: call protocol_recv_packet to receive the response into a local payload buffer
    // TODO: decode each field byte-by-byte from the payload (little-endian, no pointer casts)
    // TODO: store decoded values into pkg fields
    // TODO: return 0 on success, -1 on error
    (void)pkg;
    return 0;
}

static int8_t uart_client_server_pkg_write(uart_package_t self, uint8_t *data, size_t len)
{
    uart_client_server_pkg_t *pkg = CONTAINER_OF(self, uart_client_server_pkg_t, base.package_api);
    // TODO: serialise pkg fields into data byte-by-byte (little-endian)
    // TODO: return 0 on success, -1 on error
    (void)pkg; (void)data; (void)len;
    return 0;
}

// --- SENSOR ------------------------------------------------------------------

static void uart_client_sensor_pkg_init(uart_client_sensor_pkg_t *self)
{
    *self = (uart_client_sensor_pkg_t)
    {
        .base =
        {
            .package_api = &s_sensor_api,
        },
        .temperature = 0,
        .humidity    = 0,
        .pressure    = 0,
        .timestamp   = 0,
    };
}

static int8_t uart_client_sensor_pkg_read(uart_package_t self)
{
    uart_client_sensor_pkg_t *pkg = CONTAINER_OF(self, uart_client_sensor_pkg_t, base.package_api);
    // TODO: call protocol_recv_packet to receive the response into a local payload buffer
    // TODO: reconstruct temperature from payload[0..3]   as int32_t  (little-endian bit-shifts)
    // TODO: reconstruct humidity    from payload[4..7]   as int32_t
    // TODO: reconstruct pressure    from payload[8..11]  as int32_t
    // TODO: reconstruct timestamp   from payload[12..15] as uint32_t
    // TODO: store into pkg->temperature, pkg->humidity, pkg->pressure, pkg->timestamp
    // TODO: return 0 on success, -1 on error
    (void)pkg;
    return 0;
}

static int8_t uart_client_sensor_pkg_write(uart_package_t self, uint8_t *data, size_t len)
{
    uart_client_sensor_pkg_t *pkg = CONTAINER_OF(self, uart_client_sensor_pkg_t, base.package_api);
    // TODO: serialise pkg fields into data byte-by-byte (little-endian)
    // TODO: return 0 on success, -1 on error
    (void)pkg; (void)data; (void)len;
    return 0;
}

// --- CONFIG ------------------------------------------------------------------

static void uart_client_config_pkg_init(uart_client_config_pkg_t *self)
{
    *self = (uart_client_config_pkg_t)
    {
        .base =
        {
            .package_api = &s_config_api,
        },
        // TODO: initialise CONFIG-specific fields once wire layout is defined
    };
}

static int8_t uart_client_config_pkg_read(uart_package_t self)
{
    uart_client_config_pkg_t *pkg = CONTAINER_OF(self, uart_client_config_pkg_t, base.package_api);
    // TODO: call protocol_recv_packet to receive the response into a local payload buffer
    // TODO: decode each field byte-by-byte from the payload (little-endian, no pointer casts)
    // TODO: store decoded values into pkg fields
    // TODO: return 0 on success, -1 on error
    (void)pkg;
    return 0;
}

static int8_t uart_client_config_pkg_write(uart_package_t self, uint8_t *data, size_t len)
{
    uart_client_config_pkg_t *pkg = CONTAINER_OF(self, uart_client_config_pkg_t, base.package_api);
    // TODO: serialise pkg fields into data byte-by-byte (little-endian)
    // TODO: return 0 on success, -1 on error
    (void)pkg; (void)data; (void)len;
    return 0;
}

// --- DIAG --------------------------------------------------------------------

static void uart_client_diag_pkg_init(uart_client_diag_pkg_t *self)
{
    *self = (uart_client_diag_pkg_t)
    {
        .base =
        {
            .package_api = &s_diag_api,
        },
        // TODO: initialise DIAG-specific fields once wire layout is defined
    };
}

static int8_t uart_client_diag_pkg_read(uart_package_t self)
{
    uart_client_diag_pkg_t *pkg = CONTAINER_OF(self, uart_client_diag_pkg_t, base.package_api);
    // TODO: call protocol_recv_packet to receive the response into a local payload buffer
    // TODO: decode each field byte-by-byte from the payload (little-endian, no pointer casts)
    // TODO: store decoded values into pkg fields
    // TODO: return 0 on success, -1 on error
    (void)pkg;
    return 0;
}

static int8_t uart_client_diag_pkg_write(uart_package_t self, uint8_t *data, size_t len)
{
    uart_client_diag_pkg_t *pkg = CONTAINER_OF(self, uart_client_diag_pkg_t, base.package_api);
    // TODO: serialise pkg fields into data byte-by-byte (little-endian)
    // TODO: return 0 on success, -1 on error
    (void)pkg; (void)data; (void)len;
    return 0;
}

// --- lifecycle ---------------------------------------------------------------

int uart_client_init(uart_client_t *self)
{
    // Call protocol_crc16_init() to build the CRC lookup table

    // Declare a zero-initialised glob_t

    // Call glob() for "/dev/ttyUSB*" with flags = 0
    // If the result is 0 or GLOB_NOMATCH (not a real error), call glob() again
    //   for "/dev/ttyACM*" with GLOB_APPEND so results merge into the same glob_t

    // If g.gl_pathc == 0: print "No USB serial devices found", globfree and return -1

    // If g.gl_pathc == 1: auto-select g.gl_pathv[0], print which device is being used

    // Otherwise print a numbered list, prompt the user with "Select: ",
    //   read one character with fgetc(stdin), convert it to an index (ch - '0')
    //   If the index is >= g.gl_pathc: print "Invalid selection", globfree and return -1

    // Call serial_open(path) to open and configure the port
    // Call globfree — path string is no longer needed after serial_open
    // Store the returned fd in self->fd; return -1 if it is negative, 0 on success
}

void uart_client_work(uart_client_t *self)
{
    // Loop forever:
    //   Print the menu: [0] STATUS  [1] SERVER  [2] SENSOR  [3] CONFIG  [4] DIAG  [q] Quit
    //   Read one character from stdin with fgetc()
    //   If 'q' or 'Q': break

    //   Map the character to a uart_pkg_tag_t; for any unrecognised character print an
    //     error and continue to the next iteration

    //   Call protocol_send_cmd(self->fd, tag) — on failure continue to the next iteration

    //   Call uart_client_create_package(tag) to get a handle; on NULL continue
    //   Call uart_factory_read_package(pkg) to receive and decode the response
    //   Call uart_client_destroy_package(pkg) to release the handle
}

void uart_client_deinit(uart_client_t *self)
{
    // Call serial_close(self->fd) to flush and close the port
}

// --- factory -----------------------------------------------------------------

uart_package_t uart_client_create_package(uart_pkg_tag_t tag)
{
    // TODO: switch on tag, malloc the matching concrete type and call its init function:
    //   TAG_STATUS → uart_client_status_pkg_t, uart_client_status_pkg_init
    //   TAG_SERVER → uart_client_server_pkg_t, uart_client_server_pkg_init
    //   TAG_SENSOR → uart_client_sensor_pkg_t, uart_client_sensor_pkg_init
    //   TAG_CONFIG → uart_client_config_pkg_t, uart_client_config_pkg_init
    //   TAG_DIAG   → uart_client_diag_pkg_t,   uart_client_diag_pkg_init
    // TODO: on unknown tag or malloc failure return NULL
    // TODO: return &pkg->base.package_api as the uart_package_t handle
}

void uart_client_destroy_package(uart_package_t pkg)
{
    // TODO: uart_base_t *base = CONTAINER_OF(pkg, uart_base_t, package_api)
    // TODO: free(base) — base is the first member of every concrete type so this frees the whole allocation
}

int8_t uart_factory_read_package(uart_package_t pkg)
{
    // TODO: dispatch through the vtable: return (*pkg)->read_package(pkg)
}

int8_t uart_factory_write_package(uart_package_t pkg, uint8_t *data, size_t len)
{
    // TODO: dispatch through the vtable: return (*pkg)->write_package(pkg, data, len)
}
