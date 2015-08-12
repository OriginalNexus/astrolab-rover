#ifndef TCP_HEADER
#define TCP_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define true 1
#define false 0

// This function creates a new server and listens on the port specified until a client connects
// Any error will stop the program
void tcp_ConnectToClient(int port);

// Closes server socket
void tcp_CloseServer();

// Tries to connect to the specified server on the specified port
// Returns 0 if the connection was successfull, -1 if not.
// Any error will stop the program
int tcp_ConnectToServer(char * name, int port);

// Closes client socket
void tcp_CloseClient();

#ifdef TCP_IMPLEMENTATION

static int tcpp_servSockDesc = -1, tcpp_servSockDescNew = -1;
static int tcpp_cliSockDesc = -1;

// Error function
void tcpp_error(char * msg, int ignore) {
	fprintf(stderr, "########## TCP ##########\n## A fuck was given (i.e. error):\n## ");
	perror(msg);
	fprintf(stderr, "#########################\n");
	if (!ignore)
		exit(1);
}

void tcp_ConnectToClient(int port) {
	if (tcpp_servSockDescNew >= 0) return;
	// Open socket if needed
	if (tcpp_servSockDesc < 0) {
		tcpp_servSockDesc = socket(AF_INET, SOCK_STREAM, 0);
		if (tcpp_servSockDesc < 0) 
			tcpp_error("Failed to open socket", false);
		int enable = 1;
		if (setsockopt(tcpp_servSockDesc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			tcpp_error("Function setsockopt(SO_REUSEADDR) failed", false);
	}

	// Create server address
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
   	serv_addr.sin_port = htons(port);

	// Bind the address to the socket
	if (bind(tcpp_servSockDesc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		tcpp_error("Binding failed", false);

	// Listen for client
	printf("Listening on port %d...\n", port);
	listen(tcpp_servSockDesc, 5);

	// Accept clients
	struct sockaddr_in cli_addr;
	socklen_t cliLen = sizeof(cli_addr);
	while (1) {
		tcpp_servSockDescNew = accept(tcpp_servSockDesc, (struct sockaddr *) &cli_addr, &cliLen);
		if (tcpp_servSockDescNew < 0) 
			tcpp_error("Error accepting client", true);
		else
			break;
	}

	printf("Connected to client.\n");
}

void tcp_CloseServer() {
	if (tcpp_servSockDesc >= 0) {
		close(tcpp_servSockDesc);
		tcpp_servSockDesc = -1;
	}
	if (tcpp_servSockDescNew >= 0) {
		close(tcpp_servSockDescNew);
		tcpp_servSockDescNew = -1;
	}
}

int tcp_ConnectToServer(char * name, int port) {
	// Open socket if needed
	if (tcpp_cliSockDesc < 0) {
		tcpp_cliSockDesc = socket(AF_INET, SOCK_STREAM, 0);
		if (tcpp_cliSockDesc < 0)
			tcpp_error("Failed to open socket", false);
	}

	// Get server
	struct hostent * host;

	host = gethostbyname(name);
	if (host == NULL) {
		printf("Host not found.\n");
		return -1;
	}
	
	// Create server address from host
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	memcpy(&serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

	// Connect to server
	if (connect(tcpp_cliSockDesc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		tcpp_error("Could not connect to server", true);
		return -1;
	}

	printf("Connected to server %s.\n", name);
	return 0;
}

void tcp_CloseClient() {
	if (tcpp_cliSockDesc >= 0) {
		close(tcpp_cliSockDesc);
		tcpp_cliSockDesc = -1;
	}
}

#endif // TCP_IMPLEMENTATION

#endif // !TCP_HEADER
