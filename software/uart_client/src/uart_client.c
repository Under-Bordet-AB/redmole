#include "uart_client.h"
#include "protocol.h"
#include "serial.h"

#include <glob.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/* Module-level fd — safe because the work loop is fully sequential. */
static int s_fd = -1;

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

/* --- concrete package types ------------------------------------------------ */

typedef struct { uart_base_t base; } pkg_status_t;
typedef struct { uart_base_t base; } pkg_server_t;
typedef struct { uart_base_t base; } pkg_sensor_t;
typedef struct { uart_base_t base; } pkg_restart_t;
typedef struct { uart_base_t base; } pkg_diag_t;

/* --- vtable implementations ------------------------------------------------ */

static int8_t read_status(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_status(payload, data_len);
    return 0;
}

static int8_t read_server(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_server(payload, data_len);
    return 0;
}

static int8_t read_sensor(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_sensor(payload, data_len);
    return 0;
}

static int8_t read_restart(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_restart(payload, data_len);
    return 0;
}

static int8_t read_diag(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_diag(payload, data_len);
    return 0;
}

static int8_t read_test(uart_package_t self)
{
    (void)self;
    uint8_t tag; uint8_t payload[MAX_PACKET_PAYLOAD]; uint16_t data_len;
    if (protocol_recv_packet(s_fd, &tag, payload, &data_len) < 0) return -1;
    protocol_decode_test(payload, data_len);
    return 0;
}

static struct uart_api s_vtable_status = { read_status, NULL };
static struct uart_api s_vtable_server = { read_server, NULL };
static struct uart_api s_vtable_sensor = { read_sensor, NULL };
static struct uart_api s_vtable_restart = { read_restart, NULL };
static struct uart_api s_vtable_diag   = { read_diag,   NULL };
static struct uart_api s_vtable_test   = { read_test,   NULL };

/* --- factory --------------------------------------------------------------- */

uart_package_t uart_client_create_package(uart_pkg_tag_t tag)
{
    size_t sz;
    struct uart_api *vtable;

    switch (tag)
    {
        case TAG_STATUS: sz = sizeof(pkg_status_t); vtable = &s_vtable_status; break;
        case TAG_SERVER: sz = sizeof(pkg_server_t); vtable = &s_vtable_server; break;
        case TAG_SENSOR: sz = sizeof(pkg_sensor_t); vtable = &s_vtable_sensor; break;
        case TAG_RESTART: sz = sizeof(pkg_restart_t); vtable = &s_vtable_restart; break;
        case TAG_DIAG:   sz = sizeof(pkg_diag_t);   vtable = &s_vtable_diag;   break;
        case TAG_TEST:   sz = sizeof(pkg_diag_t);   vtable = &s_vtable_test;   break;
        default:
            fprintf(stderr, "uart_client_create_package: unknown tag %d\n", tag);
            return NULL;
    }

    uart_base_t *base = malloc(sz);
    if (!base) { fprintf(stderr, "uart_client_create_package: malloc failed\n"); return NULL; }
    base->package_api = vtable;
    return &base->package_api;
}

void uart_client_destroy_package(uart_package_t pkg)
{
    if (!pkg) return;
    uart_base_t *base = CONTAINER_OF(pkg, uart_base_t, package_api);
    free(base);
}

int8_t uart_factory_read_package(uart_package_t pkg)
{
    return (*pkg)->read_package(pkg);
}

int8_t uart_factory_write_package(uart_package_t pkg, uint8_t *data, size_t len)
{
    return (*pkg)->write_package(pkg, data, len);
}

/* --- lifecycle ------------------------------------------------------------- */

int uart_client_init(uart_client_t *self)
{
    protocol_crc16_init();

    glob_t g = {0};
    int rc = glob("/dev/ttyUSB*", 0, NULL, &g);
    if (rc == 0 || rc == GLOB_NOMATCH)
        glob("/dev/ttyACM*", GLOB_APPEND, NULL, &g);

    if (g.gl_pathc == 0)
    {
        fprintf(stderr, "No USB serial devices found.\n");
        globfree(&g);
        return -1;
    }

    const char *path;
    if (g.gl_pathc == 1)
    {
        path = g.gl_pathv[0];
        printf("Using %s\n", path);
    }
    else
    {
        for (size_t i = 0; i < g.gl_pathc; i++)
            printf("[%zu] %s\n", i, g.gl_pathv[i]);
        printf("Select: ");
        int ch = fgetc(stdin);
        { int c; while ((c = fgetc(stdin)) != '\n' && c != EOF); }
        size_t idx = (size_t)(ch - '0');
        if (idx >= g.gl_pathc)
        {
            fprintf(stderr, "Invalid selection.\n");
            globfree(&g);
            return -1;
        }
        path = g.gl_pathv[idx];
    }

    int fd = serial_open(path);
    globfree(&g);
    if (fd < 0) return -1;

    self->fd = fd;
    s_fd     = fd;
    return 0;
}

void uart_client_work(uart_client_t *self)
{
    (void)self;

    for (;;)
    {
        printf("\n[0] STATUS  [1] SERVER  [2] SENSOR  [3] RESTART  [4] DIAG  [5] TEST  [q] Quit\n> ");
        fflush(stdout);

        int ch = fgetc(stdin);
        if (ch != '\n' && ch != EOF)
        {
            int c;
            while ((c = fgetc(stdin)) != '\n' && c != EOF);
        }

        if (ch == EOF || ch == 'q' || ch == 'Q') break;
        if (ch == '\n' || ch == '\r') continue;

        uart_pkg_tag_t tag;
        switch (ch)
        {
            case '0': tag = TAG_STATUS; break;
            case '1': tag = TAG_SERVER; break;
            case '2': tag = TAG_SENSOR; break;
            case '3': tag = TAG_RESTART; break;
            case '4': tag = TAG_DIAG;   break;
            case '5': tag = TAG_TEST;   break;
            default:
                fprintf(stderr, "Unknown command '%c'\n", ch);
                continue;
        }

        if (protocol_send_cmd(s_fd, (uint8_t)tag) < 0) continue;

        uart_package_t pkg = uart_client_create_package(tag);
        if (!pkg) continue;

        uart_factory_read_package(pkg);
        uart_client_destroy_package(pkg);
    }
}

void uart_client_deinit(uart_client_t *self)
{
    serial_close(self->fd);
    s_fd = -1;
}
