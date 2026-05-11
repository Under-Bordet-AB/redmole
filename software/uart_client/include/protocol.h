#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "serial.h"

typedef enum {
    TAG_STATUS = 0,
    TAG_SERVER = 1,
    TAG_SENSOR = 2,
    TAG_CONFIG = 3,
    TAG_DIAG   = 4,
} uart_pkg_tag_t;

void     crc16_init(void);
uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t len);

int send_cmd(int fd, uint8_t tag);
int recv_packet(int fd, uint8_t *tag_out, uint8_t *payload_out, uint16_t *data_len_out);

void decode_status(const uint8_t *payload, uint16_t data_len);
void decode_server(const uint8_t *payload, uint16_t data_len);
void decode_sensor(const uint8_t *payload, uint16_t data_len);
void decode_config(const uint8_t *payload, uint16_t data_len);
void decode_diag  (const uint8_t *payload, uint16_t data_len);

#endif /* PROTOCOL_H */
