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

unsigned char *getFileData(FILE* file, long int fileSize);

unsigned char *getControlPacket(const unsigned int C, const char* filename, long int fileSize, unsigned int* cpSize);

unsigned char *getDataPacket(unsigned char *data, int dataSize, int *packetSize);

unsigned char *parseStartControlPacket(unsigned char* packet, int size, unsigned long int *fileSize);

void parseDataPacket(unsigned char* buffer, const unsigned char* packet, const unsigned int packetSize);

#endif // _APPLICATION_LAYER_H_
