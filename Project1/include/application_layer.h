// Application layer protocol header.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

// Fetches the file data.
unsigned char *getFileData(FILE* file, long int fileSize);

// Creates a control packet with the given parameters.
unsigned char *getControlPacket(unsigned char C, const char *filename, long int fileSize, int *packetSize);

// Creates a data packet with the given parameters.
unsigned char *getDataPacket(unsigned char *data, int dataSize, int *packetSize);

// Parses a control packet.
unsigned char *parseControlPacket(unsigned char* packet, long int *fileSize);

#endif // _APPLICATION_LAYER_H_
