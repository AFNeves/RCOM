// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC1_OK,
    BCC2_OK,
    STOP
} LinkLayerState;

// ----- MACROS -----

#define MAX_PAYLOAD_SIZE 1000
#define BAUDRATE 38400
#define _POSIX_SOURCE 1

#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

// FRAMES

#define FLAG 0x7E
#define ESC 0x7D

#define A_ER 0x03
#define A_RE 0x01

#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define C_I(N) (N << 6)
#define C_RR(N) ((N << 7) || 0x05)
#define C_REJ(N) ((N << 7) || 0x01)

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

// Alarm function handler
void alarmHandler(int signal);

// Send supervision frame
int sendSupervisionFrame(int serialPort, unsigned char A, unsigned char C);

// Checks the quality of the received control frame and returns its control byte
unsigned char checkControlFrame(int fd);

#endif // _LINK_LAYER_H_
