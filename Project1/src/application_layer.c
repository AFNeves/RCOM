// Application layer protocol implementation

#include "link_layer.h"
#include "application_layer.h"

const char *nameAppendix = "-received";

long int totalBytesSent = 0;
long int totalBytesReceived = 0;

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

    printf("Connection established\n\n");

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

            printf("File Size: %ld\n\n", fileSize);

            int cpSize;
            unsigned char *controlPacket_Start = getControlPacket(2, filename, fileSize, &cpSize);

            printf("Starter Control Packet: ");

            if (llwrite(fd, controlPacket_Start, cpSize) == -1) exit(-1);

            long int bytesLeft = fileSize;
            unsigned char *fileData = getFileData(file, fileSize);

            int payloadNumber = 1;
            while (bytesLeft > 0)
            {
                int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
                unsigned char *data = (unsigned char *) malloc(dataSize);

                memcpy(data, fileData, dataSize);

                int packetSize;
                unsigned char *packet = getDataPacket(data, dataSize, &packetSize);

                printf("Payload #%d: ", payloadNumber);
                
                if (llwrite(fd, packet, packetSize) == -1) exit(-1);
                
                fileData += dataSize; totalBytesSent += dataSize; payloadNumber++;
                bytesLeft -= (long int) MAX_PAYLOAD_SIZE;
            }

            unsigned char *controlPacket_End = getControlPacket(3, filename, fileSize, &cpSize);

            printf("Ending Control Packet: ");

            if(llwrite(fd, controlPacket_End, cpSize) == -1) exit(-1);

            printf("Total Bytes Sent: %ld\n\n", totalBytesSent);

            llclose(fd, LlTx);

            printf("Connection closed\n\n");

            break;
        }

        case LlRx:
        {
            unsigned char *packet = (unsigned char *) malloc(MAX_PAYLOAD_SIZE + 3);

            printf("Starter Control Packet: ");

            int packetSize = -1;
            while ((packetSize = llread(fd, packet)) < 0);

            long int fileSize = 0;
            char *name = parseControlPacket(packet, &fileSize, nameAppendix);

            printf("File Size: %ld\n\n", fileSize);
            printf("File Name: %s\n\n", name);

            FILE* newFile = fopen(filename, "wb+");
            if (newFile == NULL) exit(-1);

            int payloadNumber = 1;
            LinkLayerState state = START;
            while (state != STOP)
            {
                printf("Payload #%d: ", payloadNumber);

                while ((packetSize = llread(fd, packet)) < 0); 

                if (packet[0] == 1)
                {
                    unsigned char *buffer = (unsigned char *) malloc(packetSize - 3);
                    memcpy(buffer, packet + 3, packetSize - 3);
                    fwrite(buffer, sizeof(unsigned char), packetSize - 3, newFile);
                    free(buffer);
                }
                else if (packet[0] == 3)
                {
                    printf("Total Bytes Received: %ld\n\n", totalBytesReceived);

                    long int fileSize2;
                    name = parseControlPacket(packet, &fileSize2, nameAppendix);

                    if ((fileSize == fileSize2))
                    {
                        fclose(newFile);
                        state = STOP;
                    }
                }

                payloadNumber++;
                totalBytesReceived += packetSize - 3;
            }

            llclose(fd, LlRx);

            printf("Connection closed\n\n");

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

    fclose(file);
    
    return data;
}

char *getNewFilename(const char *filename, const char *appendix, int fileNameSize)
{
    const char *dotPosition = strrchr(filename, '.');

    char *newFilename = (char *) malloc(fileNameSize);

    strncpy(newFilename, filename, (int)(dotPosition - filename));

    newFilename[(int)(dotPosition - filename)] = '\0';

    strcat(newFilename, appendix);

    strcat(newFilename, dotPosition);

    return newFilename;
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
        packet[3 + i] = (unsigned char) ((fileSize >> (8 * i)) & 0xFF);
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

char *parseControlPacket(unsigned char *packet, long int *fileSize, const char *appendix)
{
    int fileSizeBytes = packet[2];
    int fileNameBytes = packet[4 + fileSizeBytes];

    for (int i = fileSizeBytes - 1; i >= 0; i--)
        *fileSize = (*fileSize << 8) | packet[3 + i];

    unsigned char *filename = (unsigned char *) malloc(fileNameBytes + 1);

    memcpy(filename, packet + 5 + fileSizeBytes, fileNameBytes);

    filename[fileNameBytes] = '\0';

    char *newFileName = getNewFilename((const char *) filename, appendix, fileNameBytes + strlen(appendix) + 1);

    return newFileName;
}
