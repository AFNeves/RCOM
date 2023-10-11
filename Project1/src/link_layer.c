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
                if (byte == A_RE)
                    state = A_RCV;
                else if (byte == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
                if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) || byte == C_DISC) {
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
                    state = STOP; // Received Control Frame!
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
    //TODO

    return 0;
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
    if (role == LlRx) {
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
