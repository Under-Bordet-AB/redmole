#include "protocol.h"
#include "serial.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static uint16_t s_crc16_table[256];

void protocol_crc16_init(void)
{
    for (int i = 0; i < 256; i++)
    {
        uint16_t crc = (uint16_t)i;
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? ((crc >> 1) ^ 0x8408u) : (crc >> 1);
        s_crc16_table[i] = crc;
    }
}

uint16_t protocol_crc16_update(uint16_t crc, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ s_crc16_table[(crc ^ data[i]) & 0xFF];
    return crc;
}

int protocol_send_cmd(int fd, uint8_t tag)
{
    ssize_t n = write(fd, &tag, 1);
    if (n != 1)
    {
        fprintf(stderr, "protocol_send_cmd: write failed: %s\n", strerror(errno));
        return -1;
    }
    printf("sent cmd 0x%02x\n", tag);
    return 0;
}

int protocol_recv_packet(int fd, uint8_t *tag_out, uint8_t *payload_out, uint16_t *data_len_out)
{
    uint8_t hdr[3];
    if (serial_read_exact(fd, hdr, 3, RECV_TIMEOUT_MS) < 0)
        return -1;

    uint8_t  tag      = hdr[0];
    uint16_t data_len = (uint16_t)(hdr[1] | ((uint16_t)hdr[2] << 8));

    fprintf(stderr, "hdr: [%02x %02x %02x]  tag=%u  data_len=%u\n",
            hdr[0], hdr[1], hdr[2], tag, data_len);

    if (data_len == 0 || data_len > MAX_PACKET_PAYLOAD)
    {
        fprintf(stderr, "protocol_recv_packet: bad data_len %u\n", data_len);
        return -1;
    }

    if (serial_read_exact(fd, payload_out, data_len, RECV_TIMEOUT_MS) < 0)
        return -1;

    uint16_t computed = (uint16_t)~protocol_crc16_update(0xFFFF, payload_out, (size_t)(data_len - 2));
    uint16_t received = (uint16_t)(payload_out[data_len - 2] |
                                   ((uint16_t)payload_out[data_len - 1] << 8));
    if (computed != received)
    {
        fprintf(stderr, "protocol_recv_packet: CRC mismatch: computed 0x%04x received 0x%04x\n",
                computed, received);
        return -1;
    }

    *tag_out      = tag;
    *data_len_out = data_len;
    return 0;
}

void protocol_decode_status(const uint8_t *payload, uint16_t data_len)
{
    if (data_len < 11) { fprintf(stderr, "decode_status: short payload\n"); return; }

    uint8_t  status = payload[0];
    uint32_t uptime = (uint32_t)( payload[1]
                    | ((uint32_t)payload[2] <<  8)
                    | ((uint32_t)payload[3] << 16)
                    | ((uint32_t)payload[4] << 24));
    uint32_t ts     = (uint32_t)( payload[5]
                    | ((uint32_t)payload[6] <<  8)
                    | ((uint32_t)payload[7] << 16)
                    | ((uint32_t)payload[8] << 24));

    printf("--- STATUS ---\n");
    printf("  WiFi:      %s\n", (status & (1 << 0)) ? "online" : "offline");
    printf("  Server:    %s\n", (status & (1 << 1)) ? "online" : "offline");
    printf("  Sensor:    %s\n", (status & (1 << 2)) ? "online" : "offline");
    printf("  Uptime:    %u s\n", uptime);
    printf("  Timestamp: %u s\n", ts);
}

void protocol_decode_server(const uint8_t *payload, uint16_t data_len)
{
    /* payload: [json (data_len-6 bytes)] [timestamp_s (4)] [crc16 (2)] */
    if (data_len < 6) { fprintf(stderr, "decode_server: short payload\n"); return; }

    size_t   json_len = (size_t)(data_len - 6);
    uint32_t ts       = (uint32_t)( payload[json_len]
                      | ((uint32_t)payload[json_len + 1] <<  8)
                      | ((uint32_t)payload[json_len + 2] << 16)
                      | ((uint32_t)payload[json_len + 3] << 24));

    printf("--- SERVER ---\n");
    printf("  JSON (%zu bytes):\n  %.*s\n", json_len, (int)json_len, payload);
    printf("  Timestamp: %u s\n", ts);
}

void protocol_decode_sensor(const uint8_t *payload, uint16_t data_len)
{
    if (data_len < 18) { fprintf(stderr, "decode_sensor: short payload\n"); return; }

    int32_t temp = (int32_t)((uint32_t) payload[0]
                 | ((uint32_t)payload[1]  <<  8)
                 | ((uint32_t)payload[2]  << 16)
                 | ((uint32_t)payload[3]  << 24));
    int32_t humi = (int32_t)((uint32_t) payload[4]
                 | ((uint32_t)payload[5]  <<  8)
                 | ((uint32_t)payload[6]  << 16)
                 | ((uint32_t)payload[7]  << 24));
    int32_t pres = (int32_t)((uint32_t) payload[8]
                 | ((uint32_t)payload[9]  <<  8)
                 | ((uint32_t)payload[10] << 16)
                 | ((uint32_t)payload[11] << 24));
    uint32_t ts  = (uint32_t)( payload[12]
                 | ((uint32_t)payload[13] <<  8)
                 | ((uint32_t)payload[14] << 16)
                 | ((uint32_t)payload[15] << 24));

    printf("--- SENSOR ---\n");
    printf("  Temperature: %.2f C\n",   temp / 100.0);
    printf("  Humidity:    %.2f %%\n",  humi / 100.0);
    printf("  Pressure:    %.2f hPa\n", pres / 100.0);
    printf("  Timestamp:   %u s\n", ts);
}

void protocol_decode_restart(const uint8_t *payload, uint16_t data_len)
{
    if (data_len < 7) { fprintf(stderr, "decode_restart: short payload\n"); return; }

    uint8_t  rslt = payload[0];
    uint32_t ts   = (uint32_t)( payload[1]
                  | ((uint32_t)payload[2] <<  8)
                  | ((uint32_t)payload[3] << 16)
                  | ((uint32_t)payload[4] << 24));

    printf("--- RESTART ---\n");
    printf("  Result:    %s\n", (rslt & (1 << 0)) ? "OK" : (rslt & (1 << 1)) ? "FAILED" : "unknown");
    printf("  Timestamp: %u s\n", ts);
}

void protocol_decode_test(const uint8_t *payload, uint16_t data_len)
{
    /* payload: [0xDE][0xAD][0xBE][0xEF][crc16_lo][crc16_hi] */
    if (data_len < 6) { fprintf(stderr, "decode_test: short payload\n"); return; }

    printf("--- TEST ---\n");
    printf("  Magic: %02x %02x %02x %02x\n",
           payload[0], payload[1], payload[2], payload[3]);
    printf("  %s\n", (payload[0] == 0xDE && payload[1] == 0xAD &&
                      payload[2] == 0xBE && payload[3] == 0xEF)
                     ? "PASS: magic matches 0xDEADBEEF"
                     : "FAIL: magic mismatch");
}

void protocol_decode_diag(const uint8_t *payload, uint16_t data_len)
{
    if (data_len < 15) { fprintf(stderr, "decode_diag: short payload\n"); return; }

    uint8_t  n_tasks    = payload[0];
    uint32_t stack_hw   = (uint32_t)( payload[1]
                        | ((uint32_t)payload[2] <<  8)
                        | ((uint32_t)payload[3] << 16)
                        | ((uint32_t)payload[4] << 24));
    uint32_t stack_used = (uint32_t)( payload[5]
                        | ((uint32_t)payload[6] <<  8)
                        | ((uint32_t)payload[7] << 16)
                        | ((uint32_t)payload[8] << 24));
    uint32_t ts         = (uint32_t)( payload[9]
                        | ((uint32_t)payload[10] <<  8)
                        | ((uint32_t)payload[11] << 16)
                        | ((uint32_t)payload[12] << 24));

    printf("--- DIAG ---\n");
    printf("  Tasks:      %u\n",       n_tasks);
    printf("  Stack free: %u bytes\n", stack_hw);
    printf("  Stack used: %u bytes\n", stack_used);
    printf("  Timestamp:  %u s\n",     ts);
}
