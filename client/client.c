#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"

#define true 1
#define false 0

#define PORT 7854

// Error function
void error(char * msg) {
	fprintf(stderr, "###################\n## A fuck was given (i.e. error):\n## ");	
	perror(msg);
	fprintf(stderr, "###################\n");
	exit(1);
}

void init() {

}

void finish() {
	tcp_CloseClient();
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
		if (tcp_ConnectToServer(hostName, PORT) == 0)
			break;	
	}

	finish();
	return 0;
}
