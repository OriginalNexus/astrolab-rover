#ifndef MOTOR_HEADER
#define MOTOR_HEADER

// Start moving forward with the specified speed. Call with speed = 0 to stop. Speed must be > 0
void mot_Forward(int speed);

// Start moving backward with the specified speed. Call with speed = 0 to stop. Speed must be > 0
void mot_Backward(int speed);

// Start turning left with the specified speed. Call with speed = 0 to stop. Speed must be > 0
void mot_Left(int speed);

// Start turning right with the specified speed. Call with speed = 0 to stop. Speed must be > 0
void mot_Right(int speed);


#ifdef MOTOR_IMPLEMENTATION

#include <wiringPi.h>

typedef enum {DC, STEP, SERVO} motp_type;

typedef struct {
	int pin1;
	int pin2;
	motp_type type;
	int speed;
} motp_Motor;

// Motors
static motp_Motor motp_L, motp_R;

static int isInit = 0;

motp_Motor motp_newMotor(motp_type type, int pin1, int pin2) {
	motp_Motor m;
	m.pin1 = pin1;
	m.pin2 = pin2;
	m.type = type;
	m.speed = 0;
	return m;
}

// Starts/stops a motor.
// speed > 0, the motor spins forward
// speed < 0, the motor spins backward
// speed = 0, the motor stops.
void motp_setMotor(motp_Motor * m, int speed) {
	switch (m->type) {
		case DC:
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
			break;
		case STEP:
			// Soon
			break;
		case SERVO:
			// Soon
			break;
	}
}

void motp_init() {
	// Setting up pins
	wiringPiSetup();
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);

	// Create motors
	motp_L = motp_newMotor(DC, 0, 1);
	motp_R = motp_newMotor(DC, 2, 3);
	isInit = 1;
}

void mot_Forward(int speed) {
	if (!isInit) motp_init();
	if (speed >= 0) {
		motp_setMotor(&motp_L, speed);
		motp_setMotor(&motp_R, speed);
	}
}

void mot_Backward(int speed) {
	if (!isInit) motp_init();
	if (speed >= 0) {
		motp_setMotor(&motp_L, -speed);
		motp_setMotor(&motp_R, -speed);
	}
}

void mot_Left(int speed) {
	if (!isInit) motp_init();
	if (speed >= 0) {
		motp_setMotor(&motp_L, -speed);
		motp_setMotor(&motp_R, speed);
	}
}

void mot_Right(int speed) {
	if (!isInit) motp_init();
	if (speed >= 0) {
		motp_setMotor(&motp_L, speed);
		motp_setMotor(&motp_R, -speed);
	}
}

#endif // MOTOR_IMPLEMENTATION

#endif // !MOTOR_HEADER
