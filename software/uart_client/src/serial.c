#define _GNU_SOURCE
#include "serial.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define BAUD_RATE B115200

int serial_open(const char *path)
{
    int fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        fprintf(stderr, "serial_open: open %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);
    cfmakeraw(&tty);
    cfsetispeed(&tty, BAUD_RATE);
    cfsetospeed(&tty, BAUD_RATE);
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIFLUSH);

    return fd;
}

void serial_close(int fd)
{
    tcflush(fd, TCIOFLUSH);
    close(fd);
}

int serial_read_exact(int fd, void *buf, size_t len, int timeout_ms)
{
    uint8_t *p         = buf;
    size_t   remaining = len;
    int      result    = 0;

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (tfd < 0)
    {
        fprintf(stderr, "serial_read_exact: timerfd_create: %s\n", strerror(errno));
        return -1;
    }

    struct itimerspec ts = {
        .it_value    = { .tv_sec  =  timeout_ms / 1000,
                         .tv_nsec = (long)(timeout_ms % 1000) * 1000000L },
        .it_interval = { 0 },
    };
    timerfd_settime(tfd, 0, &ts, NULL);

    struct pollfd fds[2] = {
        { .fd = fd,  .events = POLLIN },
        { .fd = tfd, .events = POLLIN },
    };

    while (remaining > 0)
    {
        int ready = poll(fds, 2, -1);
        if (ready < 0)
        {
            fprintf(stderr, "serial_read_exact: poll: %s\n", strerror(errno));
            result = -1;
            break;
        }

        if (fds[1].revents & POLLIN)
        {
            uint64_t exp;
            (void)read(tfd, &exp, sizeof(exp));
            fprintf(stderr, "serial_read_exact: timeout waiting for %zu bytes\n", remaining);
            result = -1;
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            ssize_t n = read(fd, p, remaining);
            if (n < 0)
            {
                fprintf(stderr, "serial_read_exact: read: %s\n", strerror(errno));
                result = -1;
                break;
            }
            p         += (size_t)n;
            remaining -= (size_t)n;
        }
    }

    close(tfd);
    return result;
}

int serial_write_exact(int fd, const void *buf, size_t len)
{
    const uint8_t *p = buf;
    size_t remaining = len;

    while (remaining > 0)
    {
        ssize_t n = write(fd, p, remaining);
        if (n < 0)
        {
            fprintf(stderr, "serial_write_exact: write: %s\n", strerror(errno));
            return -1;
        }
        p         += (size_t)n;
        remaining -= (size_t)n;
    }

    return 0;
}
