#include "client.h"

int parseToURL(char *input, URL *url)
{
	regex_t regex;
	struct hostent *h; 

	regcomp(&regex, "/", 0);
	if (regexec(&regex, input, 0, NULL, 0)) return -1;

	regcomp(&regex, "@", 0);
	if (regexec(&regex, input, 0, NULL, 0) == 0)
	{
		sscanf(input, HOST_REGEX, url->host);
		sscanf(input, USER_REGEX, url->user);
		sscanf(input, PASS_REGEX, url->pass);
	}
	else
	{
		sscanf(input, DEFAULT_HOST, url->host);
		strcpy(url->user, "anonymous");
		strcpy(url->pass, "password");
	}

	sscanf(input, RESOURCE_REGEX, url->resource);
	if (strlen(url->resource) == 0) return -1;

	strcpy(url->file, strrchr(input, '/') + 1);
	if (strlen(url->file) == 0) return -1;

	if ((h = gethostbyname(url->host)) == NULL) return -1;
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

int readResponse(int socket, char *buffer)
{
	char byte;
	int bufferPos = 0, answerCode;
	ResponseState state = START;

	while (state != END)
	{
		if (read(socket, &byte, 1) > 0)
		{
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
					buffer[bufferPos++] = byte;
				break;

			case SINGLE:
				if (byte == '\n')
					state = END;
				else
					buffer[bufferPos++] = byte;
				break;

			case MULTIPLE:
				if (byte == '\n')
				{
					bufferPos = 0;
					memset(buffer, 0, MAX_LENGTH);
					state = START;
				}
				else
					buffer[bufferPos++] = byte;
				break;

			default:
				break;
			}
		}
	}

	sscanf(buffer, "%d", &answerCode);

	return answerCode;
}

int authConn(int socket, char *user, char *pass)
{
	char answer[MAX_LENGTH];

	char userCommand[6 + strlen(user)];
	sprintf(userCommand, "USER %s\n", user);
	char passCommand[6 + strlen(pass)];
	sprintf(passCommand, "PASS %s\n", pass);

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

	write(socket, "PASV\n", 5);
	if (readResponse(socket, answer) != PASSIVE_MODE) return -1;

	sscanf(answer, PASSIVE_MODE_REGEX, &ip1, &ip2, &ip3, &ip4, &port1, &port2);

	sprintf(IP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
	*port = port1 * 256 + port2;

	return 0;
}

int requestResource(int socket, char *resource)
{
	char answer[MAX_LENGTH];

	char requestCommand[6 + strlen(resource)];
	sprintf(requestCommand, "RETR %s\n", resource);

	write(socket, requestCommand, sizeof(requestCommand));
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

	write(socketA, "QUIT\n", 5);
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
	if (parseToURL(argv[1], &url) != 0)
	{
		printf("Parse Error!\n");
		exit(-1);
	}

	printf("\n------------------------------\nIP Address : %s \nHost : %s\nUser : %s\nPassword : %s\nResource : %s\nFile : %s\n------------------------------\n\n",url.ip, url.host, url.user, url.pass, url.resource, url.file);

	char answer[MAX_LENGTH];

	int socketA = createSocket(url.ip, 21);
	if (socketA < 0 || readResponse(socketA, answer) != READY_AUTH)
	{
		printf("Socket A Error!\n");
		exit(-1);
	}

	if (authConn(socketA, url.user, url.pass))
	{
		printf("Authentication Error!\n");
		exit(-1);
	}

	int port;
	char ip[MAX_LENGTH];
	if (passiveMode(socketA, ip, &port))
	{
		printf("Passive Mode Error!\n");
		exit(-1);
	}

	int socketB = createSocket(ip, port);
	if (socketB < 0)
	{
		printf("Socket B Error!\n");
		exit(-1);
	}

	if (requestResource(socketA, url.resource))
	{
		printf("Unknown Resource!\n");
		exit(-1);
	}

	if (getResource(socketA, socketB, url.file))
	{
		printf("Download Error!\n");
		exit(-1);
	}

	if (closeConn(socketA, socketB) != 0)
	{
		printf("Sockets Close Error!\n");
		exit(-1);
	}

	printf("Download Complete!\n\n");

	return 0;
}
