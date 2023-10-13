// Application layer protocol implementation

#include "link_layer.h"
#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    linkLayer.serialPort = serialPort;
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    int fd = llopen(linkLayer);
    if (fd < 0) exit(-1);

    switch (linkLayer.role)
    {
        case LlTx:
        {
            FILE* file = fopen(filename, "rb");
            if (file == NULL) exit(-1);

            int prevPos = ftell(file);
            fseek(file,0L,SEEK_END);
            long int fileSize = ftell(file) - prevPos;
            fseek(file,prevPos,SEEK_SET);

            unsigned int cpSize;
            unsigned char *controlPacket_Start = getControlPacket(2, filename, fileSize, &cpSize); //TODO
            if (llwrite(fd, controlPacket_Start, cpSize) == -1)
            { 
                printf("Error in start package\n");
                exit(-1);
            }

            long int bytesLeft = fileSize;
            unsigned char *fileData = getFileData(file, fileSize);

            while (bytesLeft > 0)
            {
                int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
                unsigned char* data = (unsigned char *) malloc(dataSize);
                memcpy(data, fileData, dataSize);

                int packetSize;
                unsigned char *packet = getDataPacket(data, dataSize, &packetSize);
                
                if (llwrite(fd, packet, packetSize) == -1)
                {
                    printf("Error in data packets\n");
                    exit(-1);
                }
                
                fileData += dataSize;
                bytesLeft -= (long int) MAX_PAYLOAD_SIZE;
            }

            unsigned char *controlPacket_End = getControlPacket(3, filename, fileSize, &cpSize); //TODO
            if(llwrite(fd, controlPacket_End, cpSize) == -1)
            { 
                printf("Error in end packet\n");
                exit(-1);
            }

            llclose(fd, LlTx);
            break;
        }
        case LlRx:
        {
            int packetSize = -1;
            unsigned char *packet = (unsigned char *) malloc(MAX_PAYLOAD_SIZE);

            while ((packetSize = llread(fd, packet)) < 0);

            unsigned long int fileSize;
            unsigned char* filename = parseStartControlPacket(packet, packetSize, &fileSize);

            FILE* newFile = fopen((char *) filename, "wb+");
            while (1) {    
                while ((packetSize = llread(fd, packet)) < 0);
                if (packet[0] != 3)
                {
                    unsigned char *buffer = (unsigned char *) malloc(packetSize);
                    parseDataPacket(buffer,packet, packetSize);
                    fwrite(buffer, sizeof(unsigned char), packetSize - 4, newFile);
                    free(buffer);
                }
                else if (packetSize == -2) llclose(fd, LlRx);
                else break;
            }

            fclose(newFile);
            break;
        }
        default:
        {
            exit(-1);
            break;
        }
    }
}

unsigned char *getFileData(FILE* file, long int fileSize)
{
    unsigned char *data = (unsigned char *) malloc(sizeof(unsigned char) * fileSize);
    fread(data, sizeof(unsigned char), fileSize, file);
    return data;
}

unsigned char *makeControlPacket(const unsigned int C, const char* filename, long int fileSize, unsigned int* cpSize)
{
    
}

















unsigned char * getDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *packetSize);


















unsigned char* parseStartControlPacket(unsigned char *packet, int size, unsigned long int *fileSize);

void parseDataPacket(const unsigned char* packet, const unsigned int packetSize, unsigned char* buffer);

unsigned char * getData(FILE* fd, long int fileLength);
