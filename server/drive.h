#ifndef DRIVE_HEADER
#define DRIVE_HEADER

#define DRV_L_NUM 2
#define DRV_R_NUM 1

// int drv_Forward(int speed);
// int drv_Backward(int speed);
// int drv_Left(int speed);
// int drv_Right(int speed);
int drv_SetMotor(int mot, int speed);
int drv_Claw(int angle); // angle: 0 = closed, 90 = open
int drv_Step_Step(int steps); // steps > 0, move forward, steps < 0, move backward
int drv_Step_Release(void);


#ifdef DRIVE_IMPLEMENTATION

#include "MotorControl.h"

#include <time.h>
#include <wiringPi.h>


#define DRVP_STEP_NUM 2
#define DRVP_STEP_STEPS 400
#define DRVP_STEP_RPM 5
#define DRVP_STEP_STYLE MC_DOUBLE

#define DRVP_RELAY_GPIO 0

#define DRVP_CLAW_PWM 0
#define DRVP_ARM_FREQ 50
#define DRVP_MOTOR_FREQ 1500

static int drvp_currentFreq = 0;
static int drvp_isArmInit = 0;
static int drvp_isUsingArm = 0;

int drvp_wait_us(unsigned long us);
int drvp_usingArm(int value);


int drvp_wait_ms(unsigned long ms) {
	struct timespec delay;
	delay.tv_sec = ms / 1000;
	delay.tv_nsec = (ms % 1000) * 1000000;
	return nanosleep(&delay, NULL);
}

int drvp_usingArm(int value) {
	if (!drvp_isArmInit) {
		if (mc_StopAll() || mc_SetPWMFreq(DRVP_MOTOR_FREQ) || drvp_wait_ms(100)) return -1;
		// Init stepper
		if (mc_Step_SetStepsPerRev(DRVP_STEP_NUM, DRVP_STEP_STEPS)) return -1;
		if (mc_Step_SetRPM(DRVP_STEP_NUM, DRVP_STEP_RPM)) return -1;
		// Init relay
		pinMode(DRVP_RELAY_GPIO, OUTPUT);
		digitalWrite(DRVP_RELAY_GPIO, LOW);
		drvp_wait_ms(50);
		drvp_isArmInit = 1;
	}
	if (value) {
		if (!drvp_isUsingArm) {
			if (mc_StopAll()) return -1;
			if (drvp_currentFreq != DRVP_ARM_FREQ && mc_SetPWMFreq(DRVP_ARM_FREQ)) return -1;
			if (drvp_wait_ms(100)) return -1;
			digitalWrite(DRVP_RELAY_GPIO, HIGH);
			if (drvp_wait_ms(50)) return -1;
			drvp_isUsingArm = 1;
		}
	}
	else {
		if (drvp_isUsingArm) {
			if (mc_StopAll()) return -1;
			if (drvp_currentFreq != DRVP_MOTOR_FREQ && mc_SetPWMFreq(DRVP_MOTOR_FREQ)) return -1;
			if (drvp_wait_ms(100)) return -1;
			digitalWrite(DRVP_RELAY_GPIO, LOW);
			if (drvp_wait_ms(50)) return -1;
			drvp_isUsingArm = 0;
		}
	}
	return 0;
}

int drv_SetMotor(int mot, int speed) {
	if (mot != DRV_L_NUM && mot != DRV_R_NUM) return -1;
	if (drvp_usingArm(0)) return -1;
	if (speed > 0 && (mc_DC_SetSpeed(mot, speed) || mc_DC_SetSpeed(mot, speed))) return -1;
	else if (speed < 0 && (mc_DC_SetSpeed(mot, -speed) || mc_DC_SetSpeed(mot, -speed))) return -1;

	if (speed > 0 && (mc_DC_Run(mot, MC_FORWARD) || mc_DC_Run(mot, MC_FORWARD))) return -1;
	else if (speed < 0 && ((mc_DC_Run(mot, MC_BACKWARD) || mc_DC_Run(mot, MC_BACKWARD)))) return -1;
	else if (speed == 0 && (mc_DC_Run(mot, MC_RELEASE) || mc_DC_Run(mot, MC_RELEASE))) return -1;
	return 0;
}

// int drv_Forward(int speed) {
// 	if (drvp_usingArm(0)) return -1;
// 	if (mc_DC_SetSpeed(DRV_L_NUM, speed) || mc_DC_SetSpeed(DRV_R_NUM, speed)) return -1;
// 	if (speed && (mc_DC_Run(DRV_L_NUM, MC_FORWARD) || mc_DC_Run(DRV_R_NUM, MC_FORWARD))) return -1;
// 	if (!speed && (mc_DC_Run(DRV_L_NUM, MC_RELEASE) || mc_DC_Run(DRV_R_NUM, MC_RELEASE))) return -1;
// 	return 0;
// }
//
// int drv_Backward(int speed) {
// 	if (drvp_usingArm(0)) return -1;
// 	if (mc_DC_SetSpeed(DRV_L_NUM, speed) || mc_DC_SetSpeed(DRV_R_NUM, speed)) return -1;
// 	if (speed && (mc_DC_Run(DRV_L_NUM, MC_BACKWARD) || mc_DC_Run(DRV_R_NUM, MC_BACKWARD))) return -1;
// 	if (!speed && (mc_DC_Run(DRV_L_NUM, MC_RELEASE) || mc_DC_Run(DRV_R_NUM, MC_RELEASE))) return -1;
// 	return 0;
// }
//
// int drv_Left(int speed) {
// 	if (drvp_usingArm(0)) return -1;
// 	if (mc_DC_SetSpeed(DRV_L_NUM, speed / 2) || mc_DC_SetSpeed(DRV_R_NUM, speed)) return -1;
// 	if (speed && (mc_DC_Run(DRV_L_NUM, MC_BACKWARD) || mc_DC_Run(DRV_R_NUM, MC_FORWARD))) return -1;
// 	if (!speed && (mc_DC_Run(DRV_L_NUM, MC_RELEASE) || mc_DC_Run(DRV_R_NUM, MC_RELEASE))) return -1;
// 	return 0;
// }
//
// int drv_Right(int speed) {
// 	if (drvp_usingArm(0)) return -1;
// 	if (mc_DC_SetSpeed(DRV_L_NUM, speed) || mc_DC_SetSpeed(DRV_R_NUM, speed / 2)) return -1;
// 	if (speed && (mc_DC_Run(DRV_L_NUM, MC_FORWARD) || mc_DC_Run(DRV_R_NUM, MC_BACKWARD))) return -1;
// 	if (!speed && (mc_DC_Run(DRV_L_NUM, MC_RELEASE) || mc_DC_Run(DRV_R_NUM, MC_RELEASE))) return -1;
// 	return 0;
// }

int drv_Claw(int angle) {
	if (drvp_usingArm(1)) return -1;
	int value = (angle / 90.0) * 210 + 180;
	if (mcp_setPWM(DRVP_CLAW_PWM, value)) return -1;
	return 0;
}

int drv_Step_Step(int steps) {
	if (drvp_usingArm(1)) return -1;
	if (steps > 0 && mc_Step_Step(DRVP_STEP_NUM, steps, MC_FORWARD, DRVP_STEP_STYLE))return -1;
	if (steps < 0 && mc_Step_Step(DRVP_STEP_NUM, -steps, MC_BACKWARD, DRVP_STEP_STYLE)) return -1;
	return 0;
}

int drv_Step_Release(void) {
	if (mc_Step_Release(DRVP_STEP_NUM)) return -1;
	return 0;
}

#endif // DRIVE_IMPLEMENTATION

#endif // !DRIVE_HEADER
