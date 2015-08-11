// Enter this in vim to configure make
// map <F9> :!ssh 192.168.1.12 "cd ~/Github/astrolab-rover/raspberry-pi/ && make"<CR>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define true 1
#define false 0

#define PORT 1337


typedef struct {
	int pin1;
	int pin2;
	int speed;
}motor;

// Motors
motor motor_L, motor_R;

// Server variables
int mSockDesc = -1, sockDesc = -1;
struct sockaddr_in server_addr, client_addr;

// Error function
void error(char * msg) {
	fprintf(stderr, "###################\n## A fuck was given (i.e. error):\n## ");
	perror(msg);
	exit(1);
}

motor newMotor(int pin1, int pin2) {
	motor m;
	m.pin1 = pin1;
	m.pin2 = pin2;
	return m;
}

// Starts/stops a motor.
// speed > 0, the motor spins forward
// speed < 0, the motor spins backward
// speed = 0, the motor stops.
void setMotor(motor * m, int speed) {
	if (speed > 0) {
		digitalWrite(m->pin1, HIGH);
		digitalWrite(m->pin2, LOW);	
	}
	else if (speed < 0) {
		digitalWrite(m->pin1, LOW);
		digitalWrite(m->pin2, HIGH);
	}
	else {
		digitalWrite(m->pin1, LOW);
		digitalWrite(m->pin2, LOW);
	}
	m->speed = speed;
}

void init() {
	// Setting up pins
	wiringPiSetup();
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);

	// Create motors
	motor_L = newMotor(0, 1);
	motor_R = newMotor(2, 3);

	// Setup server
	// Open socket
	mSockDesc = socket(AF_INET, SOCK_STREAM, 0);
	if (mSockDesc < 0)
		  error("Failed to open socket");

	// Create server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
   	server_addr.sin_port = htons(PORT);

	// Bind the address to the socket
	if (bind(mSockDesc, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		error("Binding failed");
}

void testMotors() {
	// Test motors
	printf("Testing motors...\n");
	printf("Forward");
	setMotor(&motor_L, 1);
	setMotor(&motor_R, 1);
	getchar();

	setMotor(&motor_L, 0);
	setMotor(&motor_R, 0);

	sleep(1);

	printf("Backward");
	setMotor(&motor_L, -1);
	setMotor(&motor_R, -1);
	getchar();

	setMotor(&motor_L, 0);
	setMotor(&motor_R, 0);

	printf("Done.\n");
}

int main(int argc, char * argv[]) {
	printf("=======================================\n");
	printf("======== AstroLab Rover Server ========\n");
	printf("=======================================\n");

	init();
	
	testMotors();

	// Listen for client
	printf("Listening on port %d...\n", PORT);
	listen(mSockDesc, 5);

	// Accept client
	socklen_t clientLen = sizeof(client_addr);
	sockDesc = accept(mSockDesc, (struct sockaddr *) &client_addr, &clientLen);
	if (sockDesc < 0) 
		error("Error accepting client");
	printf("Connected to client %d on port %d.\n", ntohs(client_addr.sin_addr.s_addr), PORT);

	// Close sockets
	close(sockDesc);
	close(mSockDesc);

	// Terminate program
	return 0;
}

