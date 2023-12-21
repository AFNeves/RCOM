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

/* FTP SETTINGS */
#define FTP_PORT 21

/* LOGIN CREDENTIALS */
#define RCOM "rcom"
#define DEFAULT_USER "anonymous"
#define DEFAULT_PASS "password"

/* REGULAR EXPRESSIONS */
#define AT              "@"
#define BAR             "/"
#define HOST_REGEX      "%*[^/]//%[^/]"
#define HOST_AT_REGEX   "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE_REGEX  "%*[^/]//%*[^/]/%s"
#define USER_REGEX      "%*[^/]//%[^:/]"
#define PASS_REGEX      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE_REGEX  "%d"
#define PASSIVE_REGEX   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

/* SERVER RESPONSES */
#define READY_AUTH 210
#define READY_PASS 331
#define LOGIN_SUCCESS 230
#define PASSIVE_MODE 227
#define READY_TRANSFER 150
#define TRANSFER_COMPLETE 226
#define GOODBYE 221

/* LENGTHS */
#define MAX_LENGTH 300
#define URL_LENGTH 100

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

int getIP(char *hostname, struct hostent **h);

int parseToURL(char *input, URL *url);

int createSocket(char *ip, int port);

int authConn(int socket, char *user, char *pass);

int readResponse(const int socket, char *buffer);

int passiveMode(const int socket, char* ip, int *port);

int requestResource(const int socket, char *resource);

int getResource(const int socketA, const int socketB, char *filename);

int closeConnection(const int socketA, const int socketB);
