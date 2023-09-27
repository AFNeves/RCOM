#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "macros.h" // File containing all the macros

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define TRUE 1
#define FALSE 0

#define BUF_SIZE 256

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

	// ---------------------- //

    printf("Setting up connection...\n");

    // Create UA buffer
    unsigned char buf[BUF_SIZE] = {0};
    // Create buffer to fill with SET UP package
    unsigned char bufrec[BUF_SIZE + 1] = {0};

    // Fill the UA buffer
    buf[0] = FLAG;
    buf[1] = UA_ADDRESS;
    buf[2] = UA_CONTROL;
    buf[3] = UA_BCC;
    buf[4] = FLAG;

    printf("Awaiting SET UP...\n");

    int state = STATE_START;
    while (state != STATE_STOP)
    {
        // Receive the SET UP package if available
        int bytes = read(fd, bufrec, 1);

        if (bytes > 0)
        {
            switch (state)
            {
            case STATE_START:
                if (bufrec[0] == FLAG)
                {
                    state = STATE_FLAG_RCV;
                }
                break;
            case STATE_FLAG_RCV:
                if (bufrec[0] == SET_UP_ADDRESS)
                {
                    state = STATE_A_RCV;
                }
                else if (bufrec[0] == FLAG)
                {
                    state = STATE_FLAG_RCV;
                }
                else
                {
                    state = STATE_START;
                }
                break;
            case STATE_A_RCV:
                if (bufrec[0] == SET_UP_CONTROL)
                {
                    state = STATE_C_RCV;
                }
                else if (bufrec[0] == FLAG)
                {
                    state = STATE_FLAG_RCV;
                }
                else
                {
                    state = STATE_START;
                }
                break;
            case STATE_C_RCV:
                if (bufrec[0] == SET_UP_BCC)
                {
                    state = STATE_BCC_OK;
                }
                else if (bufrec[0] == FLAG)
                {
                    state = STATE_FLAG_RCV;
                }
                else
                {
                    state = STATE_START;
                }
                break;
            case STATE_BCC_OK:
                if (bufrec[0] == FLAG)
                {
                    state = STATE_STOP;

                    printf("Received SET UP!\n");
                    
                    // Write the buffer in the port
                    int bytes = write(fd, buf, BUF_SIZE);
	                printf("UA sent.\n");
                }
                else
                {
                    state = STATE_START;
                }
                break;
            default:
                printf("ERROR: No state with such name\n");
                break;
            }
        }
    }

	// ---------------------- //

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
