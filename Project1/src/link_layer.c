// Link layer protocol implementation

#include "../include/link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;
int timeout = 0;
int nRetransmissions = 0;
unsigned char tramaT = 0;
unsigned char tramaR = 1;

struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount--;

    printf("--- Alarm #%d ---\n", alarmCount);
}

int sendSupervisionFrame(int serialPort, unsigned char A, unsigned char C)
{
    unsigned char frame[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(serialPort, frame, 5);
}

unsigned char checkControlFrame(int fd)
{
    unsigned char byte, C;
    LinkLayerState state = START;

    while (state != STOP && alarmEnabled == TRUE)
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
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
                if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC)
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
                if (byte == (A_RE ^ C))
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
                    state = STOP; // Received Control Frame!
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

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    unsigned char byte;
    LinkLayerState state = START;
    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    alarmCount = nRetransmissions;

    int fd = openSerialPort(connectionParameters.serialPort, &oldtio, &newtio);
    if (fd < 0) return -1;

    switch (connectionParameters.role)
    {

    case LlTx:
    {

        (void)signal(SIGALRM, alarmHandler);
        while (alarmCount != 0 && state != STOP)
        {
            if (alarmEnabled == FALSE)
            {
                sendSupervisionFrame(fd, A_ER, C_SET);
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
                    else if (byte == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case A_RCV:
                    if (byte == C_UA)
                        state = C_RCV;
                    else if (byte == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case C_RCV:
                    if (byte == (A_RE ^ C_UA))
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
                        state = STOP; // Received UA!
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
        break;
    }

    case LlRx:
    {

        while (state != STOP)
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
                    if (byte == A_ER)
                        state = A_RCV;
                    else if (byte == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case A_RCV:
                    if (byte == C_SET)
                        state = C_RCV;
                    else if (byte == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case C_RCV:
                    if (byte == (A_ER ^ C_SET))
                        state = BCC1_OK;
                    else if (byte == FLAG)
                        state = FLAG_RCV;
                    else
                        state = START;
                    break;
                case BCC1_OK:
                    if (byte == FLAG)
                        state = STOP; // Received SET UP!
                    else
                        state = START;
                    break;
                default:
                    break;
                }
            }
        }

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
    unsigned char *frame = (unsigned char *) malloc(frameSize);

    frame[0] = FLAG;
    frame[1] = A_ER;
    frame[2] = C_I(tramaT);
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame + 4, buf, bufSize);

    unsigned char BCC2 = buf[0];
    for (int i = 1 ; i < bufSize ; i++) BCC2 ^= buf[i];

    for (int i = 4 ; i < frameSize - 1; i++)
    {
        if (frame[i] == FLAG)
        {
            frameSize += 2;
            frame = realloc(frame,frameSize);

            memmove(frame + i + 2, frame + i, frameSize - i - 2);

            frame[i++] = ESC;
            frame[i++] = 0x5D;
            frame[i++] = 0x5E;
        }
        else if (frame[i] == ESC)
        {
            frame = realloc(frame,++frameSize);

            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = ESC;
            frame[i++] = 0x5D;
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
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd, LinkLayerRole role)
{
    if (role == LlRx)
    {
        // Restore the old port settings
        if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
        {
            perror("tcsetattr");
            exit(-1);
        }

        close(fd);
        return 0;
    }

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
                else if (byte == FLAG)
                    state = FLAG_RCV;
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
