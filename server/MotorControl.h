#ifndef MOTOR_CONTROL_HEADER
#define MOTOR_CONTROL_HEADER


#ifndef MC_I2C_ADDRESS
#define MC_I2C_ADDRESS 0x60
#endif

#ifndef MC_PWM_FREQ
#define MC_PWM_FREQ 1500 // Hz
#endif

#ifndef MC_MICROSTEPS
#define MC_MICROSTEPS 16 // Even number
#endif

#define MC_FORWARD 1
#define MC_BACKWARD 2
#define MC_RELEASE 3

#define MC_SINGLE 1
#define MC_DOUBLE 2
#define MC_INTERLEAVE 3
#define MC_MICROSTEP 4

int mc_SetPWMFreq(int freq);
int mc_StopAll(void);
int mc_Finish(void);

int mc_DC_SetSpeed(int motNum, int speed);
int mc_DC_Run(int motNum, int cmd);

int mc_Step_SetRPM(int motNum, int rpm);
int mc_Step_SetStepsPerRev(int motNum, int steps);
int mc_Step_OneStep(int motNum, int dir, int style);
int mc_Step_Step(int motNum, int steps, int dir, int style);
int mc_Step_Release(int motNum);


#ifdef MOTOR_CONTROL_IMPLEMENTATION

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include <wiringPiI2C.h>
#ifdef MC_NCURSES
#include <ncurses.h>
#endif


#define MCP_PCA9685_MODE1 0x0
#define MCP_PCA9685_PRESCALE 0xFE

#define MCP_PCA9685_DEFAULT_MODE1 0x11
#define MCP_PCA9685_DEFAULT_FREQ 200

#define MCP_PCA9685_LED0_ON_L 0x6
#define MCP_PCA9685_LED0_OFF_L 0x8

#define MCP_PCA9685_ALL_LED_ON_L 0xFA
#define MCP_PCA9685_ALL_LED_OFF_L 0xFC

#define MCP_PI 3.14159265359


typedef struct {
  int pwm, pin1, pin2;
}mcp_DCMotor;

typedef struct {
  int pwm1, pin1, pin2;
  int pwm2, pin3, pin4;
  int stepsPerRev;
  int usPerStep;
  int currentStep;
}mcp_StepperMotor;


static int mcp_i2c_fd = -1;
static mcp_DCMotor * mcp_dcMotors;
static mcp_StepperMotor * mcp_steppers;
static int mcp_isInit = 0;
static int * mcp_microStepCurve;


int mcp_init(void);
int mcp_error(char * msg);
int mcp_perror(char * msg);
int mcp_i2c_connect(int addr);
int mcp_i2c_read8(uint8_t reg, uint8_t * buff);
int mcp_i2c_write8(uint8_t reg, uint8_t data);
int mcp_writePWM(int num, uint16_t on, uint16_t off);
int mcp_setPWM(int num, int speed);
int mcp_setPin(int num, int val);


int mcp_error(char * msg) {
#ifdef MC_LOG
#ifdef MC_NCURSES
		printw("### MC error: %s.\n", msg); refresh();
#else
		fprintf(stderr, "### MC error: %s.\n", msg);
#endif
#endif
	return -1;
}

int mcp_perror(char * msg) {
#ifdef MC_LOG
#ifdef MC_NCURSES
		printw("### MC error: %s: %s.\n", msg, strerror(errno)); refresh();
#else
		fprintf(stderr, "### MC error: %s: %s.\n", msg, strerror(errno));
#endif
#endif
	return -1;
}

int mcp_init(void) {
	// Connect to the shield
    if (mcp_i2c_connect(MC_I2C_ADDRESS))
        return mcp_perror("mcp_init: Failed to connect to shield");

    // Clear MODE1
    if (mcp_i2c_write8(MCP_PCA9685_MODE1, 0x0))
        return mcp_error("mcp_init: Failed to clear MODE1");

	// Set PWM frequency
	mcp_isInit = 1;
    if (mc_SetPWMFreq(MC_PWM_FREQ)) {
        mcp_isInit = 0;
        return mcp_error("mcp_init: Failed to set PWM frequency");
    }
    mcp_isInit = 0;

    // Read MODE1
	uint8_t mode1;
    if (mcp_i2c_read8(MCP_PCA9685_MODE1, &mode1))
        return mcp_error("mcp_init: Failed to read MODE1");
    // Enable auto increment
    if (mcp_i2c_write8(MCP_PCA9685_MODE1, mode1 | 0x20))
        return mcp_error("mcp_init: Failed to enable auto increment");


    // Turn OFF all PWM pins
    mcp_isInit = 1;
    if (mc_StopAll()) {
        mcp_isInit = 0;
        return mcp_error("mcp_init: Failed to stop all PWM pins");
    }

    // Initialise motors
    mcp_dcMotors = (mcp_DCMotor *) malloc(4 * sizeof(mcp_DCMotor));
    mcp_steppers = (mcp_StepperMotor *) malloc(2 * sizeof(mcp_StepperMotor));

    mcp_dcMotors[0].pwm = mcp_steppers[0].pwm1 = 8;
    mcp_dcMotors[0].pin1 = mcp_steppers[0].pin1 = 10;
    mcp_dcMotors[0].pin2 = mcp_steppers[0].pin2 = 9;

    mcp_dcMotors[1].pwm = mcp_steppers[0].pwm2 = 13;
    mcp_dcMotors[1].pin1 = mcp_steppers[0].pin3 = 11;
    mcp_dcMotors[1].pin2 = mcp_steppers[0].pin4 = 12;

    mcp_dcMotors[2].pwm = mcp_steppers[1].pwm1 = 2;
    mcp_dcMotors[2].pin1 = mcp_steppers[1].pin1 = 4;
    mcp_dcMotors[2].pin2 = mcp_steppers[1].pin2 = 3;

    mcp_dcMotors[3].pwm = mcp_steppers[1].pwm2 = 7;
    mcp_dcMotors[3].pin1 = mcp_steppers[1].pin3 = 5;
    mcp_dcMotors[3].pin2 = mcp_steppers[1].pin4 = 6;

    int i;
    for (i = 0; i < 2; i++) {
        mcp_steppers[i].currentStep = 0;
        mcp_steppers[i].stepsPerRev = 0;
        mcp_steppers[i].usPerStep = -1;
    }

    // Calculate microstep curve
    mcp_microStepCurve = (int *) malloc((MC_MICROSTEPS + 1) * sizeof(int));
    for (i = 0; i < MC_MICROSTEPS + 1; i++)
        mcp_microStepCurve[i] = (int)floor((1 << 12) * sin((MCP_PI / 2 ) * (i / (double)MC_MICROSTEPS)) + 0.5);

    return 0;
}

int mcp_i2c_connect(int addr) {
    mcp_i2c_fd = wiringPiI2CSetup(addr);
	if (mcp_i2c_fd < 0)
		return mcp_perror("mcp_i2c_connect: Failed to connect to device");
    return 0;
}

int mcp_i2c_read8(uint8_t reg, uint8_t * buf) {
    if (mcp_i2c_fd < 0) return mcp_error("mcp_i2c_read8: Not connected to I2C. Connect first");
    int ret = wiringPiI2CReadReg8(mcp_i2c_fd, reg);
    if (ret < 0) {
        return mcp_perror("mcp_i2c_read8: Failed to read");
    }
    *buf = (uint8_t) ret;
    return 0;
}

int mcp_i2c_write8(uint8_t reg, uint8_t data) {
    if (mcp_i2c_fd < 0) return mcp_error("mcp_i2c_write8: Not connected to I2C. Connect first");
    if (wiringPiI2CWriteReg8(mcp_i2c_fd, reg, data))
        return mcp_perror("mcp_i2c_write8: Failed to write");
    return 0;
}

int mcp_writePWM(int num, uint16_t on, uint16_t off) {
	if (!mcp_isInit && mcp_init()) return mcp_error("mcp_writePWM: Library not initialised");
    if (num < 0 || num > 16) return mcp_error("mcp_writePWM: Invalid pin num");

    on = on & 0x1FFF; off = off & 0x1FFF; // only the first 13 bits are used

    if (num < 16) {
        if ((wiringPiI2CWriteReg16(mcp_i2c_fd, MCP_PCA9685_LED0_ON_L + 4 * num, on) < 0) ||
            (wiringPiI2CWriteReg16(mcp_i2c_fd, MCP_PCA9685_LED0_OFF_L + 4 * num, off) < 0)) {
            return mcp_perror("mcp_writePWM: Failed to write pwm");
        }
    }
    else {
        if ((wiringPiI2CWriteReg16(mcp_i2c_fd, MCP_PCA9685_ALL_LED_ON_L, on) < 0) ||
            (wiringPiI2CWriteReg16(mcp_i2c_fd, MCP_PCA9685_ALL_LED_OFF_L, off) < 0)) {
            return mcp_perror("mcp_writePWM: Failed to write pwm");
        }
    }
    return 0;
}

int mcp_setPWM(int num, int speed) {
    int ret;
    if (speed >= 1 << 12) ret = mcp_writePWM(num, 1 << 12, 0);
    else if (speed <= 0) ret = mcp_writePWM(num, 0, 1 << 12);
    else ret = mcp_writePWM(num, 0, speed);
    if (ret) return mcp_error("mcp_setPWM: Failed to set pwm");
    return 0;
}

int mcp_setPin(int num, int val) {
    int ret;
    if (val) ret = mcp_setPWM(num, 1 << 12); // ON
    else ret = mcp_setPWM(num, 0); // OFF
    if (ret) return mcp_error("mcp_setPin: Failed to set pin");
    return 0;
}


int mc_SetPWMFreq(int freq) {
    if (!mcp_isInit && mcp_init()) return mcp_error("mc_SetPWMFreq: Library not initialised");
    // Calculate prescale value with formula taken from PCA9685's datasheet
	uint8_t preScale = floor(25000000 / (double)(4096 * freq) + 0.5) - 1;

    preScale = (preScale < 3) ? 3: preScale;

    // Read MODE1
	uint8_t mode1 = MCP_PCA9685_DEFAULT_MODE1;
    if (mcp_i2c_read8(MCP_PCA9685_MODE1, &mode1))
        return mcp_error("mc_SetPWMFreq: Failed to read MODE1");

    uint8_t sleep = (mode1 & 0x7F) | 0x10; // 0x10 is SLEEP bit, 0x7F clears RESTART bit
    uint8_t wake = sleep & 0xEF; // 0xEF clears SLEEP bit
    uint8_t restart = wake | 0x80; // 0x80 sets RESTART bit

    // Sleep
	if (mcp_i2c_write8(MCP_PCA9685_MODE1, sleep))
        return mcp_error("mc_SetPWMFreq: Failed to sleep");

    // Set the prescaler
	if (mcp_i2c_write8(MCP_PCA9685_PRESCALE, preScale))
        return mcp_error("mc_SetPWMFreq: Failed to write prescaler");

    // Wake
    if (mcp_i2c_write8(MCP_PCA9685_MODE1, wake))
        return mcp_error("mc_SetPWMFreq: Failed to wake");

    // Allow time for oscillator to stabilize
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 1000000;
	nanosleep(&delay, NULL);

    // Restart
	if (mcp_i2c_write8(MCP_PCA9685_MODE1, restart))
        return mcp_error("mc_SetPWMFreq: Failed to restart");

    return 0;
}

int mc_StopAll(void) {
    if (mcp_setPWM(16, 0))
        return mcp_error("mc_StopAll: Failed to stop all PWM pins");
    return 0;
}

int mc_Finish(void) {
    if (!mcp_isInit) return 0;
    if (mc_StopAll())
        return mcp_error("mc_Finish: Failed to stop all pwm");
    if (mc_SetPWMFreq(MCP_PCA9685_DEFAULT_FREQ))
        return mcp_error("mc_Finish: Failed to restore prescale");
    if (mcp_i2c_write8(MCP_PCA9685_MODE1, MCP_PCA9685_DEFAULT_MODE1))
        return mcp_error("mc_Finish: Failed to restore MODE1");
    free(mcp_dcMotors);
    free(mcp_steppers);
    free(mcp_microStepCurve);
    mcp_isInit = 0;
    return 0;
}


int mc_DC_SetSpeed(int motNum, int speed) {
    if (!mcp_isInit && mcp_init()) return mcp_error("mc_DC_SetSpeed: Library not initialised");
    if (motNum < 1 || motNum > 4) return mcp_error("mc_DC_SetSpeed: Invalid motor number");
    motNum--;
    if (mcp_setPWM(mcp_dcMotors[motNum].pwm, speed))
        return mcp_error("mc_DC_SetSpeed: Failed to set speed");
    return 0;
}

int mc_DC_Run(int motNum, int cmd) {
	if (!mcp_isInit && mcp_init() < 0) return mcp_error("mc_DC_Run: Library not initialised");
    if (motNum < 1 || motNum > 4)
        return mcp_error("mc_DC_Run: Invalid motor number");
    motNum--;
    int ret = 0;
    switch (cmd) {
        case MC_FORWARD:
            ret = mcp_setPin(mcp_dcMotors[motNum].pin2, 0) ||
                mcp_setPin(mcp_dcMotors[motNum].pin1, 1);
            break;
        case MC_BACKWARD:
            ret = mcp_setPin(mcp_dcMotors[motNum].pin1, 0) ||
                mcp_setPin(mcp_dcMotors[motNum].pin2, 1);
            break;
        case MC_RELEASE:
            ret = mcp_setPin(mcp_dcMotors[motNum].pin1, 0) ||
                mcp_setPin(mcp_dcMotors[motNum].pin2, 0);
            break;
        default:
            return mcp_error("mc_DC_Run: Invalid command");
            break;
    }
    if (ret) return mcp_error("mc_DC_Run: Failed to run motor");
    return 0;
}


int mc_Step_SetRPM(int motNum, int rpm) {
	if (!mcp_isInit && mcp_init()) return mcp_error("mc_Step_SetRPM: Library not initialised");
    if (motNum < 1 || motNum > 2)
        return mcp_error("mc_Step_SetRPM: Invalid motor number");
    motNum--;
    if (rpm <= 0) return mcp_error("mc_Step_SetRPM: RPM must be > 0");
    if (mcp_steppers[motNum].stepsPerRev <= 0) return mcp_error("mc_Step_SetRPM: StepsPerRev not set");
    mcp_steppers[motNum].usPerStep = 60000000 / (rpm * mcp_steppers[motNum].stepsPerRev);
    return 0;
}

int mc_Step_SetStepsPerRev(int motNum, int steps) {
    if (!mcp_isInit && mcp_init()) return mcp_error("mc_Step_SetStepsPerRev: Library not initialised");
    if (motNum < 1 || motNum > 2)
        return mcp_error("mc_Step_SetStepsPerRev: Invalid motor number");
    motNum--;
    if (steps <= 0) return mcp_error("mc_Step_SetStepsPerRev: StepsPerRev must be > 0");
    mcp_steppers[motNum].stepsPerRev = steps;
    return 0;
}

int mc_Step_OneStep(int motNum, int dir, int style) {
	if (!mcp_isInit && mcp_init()) return mcp_error("mc_Step_OneStep: Library not initialised");
    if (motNum < 1 || motNum > 2)
        return mcp_error("mc_Step_OneStep: Invalid motor number");
    motNum--;

    if (dir != MC_FORWARD && dir != MC_BACKWARD)
        return mcp_error("mc_Step_OneStep: Invalid direction");
    if (style != MC_SINGLE && style != MC_DOUBLE && style != MC_INTERLEAVE && style != MC_MICROSTEP)
        return mcp_error("mc_Step_OneStep: Invalid style");

    int curStep = mcp_steppers[motNum].currentStep;

    // Calculate pwm1 and pwm2
    int pwm1 = 1 << 12, pwm2 = 1 << 12;
    if (style == MC_MICROSTEP) {
        if ((curStep / MC_MICROSTEPS) % 2) {
            pwm1 = mcp_microStepCurve[curStep % MC_MICROSTEPS];
            pwm2 = mcp_microStepCurve[MC_MICROSTEPS - (curStep % MC_MICROSTEPS)];
        }
        else {
            pwm1 = mcp_microStepCurve[MC_MICROSTEPS - (curStep % MC_MICROSTEPS)];
            pwm2 = mcp_microStepCurve[curStep % MC_MICROSTEPS];
        }
    }

    if (mcp_setPWM(mcp_steppers[motNum].pwm1, pwm1))
        return mcp_error("mc_Step_OneStep: Failed to set coil 1 speed");

    if (mcp_setPWM(mcp_steppers[motNum].pwm2, pwm2))
        return mcp_error("mc_Step_OneStep: Failed to set coil 2 speed");

    int pins_state = 0;
    if (style == MC_SINGLE) {
        pins_state |= 1 << (curStep / MC_MICROSTEPS);
    }
    else if (style == MC_DOUBLE || style == MC_MICROSTEP) {
        pins_state |= 1 << (curStep / MC_MICROSTEPS);
        pins_state |= 1 << (curStep / MC_MICROSTEPS + 1);
    }
    else if (style == MC_INTERLEAVE) {
        pins_state |= 1 << (curStep / MC_MICROSTEPS + 1);
        if (curStep % MC_MICROSTEPS == 0)
            pins_state |= 1 << (curStep / MC_MICROSTEPS);
    }

    //printw("%d %d    %d %d %d %d\n", pwm1, pwm2, (pins_state & (1 << 0)) || (pins_state & (1 << 4)), pins_state & (1 << 1), pins_state & (1 << 2), pins_state & (1 << 3)); refresh();

    if ((pins_state & (1 << 0)) || (pins_state & (1 << 4))) {
        if (mcp_setPin(mcp_steppers[motNum].pin1, 1))
            return mcp_error("mc_Step_OneStep: Failed to set coil 1 pin 1");
    }
    else {
        if (mcp_setPin(mcp_steppers[motNum].pin1, 0))
            return mcp_error("mc_Step_OneStep: Failed to set coil 1 pin 1");
    }

    if (pins_state & (1 << 1)) {
        if (mcp_setPin(mcp_steppers[motNum].pin4, 1))
            return mcp_error("mc_Step_OneStep: Failed to set coil 2 pin 2");
    }
    else {
        if (mcp_setPin(mcp_steppers[motNum].pin4, 0))
            return mcp_error("mc_Step_OneStep: Failed to set coil 2 pin 2");
    }

    if (pins_state & (1 << 2)) {
        if (mcp_setPin(mcp_steppers[motNum].pin2, 1))
            return mcp_error("mc_Step_OneStep: Failed to set coil 1 pin 2");
    }
    else {
        if (mcp_setPin(mcp_steppers[motNum].pin2, 0))
            return mcp_error("mc_Step_OneStep: Failed to set coil 1 pin 2");
    }

    if (pins_state & (1 << 3)) {
        if (mcp_setPin(mcp_steppers[motNum].pin3, 1))
            return mcp_error("mc_Step_OneStep: Failed to set coil 2 pin 1");
    }
    else {
        if (mcp_setPin(mcp_steppers[motNum].pin3, 0))
            return mcp_error("mc_Step_OneStep: Failed to set coil 2 pin 1");
    }

    // Get next step number based on style and direction
    if (style == MC_SINGLE || style == MC_DOUBLE) {
        if (curStep % MC_MICROSTEPS) {
            if (dir == MC_FORWARD)
                curStep += MC_MICROSTEPS - curStep % MC_MICROSTEPS;
            else
                curStep -= curStep % MC_MICROSTEPS;
        }
        else {
            if (dir == MC_FORWARD)
                curStep += MC_MICROSTEPS;
            else
                curStep -= MC_MICROSTEPS;
        }
    }
    else if (style == MC_INTERLEAVE) {
        if (curStep % (MC_MICROSTEPS / 2)) {
            if (dir == MC_FORWARD)
                curStep += (MC_MICROSTEPS / 2) - curStep % (MC_MICROSTEPS / 2);
            else
                curStep -= curStep % (MC_MICROSTEPS / 2);
        }
        else {
            if (dir == MC_FORWARD)
                curStep += MC_MICROSTEPS / 2;
            else
                curStep -= MC_MICROSTEPS / 2;
        }
    }
    else if (style == MC_MICROSTEP) {
        if (dir == MC_FORWARD)
            curStep++;
        else
            curStep--;
    }

    // Make sure curStep is positive
    curStep += MC_MICROSTEPS * 4;
    // Keep it between 0 and MC_MICROSTEPS * 4 - 1
    curStep %= MC_MICROSTEPS * 4;

    mcp_steppers[motNum].currentStep = curStep;

    return 0;
}

int mc_Step_Step(int motNum, int steps, int dir, int style) {
	if (!mcp_isInit && mcp_init()) return mcp_error("mc_Step_Step: Library not initialised");
    if (motNum < 1 || motNum > 2)
        return mcp_error("mc_Step_SetSpeed: Invalid motor number");
    motNum--;
    int delay = mcp_steppers[motNum].usPerStep;
    if (delay < 0) return mcp_error("mc_Step_Step: RPM and/or StepsPerRev not set");
    steps = (steps < 0) ? 0 : steps;

    if (style == MC_INTERLEAVE) {
        delay /= 2;
        steps *= 2;
    }
    else if (style == MC_MICROSTEP) {
        delay /= MC_MICROSTEPS;
        steps *= MC_MICROSTEPS;
    }

    struct timespec wait;
    wait.tv_nsec = (delay % 1000000) * 1000;
    wait.tv_sec = delay / 1000000;

    while (steps) {
        if (mc_Step_OneStep(motNum + 1, dir, style))
            return mcp_error("mc_Step_Step: Failed to step");
        nanosleep(&wait, NULL);
        steps--;
    }
    if (style == MC_MICROSTEP) {
        if (mc_Step_OneStep(motNum + 1, dir, style))
            return mcp_error("mc_Step_Step: Failed to step");
        nanosleep(&wait, NULL);
        mcp_steppers[motNum].currentStep--;
    }
    return 0;
}

int mc_Step_Release(int motNum) {
	if (!mcp_isInit && mcp_init()) return mcp_error("mc_Step_Release: Library not initialised");
    if (motNum < 1 || motNum > 2)
        return mcp_error("mc_Step_Release: Invalid motor number");
    motNum--;
    if (mcp_setPin(mcp_steppers[motNum].pin1, 0) ||
        mcp_setPin(mcp_steppers[motNum].pin2, 0) ||
        mcp_setPin(mcp_steppers[motNum].pin3, 0) ||
        mcp_setPin(mcp_steppers[motNum].pin4, 0) ||
        mcp_setPWM(mcp_steppers[motNum].pwm1, 0) ||
        mcp_setPWM(mcp_steppers[motNum].pwm2, 0))
        return mcp_error("mc_Step_Release: Failed to release all pins");
    return 0;
}

#endif

#endif
