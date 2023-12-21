#include "client.h"

int getIP(char *hostname, struct hostent **h)
{
	if (hostname == NULL || h == NULL) return -1;

	if ((*h = gethostbyname(hostname)) == NULL)
	{
		herror("gethostbyname()");
		return -1;
	}

	return 0;
}

int parseToURL(char *input, URL *url)
{
	regex_t regex;
	struct hostent *h; 

	regcomp(&regex, BAR, 0);
	if (regexec(&regex, input, 0, NULL, 0)) return -1;

	regcomp(&regex, AT, 0);
	if (regexec(&regex, input, 0, NULL, 0))
	{
		sscanf(input, HOST_AT_REGEX, url->host);
		sscanf(input, USER_REGEX, url->user);
		sscanf(input, PASS_REGEX, url->pass);
	}
	else
	{
		sscanf(input, HOST_REGEX, url->host);
		strcpy(url->user, DEFAULT_USER);
		strcpy(url->pass, DEFAULT_PASS);
	}

	sscanf(input, RESOURCE_REGEX, url->resource);
	if (strlen(url->resource) == 0) return -1;

	strcpy(url->file, strrchr(url->resource, '/') + 1);
	if (strlen(url->file) == 0) return -1;

	if (getIP(url->host, &h) != 0) return -1;
	strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr_list[0])));

	return 0;
}

int createSocket(char *IP, int port)
{
	int fd;
	struct sockaddr_in server_addr;

	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(IP);
	server_addr.sin_port = htons(port);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

	if (connect(fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) return -1;

	return fd;
}

int readResponse(const int socket, char *buffer)
{

	char byte;
	int index = 0, responseCode;
	ResponseState state = START;
	memset(buffer, 0, MAX_LENGTH);

	while (state != END)
	{

		read(socket, &byte, 1);
		switch (state)
		{
		case START:
			if (byte == ' ')
				state = SINGLE;
			else if (byte == '-')
				state = MULTIPLE;
			else if (byte == '\n')
				state = END;
			else
				buffer[index++] = byte;
			break;
		case SINGLE:
			if (byte == '\n')
				state = END;
			else
				buffer[index++] = byte;
			break;
		case MULTIPLE:
			if (byte == '\n')
			{
				memset(buffer, 0, MAX_LENGTH);
				state = START;
				index = 0;
			}
			else
				buffer[index++] = byte;
			break;
		case END:
			break;
		default:
			break;
		}
	}

	sscanf(buffer, RESPCODE_REGEX, &responseCode);
	return responseCode;
}

int authConn(int socket, char *user, char *pass)
{
	char answer[MAX_LENGTH];

	char userCommand[6 + strlen(user)];
	sprintf(userCommand, "user %s\n", user);
	char passCommand[6 + strlen(pass)];
	sprintf(passCommand, "pass %s\n", pass);

	write(socket, userCommand, strlen(userCommand));
	if (readResponse(socket, answer) != READY_PASS) return -1;

	write(socket, passCommand, strlen(passCommand));
	if (readResponse(socket, answer) != LOGIN_SUCCESS) return -1;

	return 0;
}

int passiveMode(int socket, char *IP, int *port)
{
	char answer[MAX_LENGTH];
	int ip1, ip2, ip3, ip4, port1, port2;

	write(socket, "pasv\n", 5);
	if (readResponse(socket, answer) != PASSIVE_MODE) return -1;

	sscanf(answer, PASSIVE_REGEX, &ip1, &ip2, &ip3, &ip4, &port1, &port2);

	sprintf(IP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
	*port = port1 * 256 + port2;

	return 0;
}

int requestResource(int socket, char *resource)
{
	char answer[MAX_LENGTH];

	char command[6 + strlen(resource)];
	sprintf(command, "retr %s\n", resource);

	write(socket, command, sizeof(command));
	if (readResponse(socket, answer) != READY_TRANSFER) return -1;

	return 0;
}

int getResource(int socketA, int socketB, char *filename)
{
	int bytes;
	char answer[MAX_LENGTH], buffer[MAX_LENGTH];

	FILE *fd = fopen(filename, "wb");
	if (fd == NULL) return -1;

	while ((bytes = read(socketB, buffer, MAX_LENGTH)) > 0)
	{
		if (fwrite(buffer, bytes, 1, fd) < 0) return -1;
	}
	
	if (readResponse(socketA, answer) != TRANSFER_COMPLETE) return -1;

	fclose(fd);
	return 0;
}

int closeConn(int socketA, int socketB)
{
	char answer[MAX_LENGTH];

	write(socketA, "quit\n", 5);
	if (readResponse(socketA, answer) != GOODBYE) return -1;

	return close(socketA) || close(socketB);
}

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		printf("Usage: %s <URL>\n", argv[0]);
		exit(-1);
	}

	URL url;
	memset(&url, 0, sizeof(url));
	if (parseURL(argv[1], &url) != 0)
	{
		printf("Parse error!\n");
		exit(-1);
	}

	printf("---------------\nIP Address : % s \nHost : % s\nUser : % s\nPassword : % s\nResource : % s\nFile : % s\n ",url.ip, url.host, url.user, url.pass, url.resource, url.file);

	char answer[MAX_LENGTH];
	int socketA = createSocket(url.ip, FTP_PORT);
	if (socketA < 0 || readResponse(socketA, answer) != SV_READY4AUTH)
	{
		printf("Socket to '%s' and port %d failed\n", url.ip, FTP_PORT);
		exit(-1);
	}

	if (authConn(socketA, url.user, url.password) != SV_LOGINSUCCESS)
	{
		printf("Authentication failed with username = '%s' and password = '%s'.\n", url.user, url.password);
		exit(-1);
	}

	int port;
	char ip[MAX_LENGTH];
	if (passiveMode(socketA, ip, &port) != PASSIVE_MODE)
	{
		printf("Passive mode failed\n");
		exit(-1);
	}

	int socketB = createSocket(ip, port);
	if (socketB < 0)
	{
		printf("Socket to '%s:%d' failed\n", ip, port);
		exit(-1);
	}

	if (requestResource(socketA, url.resource) != SV_READY4TRANSFER)
	{
		printf("Unknown resouce '%s' in '%s:%d'\n", url.resource, ip, port);
		exit(-1);
	}

	if (getResource(socketA, socketB, url.file) != SV_TRANSFER_COMPLETE)
	{
		printf("Error transfering file '%s' from '%s:%d'\n", url.file, ip, port);
		exit(-1);
	}

	if (closeConnection(socketA, socketB) != 0)
	{
		printf("Sockets close error\n");
		exit(-1);
	}

	return 0;
}
