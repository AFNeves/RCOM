#include <stdio.h>
#include <netdb.h>
#include <regex.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* REGULAR EXPRESSIONS */
#define DEFAULT_HOST "%*[^/]//%[^/]"
#define HOST_REGEX "%*[^/]//%*[^@]@%[^/]"
#define USER_REGEX "%*[^/]//%[^:/]"
#define PASS_REGEX "%*[^/]//%*[^:]:%[^@\n$]"
#define RESOURCE_REGEX "%*[^/]//%*[^/]/%s"
#define PASSIVE_MODE_REGEX "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

/* SERVER RESPONSES */
#define READY_AUTH 220
#define READY_PASS 331
#define LOGIN_SUCCESS 230
#define PASSIVE_MODE 227
#define READY_TRANSFER 150
#define TRANSFER_COMPLETE 226
#define GOODBYE 221

/* LENGTHS */
#define MAX_LENGTH 500
#define URL_LENGTH 150

typedef struct
{
    char ip[URL_LENGTH];
    char host[URL_LENGTH];
    char user[URL_LENGTH];
    char pass[URL_LENGTH];
    char resource[URL_LENGTH];
    char file[URL_LENGTH];
} URL;

typedef enum
{
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

int parseToURL(char *input, URL *url);

int createSocket(char *IP, int port);

int readResponse(int socket, char *buffer);

int authConn(int socket, char *user, char *pass);

int passiveMode(int socket, char* IP, int *port);

int requestResource(int socket, char *resource);

int getResource(int socketA, int socketB, char *filename);

int closeConn(int socketA, int socketB);
