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

            int cpSize;
            unsigned char *controlPacket_Start = getControlPacket(2, filename, fileSize, &cpSize);
            if (llwrite(fd, controlPacket_Start, cpSize) == -1) exit(-1);

            long int bytesLeft = fileSize;
            unsigned char *fileData = getFileData(file, fileSize);

            while (bytesLeft > 0)
            {
                int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
                unsigned char* data = (unsigned char *) malloc(dataSize);

                memcpy(data, fileData, dataSize);

                int packetSize;
                unsigned char *packet = getDataPacket(data, dataSize, &packetSize);
                
                if (llwrite(fd, packet, packetSize) == -1) exit(-1);
                
                fileData += dataSize;
                bytesLeft -= (long int) MAX_PAYLOAD_SIZE;
            }

            unsigned char *controlPacket_End = getControlPacket(3, filename, fileSize, &cpSize);

            if(llwrite(fd, controlPacket_End, cpSize) == -1) exit(-1);

            llclose(fd, LlTx);

            break;
        }

        case LlRx:
        {
            unsigned char *packet = (unsigned char *) malloc(MAX_PAYLOAD_SIZE);

            int packetSize = -1;
            while ((packetSize = llread(fd, packet)) < 0);

            long int fileSize;
            unsigned char* name = parseControlPacket(packet, packetSize, &fileSize);

            FILE* newFile = fopen((char *) name, "wb+");
            while (1) {    
                while ((packetSize = llread(fd, packet)) < 0);
                if (packet[0] == 1)
                {
                    unsigned char *buffer = (unsigned char *) malloc(packetSize);
                    memcpy(buffer, packet + 3, packetSize - 3);
                    fwrite(buffer, sizeof(unsigned char), packetSize - 3, newFile);
                    free(buffer);
                }
                else if (packet[0] == 3)
                {
                    long int fileSize2;
                    unsigned char* name2 = parseControlPacket(packet, packetSize, &fileSize);
                    if ((fileSize == fileSize2) && (name == name2))
                    {
                        fclose(newFile);
                        break;
                    }
                }
                else break;
            }

            llclose(fd, LlRx);

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

unsigned char *getControlPacket(unsigned char C, const char *filename, long int fileSize, int *packetSize)
{
    int fileSizeBytes = (int) ceil(log2f((float) fileSize) / 8.0);

    int fileNameBytes = strlen(filename);

    *packetSize = 5 + fileSizeBytes + fileNameBytes;

    unsigned char *packet = (unsigned char *) malloc(*packetSize);
    
    int packetPos = 0;
    packet[packetPos++] = C;
    packet[packetPos++] = 0;
    packet[packetPos++] = (unsigned char) fileSizeBytes;

    for (int i = 0 ; i < fileSizeBytes ; i++)
    {
        packet[2 + fileSizeBytes - i] = (fileSize >> (8 * i)) & 0xFF;
        packetPos++;
    }

    packet[packetPos++] = 1;
    packet[packetPos++] = (unsigned char) fileNameBytes;

    memcpy(packet + packetPos, filename, fileNameBytes);

    return packet;
}

unsigned char *getDataPacket(unsigned char *data, int dataSize, int *packetSize)
{
    *packetSize = 3 + dataSize;

    unsigned char *packet = (unsigned char *) malloc(*packetSize);

    packet[0] = 1;
    packet[1] = (unsigned char) dataSize / 256;
    packet[2] = (unsigned char) dataSize % 256;

    memcpy(packet + 3, data, dataSize);

    return packet;
}

unsigned char *parseControlPacket(unsigned char* packet, long int *fileSize)
{
    unsigned char fileSizeBytes = packet[2];
    unsigned char fileNameBytes = packet[4 + fileSizeBytes];

    for (int i = 0; i < (int) fileSizeBytes; i++)
        *fileSize |= packet[3 + fileSizeBytes - i] << (8 * i);

    unsigned char *name = (unsigned char *) malloc(fileNameBytes);

    memcpy(name, packet + 3 + fileSizeBytes + 2, fileNameBytes);

    return name;
}
