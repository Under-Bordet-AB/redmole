/**
 * @file
 * @brief UART packet framing and decoding for the uart_client.
 *
 * Provides CRC-16/X25 computation, a command sender, a packet receiver,
 * and per-tag payload decoders. The CRC algorithm must match the ESP32
 * firmware: polynomial 0x8408 (reflected 0x1021), init=0xFFFF, xorout=0xFFFF.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "serial.h"

/**
 * @brief Packet type tags identifying the payload format.
 */
typedef enum
{
    TAG_STATUS  = 0, /*!< Status response packet. */
    TAG_SERVER  = 1, /*!< Server data response packet. */
    TAG_SENSOR  = 2, /*!< Sensor readings packet. */
    TAG_RESTART = 3, /*!< Restart acknowledgement packet. */
    TAG_DIAG    = 4, /*!< Diagnostics packet. */
    TAG_TEST    = 5, /*!< Test/loopback packet. */
} uart_pkg_tag_t;

/**
 * @brief Initialise the CRC-16/X25 lookup table.
 *
 * Must be called once before protocol_crc16_update().
 */
void protocol_crc16_init(void);

/**
 * @brief Update a running CRC-16/X25 checksum with additional data.
 *
 * @param crc  Running CRC value; pass 0xFFFF for the first call.
 * @param data Pointer to the input bytes; must not be NULL.
 * @param len  Number of bytes to process.
 * @return Updated CRC value after processing len bytes.
 * @see protocol_crc16_init
 */
uint16_t protocol_crc16_update(uint16_t crc, const uint8_t *data, size_t len);

/**
 * @brief Send a single-byte command packet to the ESP32.
 *
 * @param fd  Open serial file descriptor.
 * @param tag Command tag identifying the requested packet type.
 * @return 0 on success, -1 on write error.
 */
int protocol_send_cmd(int fd, uint8_t tag);

/**
 * @brief Receive one framed packet from the ESP32 and validate its CRC.
 *
 * Blocks until a complete packet arrives or the read timeout elapses.
 * payload_out must be large enough to hold MAX_PACKET_PAYLOAD bytes.
 *
 * @param fd           Open serial file descriptor.
 * @param tag_out      Output: populated with the received packet tag; must not be NULL.
 * @param payload_out  Output: buffer for the decoded payload; must not be NULL.
 * @param data_len_out Output: populated with the payload length in bytes; must not be NULL.
 * @return 0 on success, -1 on framing or CRC error, or a negative errno on read failure.
 */
int protocol_recv_packet(int fd, uint8_t *tag_out, uint8_t *payload_out, uint16_t *data_len_out);

/**
 * @brief Decode and print a STATUS payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_status(const uint8_t *payload, uint16_t data_len);

/**
 * @brief Decode and print a SERVER payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_server(const uint8_t *payload, uint16_t data_len);

/**
 * @brief Decode and print a SENSOR payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_sensor(const uint8_t *payload, uint16_t data_len);

/**
 * @brief Decode and print a RESTART payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_restart(const uint8_t *payload, uint16_t data_len);

/**
 * @brief Decode and print a DIAGNOSTICS payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_diag  (const uint8_t *payload, uint16_t data_len);

/**
 * @brief Decode and print a TEST payload.
 *
 * @param payload  Pointer to the raw payload bytes; must not be NULL.
 * @param data_len Payload length in bytes.
 */
void protocol_decode_test  (const uint8_t *payload, uint16_t data_len);

#endif /* PROTOCOL_H */
