#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()
#include <signal.h>
#include <string.h>

#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"
#define MOTOR_IMPLEMENTATION
#include "motor.h"

#define true 1
#define false 0

// Port used
int port = 0;

// Error function
void m_perror(char * msg) {
	fprintf(stderr, "### Server Error: ");
	perror(msg);
	exit(1);
}

void m_error(char * msg) {
	fprintf(stderr, "### Server Error: %s\n", msg);
	exit(1);
}

void finish() {
	mot_Forward(0);
	tcp_CloseServer();
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

void testMotors() {
	printf("Testing motors...\n");
	printf("Forward");
	mot_Forward(1);
	getchar();

	mot_Forward(0);
	sleep(1);

	printf("Backward");
	mot_Backward(1);
	getchar();

	mot_Forward(0);
	printf("Done.\n");
}

int main(int argc, char * argv[]) {
	printf("=======================================\n");
	printf("======== AstroLab Rover Server ========\n");
	printf("=======================================\n");

	init();

	testMotors();

	tcp_ConnectToClient(port);

	while (1) {
		pck_Client cliPack;
		tcp_ReadFromClient(cliPack);
	}

	finish();

	return 0;
}
