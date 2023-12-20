#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* FTP SETTINGS */
#define FTP_PORT 21
#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

/* LOGIN CREDENTIALS */
#define RCOM "rcom"
#define DEFAULT_USER "anonymous"
#define DEFAULT_PASS "password"

/* SERVER RESPONSES */
#define READY_AUTH 210
#define READY_PASS 331
#define LOGIN_SUCCESS 230
#define PASSIVE_MODE 227
#define READY_TRANSFER 150
#define TRANSFER_COMPLETE 226
#define GOODBYE 221

/* URL PARAMETERS LENGTH */
#define MAX_LENGTH 300
#define URL_PARAM_LENGTH 100

typedef struct
{
    char ip[URL_PARAM_LENGTH];
    char host[URL_PARAM_LENGTH];
    char user[URL_PARAM_LENGTH];
    char pass[URL_PARAM_LENGTH];
    char resource[URL_PARAM_LENGTH];
    char file[URL_PARAM_LENGTH];
} URL;

typedef enum
{
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

/*
int parse(char *input, URL *url);

int createSocket(char *ip, int port);

int authConn(const int socket, const char *user, const char *pass);

int readResponse(const int socket, char *buffer);

int passiveMode(const int socket, char* ip, int *port);

int requestResource(const int socket, char *resource);

int getResource(const int socketA, const int socketB, char *filename);

int closeConnection(const int socketA, const int socketB);
*/
