#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"

#define true 1
#define false 0

// Port used
int port = 0;

// Error function
void m_perror(char * msg) {
	fprintf(stderr, "### Client Error: ");
	perror(msg);
	exit(1);
}

void m_error(char * msg) {
	fprintf(stderr, "### Client Error: %s\n", msg);
	exit(1);
}

void finish() {
	tcp_CloseClient();
	exit(0);
}

void termHandler(int sig) {
	finish();
}

void init() {
	// Set TCP log level
	tcp_SetLogLevel(2);

	// Get port
	FILE * fp = fopen("../shared/port", "rt");
	if (fp == NULL) {
		m_error("Failed to open ../shared/port");
	}
	if (fscanf(fp, "%d", &port) < 1) {
		m_error("Failed to read port from file ../shared/port");
	}
	fclose(fp);

	// Handle SIGTERM and SIGINT
	struct sigaction sigAction;
	memset (&sigAction, '\0', sizeof(sigAction));
	sigAction.sa_handler = &termHandler;
	if (sigaction(SIGTERM, &sigAction, NULL) < 0)
		m_perror("Function sigaction(SIGTERM) failed");
	if (sigaction(SIGINT, &sigAction, NULL) < 0)
		m_perror("Function sigaction(SIGINT) failed");
}

int main(int argc, char * argv[]) {
	printf("=======================================\n");
	printf("======== AstroLab Rover Client ========\n");
	printf("=======================================\n");

	init();

	// Connect to server
	char * hostName = (char *) malloc(32 * sizeof(char));
	while (1) {
		printf("Enter server name: ");
		fflush (stdout);
		scanf("%31s", hostName);
		if (tcp_ConnectToServer(hostName, port) == 0)
			break;
	}

	tcp_WriteToServer("Sup server?");
	char * response = (char *) malloc(256 * sizeof(char));
	tcp_ReadFromServer(response, 255);

	finish();

	return 0;
}
