#include <stdio.h>
#include <glob.h>
#include "protocol.h"
#include "serial.h"

static int  init(void);
static void work(int fd);
static void deinit(int fd);

int main(void)
{
    int fd = init();
    if (fd < 0)
        return 1;

    work(fd);
    deinit(fd);
    return 0;
}

static int init(void)
{
    // Declare a zero-initialised glob_t

    // Call glob() for "/dev/ttyUSB*" with flags = 0
    // If the result is 0 or GLOB_NOMATCH (not a real error), call glob() again
    //   for "/dev/ttyACM*" with GLOB_APPEND so results merge into the same glob_t

    // If g.gl_pathc == 0: print "No USB serial devices found", globfree and return -1

    // If g.gl_pathc == 1: auto-select g.gl_pathv[0], print which device is being used

    // Otherwise print a numbered list, prompt the user with "Select: ",
    //   read one character with fgetc(stdin), convert it to an index (ch - '0')
    //   If the index is >= g.gl_pathc: print "Invalid selection", globfree and return -1

    // Call open_serial(path) to open and configure the port
    // Call globfree — path string is no longer needed after open_serial
    // Return the fd from open_serial
}

static void work(int fd)
{
    // Loop forever:
    //   Print the menu: [0] STATUS  [1] SERVER  [2] SENSOR  [3] CONFIG  [4] DIAG  [q] Quit
    //   Read one character from stdin with fgetc()
    //   If 'q' or 'Q': break

    //   Map the character to a uart_pkg_tag_t; for any unrecognised character print an
    //     error and continue to the next iteration

    //   Call send_cmd() — on failure continue to the next iteration

    //   Declare a payload buffer of MAX_PACKET_PAYLOAD bytes and a data_len variable
    //   Call recv_packet() — on failure continue to the next iteration

    //   Switch on the returned tag and call the matching decode_* function
}

static void deinit(int fd)
{
    // Call close_serial to flush and close the port
}
