#ifndef BAR_HEADER
#define BAR_HEADER

#define BMP180_ADDR 0x77 // 7-bit address

#define	BMP180_REG_CONTROL 0xF4
#define	BMP180_REG_RESULT 0xF6

#define	BMP180_COMMAND_TEMPERATURE 0x2E
#define	BMP180_COMMAND_PRESSURE0 0x34
#define	BMP180_COMMAND_PRESSURE1 0x74
#define	BMP180_COMMAND_PRESSURE2 0xB4
#define	BMP180_COMMAND_PRESSURE3 0xF4


int bar_init(void);
int bar_startTemperature(void);
int bar_getTemperature(double * T);
int bar_startPressure(char oversampling);
int bar_getPressure(double * P, double  * T);

#ifdef BAR_IMPLEMENTATION

#include <stdint.h>
#include <math.h>
#include <time.h>
#include <wiringPiI2C.h>

int barp_wait_ms(unsigned long ms);
int barp_i2c_connect(int addr);
int barp_readInt(int address, int16_t * value);
int barp_readUInt(char address, uint16_t * value);

static int16_t barp_AC1, barp_AC2, barp_AC3, barp_VB1, barp_VB2, barp_MB, barp_MC, barp_MD;
static uint16_t barp_AC4, barp_AC5, barp_AC6;
static double barp_c5, barp_c6, barp_mc, barp_md, barp_x0, barp_x1, barp_x2, barp_y0, barp_y1, barp_y2, barp_p0, barp_p1, barp_p2;
int barp_i2c_fd = -1;

int barp_wait_ms(unsigned long ms) {
	struct timespec delay;
	delay.tv_sec = ms / 1000;
	delay.tv_nsec = (ms % 1000) * 1000000;
	return nanosleep(&delay, NULL);
}

int barp_i2c_connect(int addr) {
    barp_i2c_fd = wiringPiI2CSetup(addr);
	if (barp_i2c_fd < 0)
		return -1;
    return 0;
}

// Read a signed integer (two bytes) from device
// address: register to start reading (plus subsequent register)
// value: external variable to store data (function modifies value)
int barp_readInt(int address, int16_t * value){
	if (barp_i2c_fd < 0) return -1;
    *value = (int16_t) ((((uint8_t) wiringPiI2CReadReg8(barp_i2c_fd, address)) << 8) + ((uint8_t) wiringPiI2CReadReg8(barp_i2c_fd, address + 1)));
    return 0;
}

// Read an unsigned integer (two bytes) from device
// address: register to start reading (plus subsequent register)
// value: external variable to store data (function modifies value)
int barp_readUInt(char address, uint16_t * value){
	if (barp_i2c_fd < 0) return -1;
    *value = (uint16_t) ((((uint8_t) wiringPiI2CReadReg8(barp_i2c_fd, address)) << 8) + ((uint8_t)wiringPiI2CReadReg8(barp_i2c_fd, address + 1)));
    return 0;
}

// Initialize library for subsequent pressure measurements
int bar_init(void) {
	double c3,c4,b1;

	if (barp_i2c_connect(BMP180_ADDR)) return -1;

	// The BMP180 includes factory calibration data stored on the device.
	// Each device has different numbers, these must be retrieved and
	// used in the calculations when taking pressure measurements.

	// Retrieve calibration data from device:

	if (barp_readInt(0xAA, &barp_AC1) ||
		barp_readInt(0xAC, &barp_AC2) ||
		barp_readInt(0xAE, &barp_AC3) ||
		barp_readUInt(0xB0, &barp_AC4) ||
		barp_readUInt(0xB2, &barp_AC5) ||
		barp_readUInt(0xB4, &barp_AC6) ||
		barp_readInt(0xB6, &barp_VB1) ||
		barp_readInt(0xB8, &barp_VB2) ||
		barp_readInt(0xBA, &barp_MB) ||
		barp_readInt(0xBC, &barp_MC) ||
		barp_readInt(0xBE, &barp_MD))
	{
		return -1;
	}

	// Compute floating-point polynominals:

	c3 = 160.0 * pow(2,-15) * barp_AC3;
	c4 = pow(10,-3) * pow(2,-15) * barp_AC4;
	b1 = pow(160,2) * pow(2,-30) * barp_VB1;
	barp_c5 = (pow(2,-15) / 160) * barp_AC5;
	barp_c6 = barp_AC6;
	barp_mc = (pow(2,11) / pow(160,2)) * barp_MC;
	barp_md = barp_MD / 160.0;
	barp_x0 = barp_AC1;
	barp_x1 = 160.0 * pow(2,-13) * barp_AC2;
	barp_x2 = pow(160,2) * pow(2,-25) * barp_VB2;
	barp_y0 = c4 * pow(2,15);
	barp_y1 = c4 * c3;
	barp_y2 = c4 * b1;
	barp_p0 = (3791.0 - 8.0) / 1600.0;
	barp_p1 = 1.0 - 7357.0 * pow(2,-20);
	barp_p2 = 3038.0 * 100.0 * pow(2,-36);

	return 0;
}

// Begin a temperature reading.
int bar_startTemperature(void) {
	if (barp_i2c_fd < 0) return -1;
    if (wiringPiI2CWriteReg8(barp_i2c_fd, BMP180_REG_CONTROL, BMP180_COMMAND_TEMPERATURE)) return -1;
	if (barp_wait_ms(5)) return -1;
	return 0;
}

// Retrieve a previously-started temperature reading.
int bar_getTemperature(double * T) {
	double tu, a;
    int16_t t;
	if (barp_readInt(BMP180_REG_RESULT, &t)) return -1;
    tu = t;
	a = barp_c5 * (tu - barp_c6);
	*T = a + (barp_mc / (a + barp_md));
	return 0;
}

// Begin a pressure reading.
// Oversampling: 0 to 3, higher numbers are slower, higher-res outputs.
int bar_startPressure(char oversampling) {
	unsigned char data, delay;

	switch (oversampling)
	{
		case 0:
			data = BMP180_COMMAND_PRESSURE0;
			delay = 5;
		break;
		case 1:
			data = BMP180_COMMAND_PRESSURE1;
			delay = 8;
		break;
		case 2:
			data = BMP180_COMMAND_PRESSURE2;
			delay = 14;
		break;
		case 3:
			data = BMP180_COMMAND_PRESSURE3;
			delay = 26;
		break;
		default:
			data = BMP180_COMMAND_PRESSURE0;
			delay = 5;
		break;
	}
	if (wiringPiI2CWriteReg8(barp_i2c_fd, BMP180_REG_CONTROL, data)) return -1;
	if (barp_wait_ms(delay)) return -1;
	return 0;
}

// Retrieve a previously started pressure reading, calculate abolute pressure in Pa.
// Requires startPressure() to have been called prior and sufficient time elapsed.
// Requires recent temperature reading to accurately calculate pressure.
// P: external variable to hold pressure.
// T: previously-calculated temperature.
int bar_getPressure(double * P, double  * T) {
	unsigned char data[3];
	double pu,s,x,y,z;

	data[0] = wiringPiI2CReadReg8(barp_i2c_fd, BMP180_REG_RESULT);
	data[1] = wiringPiI2CReadReg8(barp_i2c_fd, BMP180_REG_RESULT + 1);
	data[2] = wiringPiI2CReadReg8(barp_i2c_fd, BMP180_REG_RESULT + 2);
	pu = (data[0] * 256.0) + data[1] + (data[2]/256.0);

	s = *T - 25.0;
	x = (barp_x2 * pow(s,2)) + (barp_x1 * s) + barp_x0;
	y = (barp_y2 * pow(s,2)) + (barp_y1 * s) + barp_y0;
	z = (pu - x) / y;
	*P = ((barp_p2 * pow(z,2)) + (barp_p1 * z) + barp_p0) * 100;

	return 0;
}

#endif // BAR_IMPLEMENTATION

#endif // !BAR_HEADER
