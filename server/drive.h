#ifndef DRIVE_HEADER
#define DRIVE_HEADER


int drv_Forward(int speed);
int drv_Backward(int speed);
int drv_Left(int speed);
int drv_Right(int speed);
int drv_Claw(int angle); // angle: 0 = closed, 90 = open
int drv_Step_Step(int steps); // steps > 0, move forward, steps < 0, move backward
int drv_Step_Release(void);


#ifdef DRIVE_IMPLEMENTATION

#include "MotorControl.h"

#define DRVP_L_NUM 3
#define DRVP_R_NUM 4

#define DRVP_STEP_NUM 1
#define DRVP_STEP_STEPS 400
#define DRVP_STEP_RPM 10
#define DRVP_STEP_STYLE MC_SINGLE

#define DRVP_CLAW_PIN 0
#define DRVP_SERVO_FREQ 50
#define DRVP_MOTOR_FREQ 1500

int drvp_currentFreq = DRVP_MOTOR_FREQ;
int drvp_isStepInit = 0;

int drvp_changeFreq(int freq);

int drvp_changeFreq(int freq) {
	if (drvp_currentFreq != freq) {
		if (mc_StopAll() || mc_SetPWMFreq(freq)) return -1;
		drvp_currentFreq = freq;
	}
	return 0;
}

int drv_Forward(int speed) {
	if (drvp_changeFreq(DRVP_MOTOR_FREQ)) return -1;
	if (mc_DC_SetSpeed(DRVP_L_NUM, speed) || mc_DC_SetSpeed(DRVP_R_NUM, speed)) return -1;
	if (speed && (mc_DC_Run(DRVP_L_NUM, MC_FORWARD) || mc_DC_Run(DRVP_R_NUM, MC_FORWARD))) return -1;
	if (!speed && (mc_DC_Run(DRVP_L_NUM, MC_RELEASE) || mc_DC_Run(DRVP_R_NUM, MC_RELEASE))) return -1;
	return 0;
}

int drv_Backward(int speed) {
	if (drvp_changeFreq(DRVP_MOTOR_FREQ)) return -1;
	if (mc_DC_SetSpeed(DRVP_L_NUM, speed) || mc_DC_SetSpeed(DRVP_R_NUM, speed)) return -1;
	if (speed && (mc_DC_Run(DRVP_L_NUM, MC_BACKWARD) || mc_DC_Run(DRVP_R_NUM, MC_BACKWARD))) return -1;
	if (!speed && (mc_DC_Run(DRVP_L_NUM, MC_RELEASE) || mc_DC_Run(DRVP_R_NUM, MC_RELEASE))) return -1;
	return 0;
}

int drv_Left(int speed) {
	if (drvp_changeFreq(DRVP_MOTOR_FREQ)) return -1;
	if (mc_DC_SetSpeed(DRVP_L_NUM, speed / 2) || mc_DC_SetSpeed(DRVP_R_NUM, speed)) return -1;
	if (speed && (mc_DC_Run(DRVP_L_NUM, MC_FORWARD) || mc_DC_Run(DRVP_R_NUM, MC_FORWARD))) return -1;
	if (!speed && (mc_DC_Run(DRVP_L_NUM, MC_RELEASE) || mc_DC_Run(DRVP_R_NUM, MC_RELEASE))) return -1;
	return 0;
}

int drv_Right(int speed) {
	if (drvp_changeFreq(DRVP_MOTOR_FREQ)) return -1;
	if (mc_DC_SetSpeed(DRVP_L_NUM, speed) || mc_DC_SetSpeed(DRVP_R_NUM, speed / 2)) return -1;
	if (speed && (mc_DC_Run(DRVP_L_NUM, MC_FORWARD) || mc_DC_Run(DRVP_R_NUM, MC_FORWARD))) return -1;
	if (!speed && (mc_DC_Run(DRVP_L_NUM, MC_RELEASE) || mc_DC_Run(DRVP_R_NUM, MC_RELEASE))) return -1;
	return 0;
}

int drv_Claw(int angle) {
	if (drvp_changeFreq(DRVP_SERVO_FREQ)) return -1;
	int value = (angle / 90.0) * 220 + 180;
	if (mcp_setPWM(DRVP_CLAW_PIN, value)) return -1;
	return 0;
}

int drv_Step_Step(int steps) {
	if (!drvp_isStepInit) {
		if (mc_Step_SetStepsPerRev(DRVP_STEP_NUM, DRVP_STEP_STEPS)) return -1;
		if (mc_Step_SetRPM(DRVP_STEP_NUM, DRVP_STEP_RPM)) return -1;
		drvp_isStepInit = 1;
	}
	if (drvp_changeFreq(DRVP_SERVO_FREQ)) return -1;
	if (steps > 0 && mc_Step_Step(DRVP_STEP_NUM, steps, MC_FORWARD, DRVP_STEP_STYLE) )return -1;
	if (steps < 0 && mc_Step_Step(DRVP_STEP_NUM, -steps, MC_BACKWARD, DRVP_STEP_STYLE)) return -1;
	return 0;
}

int drv_Step_Release(void) {
	if (!drvp_isStepInit) {
		if (mc_Step_SetStepsPerRev(DRVP_STEP_NUM, DRVP_STEP_STEPS)) return -1;
		if (mc_Step_SetRPM(DRVP_STEP_NUM, DRVP_STEP_RPM)) return -1;
		drvp_isStepInit = 1;
	}
	if (mc_Step_Release(DRVP_STEP_NUM)) return -1;
	return 0;
}

#endif // DRIVE_IMPLEMENTATION

#endif // !DRIVE_HEADER
