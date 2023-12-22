// FTP Program Header

#ifndef _CLIENT_H_
#define _CLIENT_H_

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

// ----- MACROS -----

/* LENGTHS */
#define MAX_LENGTH 500
#define URL_LENGTH 150

/* SERVER RESPONSES */
#define READY_AUTH 220
#define READY_PASS 331
#define LOGIN_SUCCESS 230
#define PASSIVE_MODE 227
#define READY_TRANSFER 150
#define TRANSFER_COMPLETE 226
#define GOODBYE 221

/* REGULAR EXPRESSIONS */
#define DEFAULT_HOST "%*[^/]//%[^/]"
#define HOST_REGEX "%*[^/]//%*[^@]@%[^/]"
#define USER_REGEX "%*[^/]//%[^:/]"
#define PASS_REGEX "%*[^/]//%*[^:]:%[^@\n$]"
#define RESOURCE_REGEX "%*[^/]//%*[^/]/%s"
#define PASSIVE_MODE_REGEX "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

// ----- DATA STRUCTURES -----

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

// ----- AUX FUNCTIONS -----

// Parses the given URL into its various components.
int parseToURL(char *input, URL *url);

// Creates a socket and connects to the given IP and port.
int createSocket(char *IP, int port);

// Reads the response from the server, using a state machine.
int readResponse(int socket, char *buffer);

// Authenticates the user with the given credentials.
int authConn(int socket, char *user, char *pass);

// Enters passive mode by sending the PASV command.
int passiveMode(int socket, char* IP, int *port);

// Requests a resource from the server.
int requestResource(int socket, char *resource);

// Downloads a file from the server.
int getResource(int socketA, int socketB, char *filename);

// Closes all connection and sockets.
int closeConn(int socketA, int socketB);

#endif // _CLIENT_H_
