// Link layer protocol implementation

#include "../include/link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;
int timeout = 0;
int nRetransmissions = 0;

unsigned char tramaT = 0;
unsigned char tramaR = 0;

struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount--;

    printf("--- Alarm Alert! ---\n");
}

int sendControlFrame(int serialPort, unsigned char A, unsigned char C)
{
    unsigned char frame[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(serialPort, frame, 5);
}

unsigned char checkControlFrame(int serialPort, unsigned char A)
{
    unsigned char prevAlarmStatus = alarmEnabled;
    alarmEnabled == TRUE;

    unsigned char byte, C = 0xFF;
    LinkLayerState state = START;

    while (state != STOP && alarmEnabled == TRUE)
    {
        if (read(serialPort, byte, 1) > 0)
        {
            switch (state)
            {
            case START:
                if (byte == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
                if (byte == A)
                    state = A_RCV;
                else if (byte != FLAG)
                    state = START;
                break;
            case A_RCV:
                if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) ||
                    byte == C_REJ(1) || byte == C_DISC || byte == C_UA || byte == C_SET)
                {
                    state = C_RCV;
                    C = byte;
                }
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case C_RCV:
                if (byte == (A ^ C))
                    state = BCC1_OK;
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case BCC1_OK:
                if (byte == FLAG)
                {
                    // Received Valid Control Frame!
                    if (prevAlarmStatus == TRUE) alarm(0);
                    alarmEnabled = FALSE;
                    state = STOP;
                }
                else
                    state = START;
                break;
            default:
                break;
            }
        }
    }

    return C;
}

int openSerialPort(const char *serialPort)
{
    int fd = open(serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) return -1;

    if (tcgetattr(fd, oldtio) == -1) return -1;

    memset(newtio, 0, sizeof(*newtio));
    newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;
    newtio->c_lflag = 0;
    newtio->c_cc[VTIME] = 1; // Inter-character timer unused
    newtio->c_cc[VMIN] = 0;  // Read without blocking
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, newtio) == -1) return -1;

    return fd;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    timeout = connectionParameters.timeout;
    alarmCount = connectionParameters.nRetransmissions;
    nRetransmissions = connectionParameters.nRetransmissions;

    int fd = openSerialPort(connectionParameters.serialPort);
    if (fd < 0) return -1;

    switch (connectionParameters.role)
    {

    case LlTx:
    {
        (void)signal(SIGALRM, alarmHandler);
        while (1)
        {
            if (alarmCount == 0) return -1;

            if (alarmEnabled == FALSE)
            {
                sendControlFrame(fd, A_ER, C_SET);
                alarm(timeout);
                alarmEnabled = TRUE;
            }

            if (checkControlFrame(fd, A_RE) == C_UA) break;
        }

        break;
    }

    case LlRx:
    {
        if (checkControlFrame(fd, A_ER) != C_SET) return -1;

        sendSupervisionFrame(fd, A_RE, C_UA);
        
        break;
    }

    default:
        return -1;
        break;
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    int frameSize = bufSize + 6;
    unsigned char *frame = (unsigned char *)malloc(frameSize);

    frame[0] = FLAG;
    frame[1] = A_ER;
    frame[2] = C_I(tramaT);
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame + 4, buf, bufSize);

    unsigned char BCC2 = buf[0];
    for (int i = 1; i < bufSize; i++)
        BCC2 ^= buf[i];

    for (int i = 4; i < frameSize - 1; i++)
    {
        if (frame[i] == FLAG)
        {
            frameSize += 2;
            frame = realloc(frame, frameSize);

            memmove(frame + i + 2, frame + i, frameSize - i - 2);

            frame[i++] = ESC;
            frame[i++] = ESC_1;
            frame[i] = ESC_2;
        }
        else if (frame[i] == ESC)
        {
            frame = realloc(frame, ++frameSize);

            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = ESC;
            frame[i] = ESC_1;
        }
    }

    frame[i++] = BCC2;
    frame[i] = FLAG;

    unsigned char C;
    LinkLayerState state = START;
    alarmCount = nRetransmissions;

    (void)signal(SIGALRM, alarmHandler);
    while (alarmCount != 0 && state != STOP)
    {
        if (alarmEnabled == FALSE)
        {
            write(fd, frame, frameSize);
            alarm(timeout);
            alarmEnabled = TRUE;
        }

        C = checkControlFrame(fd);

        if (C == C_RR(0) || C == C_RR(1))
        {
            state = STOP;
            tramaT = (tramaT + 1) % 2;
        }
    }

    free(frame);
    return (state == STOP) ? frameSize : -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet)
{
    int framePos = 0;
    unsigned char byte, CByte;
    LinkLayerState state = START;

    while (1)
    {
        if (read(fd, byte, 1) > 0)
        {
            switch (state)
            {
            case START:
                if (byte == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
                if (byte == A_RE)
                    state = A_RCV;
                else
                    state = START;
                break;
            case A_RCV:
                if (byte == C_I(0) || byte == C_I(1))
                {
                    CByte = byte;
                    state = C_RCV;
                }
                else if (byte == C_DISC)
                    return -2;
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case C_RCV:
                if (byte == (A_RE ^ CByte))
                    state = READING_DATA;
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case READING_DATA:
                if (byte == ESC)
                    state = FOUND_ESC;
                else if (byte == FLAG)
                {
                    unsigned char BBC2 = packet[--i];
                    packet[i] = '\0';

                    unsigned char checkBBC2 = packet[0];
                    for (int j = 1; j < i; j++)
                        checkBBC2 ^= packet[j];

                    if (BBC2 == checkBBC2)
                    {
                        tramaR = (tramaR + 1) % 2;
                        sendSupervisionFrame(fd, A_RE, C_RR(tramaRx));
                        return i;
                    }
                    else
                    {
                        sendSupervisionFrame(fd, A_RE, C_REJ(tramaRx));
                        return -1;
                    }
                }
                else
                    packet[framePos++] = byte;
                break;
            case FOUND_ESC:
                if (byte == ESC_1)
                    state = AFTER_ESC;
                else
                {
                    packet[i++] = ESC;
                    packet[i++] = byte;
                    state = READING_DATA;
                }
                break;
            case AFTER_ESC:
                state = READING_DATA if (byte == ESC_2) packet[i++] = FLAG;
                else
                {
                    packet[i++] = ESC;
                    packet[i++] = byte;
                }
                break;
            default:
                break;
            }
        }
    }
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd, LinkLayerRole role)
{
    switch (role)
    {
        case LlTx:
        {
            unsigned char byte;
            LinkLayerState state = START;
            alarmCount = nRetransmissions;

            (void)signal(SIGALRM, alarmHandler);
            while (alarmCount != 0 && state != STOP)
            {
                if (alarmEnabled == FALSE)
                {
                    sendSupervisionFrame(fd, A_ER, C_DISC);
                    alarm(timeout);
                    alarmEnabled = TRUE;
                }

                if (read(fd, byte, 1) > 0)
                {
                    switch (state)
                    {
                    case START:
                        if (byte == FLAG)
                            state = FLAG_RCV;
                        break;
                    case FLAG_RCV:
                        if (byte == A_RE)
                            state = A_RCV;
                        else
                            state = START;
                        break;
                    case A_RCV:
                        if (byte == C_DISC)
                            state = C_RCV;
                        else if (byte == FLAG)
                            state = FLAG_RCV;
                        else
                            state = START;
                        break;
                    case C_RCV:
                        if (byte == (A_RE ^ C_DISC))
                            state = BCC1_OK;
                        else if (byte == FLAG)
                            state = FLAG_RCV;
                        else
                            state = START;
                        break;
                    case BCC1_OK:
                        if (byte == FLAG)
                        {
                            alarm(0);
                            state = STOP; // Received DISC!
                        }
                        else
                            state = START;
                        break;
                    default:
                        break;
                    }
                }
            }

            if (state != STOP) return -1;
            sendSupervisionFrame(fd, A_ER, C_UA);

            sleep(1);

            // Restore the old port settings
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
            {
                perror("tcsetattr");
                exit(-1);
            }

            close(fd);

            return 0;
        }

        case LlTx:
        {
            unsigned char byte;
            LinkLayerState state = START;
            alarmCount = nRetransmissions;

            (void)signal(SIGALRM, alarmHandler);
            while (alarmCount != 0 && state != STOP)
            {
                if (alarmEnabled == FALSE)
                {
                    sendSupervisionFrame(fd, A_ER, C_DISC);
                    alarm(timeout);
                    alarmEnabled = TRUE;
                }

                if (read(fd, byte, 1) > 0)
                {
                    switch (state)
                    {
                    case START:
                        if (byte == FLAG)
                            state = FLAG_RCV;
                        break;
                    case FLAG_RCV:
                        if (byte == A_RE)
                            state = A_RCV;
                        else
                            state = START;
                        break;
                    case A_RCV:
                        if (byte == C_DISC)
                            state = C_RCV;
                        else if (byte == FLAG)
                            state = FLAG_RCV;
                        else
                            state = START;
                        break;
                    case C_RCV:
                        if (byte == (A_RE ^ C_DISC))
                            state = BCC1_OK;
                        else if (byte == FLAG)
                            state = FLAG_RCV;
                        else
                            state = START;
                        break;
                    case BCC1_OK:
                        if (byte == FLAG)
                        {
                            alarm(0);
                            state = STOP; // Received DISC!
                        }
                        else
                            state = START;
                        break;
                    default:
                        break;
                    }
                }
            }

            if (state != STOP) return -1;
            sendSupervisionFrame(fd, A_ER, C_UA);

            sleep(1);

            // Restore the old port settings
            if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
            {
                perror("tcsetattr");
                exit(-1);
            }

            close(fd);

            return 0;
        }

        default:
            break;
    }

    return 0;
}
