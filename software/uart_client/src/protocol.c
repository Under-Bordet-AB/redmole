#include "protocol.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint16_t s_crc16_table[256];

void protocol_crc16_init(void)
{
    // Loop i from 0 to 255:
    //   Start with crc = (uint16_t)i
    //   Loop j from 0 to 7 (one bit per iteration):
    //     If the LSB of crc is set: crc = (crc >> 1) ^ 0xA001
    //     Otherwise:                crc = (crc >> 1)
    //   Store crc into s_crc16_table[i]
}

uint16_t protocol_crc16_update(uint16_t crc, const uint8_t *data, size_t len)
{
    // Loop over each byte in data:
    //   crc = (crc >> 8) ^ s_crc16_table[(crc ^ byte) & 0xFF]
    // Return crc
}

int protocol_send_cmd(int fd, uint8_t tag)
{
    // Write the single byte tag to fd with write()
    // If write() did not return 1, print an error and return -1
    // Return 0
}

int protocol_recv_packet(int fd, uint8_t *tag_out, uint8_t *payload_out, uint16_t *data_len_out)
{
    // Phase 1 — read the 3-byte header with serial_read_exact()
    //   Byte 0 is the tag
    //   Bytes 1-2 are data_len in little-endian: reconstruct byte-by-byte (no pointer cast)
    //   If serial_read_exact fails return -1

    // Sanity check: reject data_len == 0 or data_len > MAX_PACKET_PAYLOAD
    //   Print an error and return -1 if out of range

    // Phase 2 — read exactly data_len bytes into payload_out with serial_read_exact()
    //   If serial_read_exact fails return -1

    // CRC validation:
    //   Compute CRC over payload bytes 0 .. data_len-3 (everything except the last two)
    //     using protocol_crc16_update(0, ...)
    //   Reconstruct received CRC from the last two payload bytes (little-endian, byte-by-byte)
    //   If they differ, print both values and return -1

    // Write tag and data_len into the out-params
    // Return 0
}

// --- decode functions -------------------------------------------------------
// Each function receives the validated payload (CRC bytes already present but
// you can ignore them — they were checked in protocol_recv_packet).
// Extract fields byte-by-byte (little-endian), never cast the buffer to a struct.

void protocol_decode_status(const uint8_t *payload, uint16_t data_len)
{
    // TODO: define and extract STATUS fields, print them
    (void)payload; (void)data_len;
}

void protocol_decode_server(const uint8_t *payload, uint16_t data_len)
{
    // TODO: define and extract SERVER fields, print them
    (void)payload; (void)data_len;
}

void protocol_decode_sensor(const uint8_t *payload, uint16_t data_len)
{
    // Payload layout (all little-endian, 16 bytes before the 2-byte CRC):
    //   [0..3]   temperature_x_100  (int32_t)
    //   [4..7]   humidity_x_100     (int32_t)
    //   [8..11]  pressure_x_100     (int32_t)
    //   [12..15] timestamp_s        (uint32_t)

    // Reconstruct each int32_t field from 4 bytes with bit-shifts
    // Divide by 100.0 to recover the real value
    // Print temperature, humidity, pressure and timestamp

    (void)data_len;
    (void)payload;
}

void protocol_decode_config(const uint8_t *payload, uint16_t data_len)
{
    // TODO: define and extract CONFIG fields, print them
    (void)payload; (void)data_len;
}

void protocol_decode_diag(const uint8_t *payload, uint16_t data_len)
{
    // TODO: define and extract DIAG fields, print them
    (void)payload; (void)data_len;
}
