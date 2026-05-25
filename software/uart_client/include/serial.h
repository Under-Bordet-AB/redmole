#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <stdint.h>

#define RECV_TIMEOUT_MS    2000
#define MAX_PACKET_PAYLOAD 12288

int  serial_open(const char *path);
void serial_close(int fd);
int  serial_read_exact(int fd, void *buf, size_t len, int timeout_ms);
int  serial_write_exact(int fd, const void *buf, size_t len);

#endif /* SERIAL_H */
