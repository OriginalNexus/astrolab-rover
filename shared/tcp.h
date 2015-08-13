#ifndef TCP_HEADER
#define TCP_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Creates a new server and listens on the port specified until a client connects.
// Returns 0 on success, -1 on fail.
int tcp_ConnectToClient(int port);

// Sends the message to the client.
// Returns 0 on success, -1 on fail.
int tcp_WriteToClient(char * msg);

// Reads the message from the client.
// Up to 'count' characters are read, 'msg' must be big enough to contain the message read and the null character.
// Returns 0 on success, -1 on fail.
int tcp_ReadFromClient(char * msg, int count);

// Closes server sockets.
// Returns 0 on success, -1 on fail.
int tcp_CloseServer();


// Connect to the specified server on the specified port.
// Returns 0 on success, -1 on fail.
int tcp_ConnectToServer(char * name, int port);

// Sends the message to the server.
// Returns 0 on success, -1 on fail.
int tcp_WriteToServer(char * msg);

// Reads the message from the server.
// Up to 'count' characters are read, 'msg' must be big enough to contain the message read and the null character.
// Returns 0 on success, -1 on fail.
int tcp_ReadFromServer(char * msg, int count);

// Closes client socket.
// Returns 0 on success, -1 on fail.
int tcp_CloseClient();


// Set log level: 0 = no log; 1 = client/server dialog; 2 = client/server dialog + errors
void tcp_SetLogLevel(int level);

#ifdef TCP_IMPLEMENTATION

static int tcpp_servSockDesc = -1, tcpp_servSockDescNew = -1;
static int tcpp_cliSockDesc = -1;
// 0 = no log; 1 = client/server dialog; 2 = client/server dialog + errors
static int tcpp_logLev = 1;

// perror function. Always retruns -1
int tcpp_perror(char * msg) {
	if (tcpp_logLev >= 2) {
		fprintf(stderr, "### TCP Error: ");
		perror(msg);
	}
	return -1;
}

// error function. Always retruns -1
int tcpp_error(char * msg) {
	if (tcpp_logLev >= 2) {
		fprintf(stderr, "### TCP Error: %s\n", msg);
	}
	return -1;
}

int tcp_ConnectToClient(int port) {
	if (tcpp_servSockDescNew >= 0) return tcpp_error("Already connected to a client. Close it first.");
	// Open socket if needed
	if (tcpp_servSockDesc < 0) {
		tcpp_servSockDesc = socket(AF_INET, SOCK_STREAM, 0);
		if (tcpp_servSockDesc < 0)
			return tcpp_perror("Failed to open socket");
		int enable = 1;
		if (setsockopt(tcpp_servSockDesc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			return tcpp_perror("Failed to setsockopt(SO_REUSEADDR)");
	}

	// Create server address
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(port);

	// Bind the address to the socket
	if (bind(tcpp_servSockDesc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return tcpp_perror("Failed to bind");

	// Listen for client
	if (listen(tcpp_servSockDesc, 5) < 0)
		return tcpp_perror("Failed to listen");
	printf("Listening on port %d...\n", port);

	// Accept clients
	struct sockaddr_in cli_addr;
	socklen_t cliLen = sizeof(cli_addr);
	while (1) {
		tcpp_servSockDescNew = accept(tcpp_servSockDesc, (struct sockaddr *) &cli_addr, &cliLen);
		if (tcpp_servSockDescNew < 0)
			tcpp_perror("Error accepting client");
		else
			break;
	}

	printf("Connected to client.\n");

	return 0;
}

int tcp_WriteToClient(char * msg) {
	if (tcpp_servSockDescNew < 0) {
		return tcpp_error("Not connected to a client. Connect first.");
	}
	if (strlen(msg) == 0) return tcpp_error("Message mustn't be empty");
	int n = write(tcpp_servSockDescNew, msg, strlen(msg) * sizeof(char));
	if (n < 0) {
		return tcpp_perror("Failed to write to client");
	}
	else if (n < strlen(msg) * sizeof(char)) {
		char * temp = (char *) malloc(256 * sizeof(char));
		sprintf(temp, "Lost %d bytes when writting to client", strlen(msg) * sizeof(char) - n);
		tcpp_error(temp);
		free(temp);
		return -1;
	}
	if (tcpp_logLev) printf("SERVER: %s\n", msg);
	return 0;
}

int tcp_ReadFromClient(char * msg, int count) {
	if (count <= 0) return tcpp_error("'count' must be greater than 0");
	if (tcpp_servSockDescNew < 0) return tcpp_error("Not connected to a client. Connect first.");
	int n = read(tcpp_servSockDescNew, msg, count * sizeof(char));
	if (n < 0) {
		return tcpp_perror("Failed to read from client");
	}
	else if (n == 0){
		return tcpp_error("Nothing read from client");
	}
	msg[n / sizeof(char)] = '\0';
	if (tcpp_logLev) printf("CLIENT: %s\n", msg);
	return 0;
}

int tcp_CloseServer() {
	if (tcpp_servSockDesc >= 0) {
		if (close(tcpp_servSockDesc) < 0) {
			return tcpp_perror("Failed to close descriptor");
		}
		tcpp_servSockDesc = -1;
	}
	if (tcpp_servSockDescNew >= 0) {
		if (close(tcpp_servSockDescNew) < 0) {
			return tcpp_perror("Failed to close descriptor");
		}
		tcpp_servSockDescNew = -1;
	}
	return 0;
}


int tcp_ConnectToServer(char * name, int port) {
	// Open socket if needed
	if (tcpp_cliSockDesc < 0) {
		tcpp_cliSockDesc = socket(AF_INET, SOCK_STREAM, 0);
		if (tcpp_cliSockDesc < 0)
			return tcpp_perror("Failed to open socket");
	}

	// Get server
	struct hostent * host;

	host = gethostbyname(name);
	if (host == NULL) {
		return tcpp_error("Host not found");
	}

	// Create server address from host
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	memcpy(&serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

	// Connect to server
	if (connect(tcpp_cliSockDesc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		return tcpp_perror("Failed to connect to server");
	}

	printf("Connected to server %s.\n", name);
	return 0;
}

int tcp_WriteToServer(char * msg) {
	if (tcpp_cliSockDesc < 0) {
		return tcpp_error("Not connected to a server. Connect first.");
	}
	if (strlen(msg) == 0) return tcpp_error("Message mustn't be empty");
	int n = write(tcpp_cliSockDesc, msg, strlen(msg) * sizeof(char));
	if (n < 0) {
		return tcpp_perror("Failed to write to server");
	}
	else if (n < strlen(msg) * sizeof(char)) {
		char * temp = (char *) malloc(256 * sizeof(char));
		sprintf(temp, "Lost %d bytes when writting to server", strlen(msg) * sizeof(char) - n);
		tcpp_error(temp);
		free(temp);
		return -1;
	}

	if (tcpp_logLev) printf("CLIENT: %s\n", msg);
	return 0;
}

int tcp_ReadFromServer(char * msg, int count) {
	if (count <= 0) return tcpp_error("'count' must be greater than 0");
	if (tcpp_cliSockDesc < 0) return tcpp_error("Not connected to a server. Connect first.");
	int n = read(tcpp_cliSockDesc, msg, count * sizeof(char));
	if (n < 0) {
		return tcpp_perror("Failed to read from server");
	}
	else if (n == 0){
		return tcpp_error("Nothing read from server");
	}
	msg[n / sizeof(char)] = '\0';

	if (tcpp_logLev) printf("SERVER: %s\n", msg);
	return 0;
}

int tcp_CloseClient() {
	if (tcpp_cliSockDesc >= 0) {
		if (close(tcpp_cliSockDesc) < 0) {
			return tcpp_perror("Failed to close descriptor");
		}
		tcpp_cliSockDesc = -1;
	}
	return 0;
}

void tcp_SetLogLevel(int level){
	if (level <= 0) {
		tcpp_logLev = 0;
	}
	else if (level >= 2) {
		tcpp_logLev = 2;
	}
	else {
		tcpp_logLev = 1;
	}
}

#endif // TCP_IMPLEMENTATION

#endif // !TCP_HEADER
