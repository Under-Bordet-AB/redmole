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

int open_serial(const char *path)
{
    // Open the device at path with O_RDWR | O_NOCTTY | O_SYNC
    // If open() fails, print an error with strerror(errno) and return -1

    // Declare a struct termios and read the current port settings with tcgetattr()
    // Call cfmakeraw() to disable all line discipline processing
    // Set input and output baud rate to BAUD_RATE with cfsetispeed / cfsetospeed
    // Set VMIN = 0 and VTIME = 0 for fully non-blocking reads
    // Apply the new settings immediately with tcsetattr(..., TCSANOW, ...)

    // Return the file descriptor
}

void close_serial(int fd)
{
    // Discard any pending bytes in both TX and RX kernel buffers with tcflush(..., TCIOFLUSH)
    // Close the file descriptor
}

int read_exact(int fd, void *buf, size_t len, int timeout_ms)
{
    // Set up a pointer p into buf and a remaining byte counter starting at len
    // Declare a result variable initialised to 0

    // Create a timerfd with CLOCK_MONOTONIC and TFD_CLOEXEC | TFD_NONBLOCK
    // If timerfd_create() fails return -1

    // Fill an itimerspec: it_value from timeout_ms (split into tv_sec and tv_nsec),
    //   it_interval zero (one-shot, does not repeat)
    // Arm the timer with timerfd_settime()

    // Declare a pollfd array of 2: slot 0 is fd (POLLIN), slot 1 is tfd (POLLIN)

    // Loop while remaining > 0:
    //   Call poll() with timeout -1 (block forever — the timerfd is the deadline)
    //   If poll() returns < 0, print the error, set result = -1, break

    //   Check slot 1 (timerfd) first — if POLLIN is set the deadline expired:
    //     Read 8 bytes from tfd to drain it (the uint64_t expiration count)
    //     Print a timeout message, set result = -1, break

    //   Check slot 0 (serial fd) — if POLLIN is set:
    //     Call read() into p for up to remaining bytes
    //     If read() returns < 0, print the error, set result = -1, break
    //     Advance p and subtract from remaining

    // Close tfd (always — on both success and error paths)
    // Return result
}

int write_exact(int fd, const void *buf, size_t len)
{
    // Set up a const uint8_t pointer into buf and a remaining counter

    // Loop while remaining > 0:
    //   Call write() for up to remaining bytes
    //   If write() returns < 0, print the error and return -1
    //   Advance the pointer and subtract from remaining

    // Return 0
}
