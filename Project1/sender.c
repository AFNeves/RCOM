// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define TRUE 1
#define FALSE 0

#define BUF_SIZE 256

int alarmCount = 0;
int alarmEnabled = FALSE;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

volatile int STOP = FALSE;

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

    // Open serial port device for reading and writing, and not as controlling tty
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
    newtio.c_cc[VMIN] = 1;  // Blocking read until 5 chars received

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

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    // Create SET UP buffer
    unsigned char set_up[BUF_SIZE] = {0};
    // Create buffer to fill with UA package
    unsigned char bufrec[BUF_SIZE + 1] = {0};

	// Fill the SET UP buffer
    set_up[0] = 0x7E;
    set_up[1] = 0x03;
    set_up[2] = 0x03;
    set_up[3] = set_up[1] ^ set_up[2];
    set_up[4] = 0x7E;

    // Trying to send the SET UP
    while (alarmCount < 4)
    {
        if (alarmEnabled == FALSE)
        {
            // Write the buffer in the port
            int bytes = write(fd, set_up, BUF_SIZE);

            // Wait until all bytes have been written to the serial port
            sleep(1);

            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE; // Enable alarm
        }

        // Receive the UA package if available
        int bytes = read(fd, bufrec, BUF_SIZE);
		
		// Check if it is a UA package
        if (bufrec[0] == 0x7E && bufrec[2] == 0x07) {
            printf("Received UA!\n");
            alarm(0); // Disable alarm
            break;
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
