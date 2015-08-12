#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <wiringPi.h>

#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"

#define true 1
#define false 0

#define PORT 7854


typedef struct {
	int pin1;
	int pin2;
	int speed;
}motor;

// Motors
motor motor_L, motor_R;

// Error function
void error(char * msg) {
	fprintf(stderr, "###################\n## A fuck was given (i.e. error):\n## ");
	perror(msg);
	fprintf(stderr, "###################\n");
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

void finish() {
	// Stop motors
	setMotor(&motor_L, 0);
	setMotor(&motor_R, 0);

	tcp_CloseServer();

	exit(0);
}	

void onSIGTERM(int sig, siginfo_t *siginfo, void *context) {
	finish();
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

	// Handle SIGTERM
	struct sigaction sigAction;
	memset (&sigAction, '\0', sizeof(sigAction));
	sigAction.sa_flags = SA_SIGINFO;
	sigAction.sa_sigaction = &onSIGTERM;
	if (sigaction(SIGTERM, &sigAction, NULL) < 0)
		error("Function sigaction(SIGTERM) failed");
	if (sigaction(SIGINT, &sigAction, NULL) < 0)
		error("Function sigaction(SIGINT) failed");
}

void testMotors() {
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

	tcp_ConnectToClient(PORT);
	tcp_CloseServer();
	
	return 0;
}

