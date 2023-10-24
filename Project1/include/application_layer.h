// Application layer protocol header.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>
#include <stdlib.h>

// Application layer main function.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

// Fetches the file data.
unsigned char *getFileData(FILE* file, long int fileSize);

// Generates the new filename.
char *getNewFilename(const char *filename, const char *appendix, int fileNameSize);

// Parses a control packet.
char *parseControlPacket(unsigned char *packet, long int *fileSize, const char *appendix);

// Creates a data packet with the given parameters.
unsigned char *getDataPacket(unsigned char *data, int dataSize, int *packetSize);

// Creates a control packet with the given parameters.
unsigned char *getControlPacket(unsigned char C, const char *filename, long int fileSize, int *packetSize);

#endif // _APPLICATION_LAYER_H_
