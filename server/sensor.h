#ifndef SENSOR_HEADER
#define SENSOR_HEADER

// Returns the value indicated by the specified sensor
int sen_GetSensorVal(int sensor); // 0 to 7: analog sensors from MCP3008; 8 = Temperature from BMP180; 9 = Pressure from BMP180

#ifdef SENSOR_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#define BAR_IMPLEMENTATION
#include "barometer.h"

#define SENP_PIN_BASE 100

static int senp_isInit = 0;

int senp_init() {
    if (mcp3004Setup(SENP_PIN_BASE, 0)) return -1;
    if (bar_init()) return -1;
	senp_isInit = 1;
	return 0;
}

int sen_GetSensorVal(int sensor) {
    if (!senp_isInit && senp_init()) return -1;
	if (sensor < 0 || sensor > 9) return -1;
    if (sensor < 8)
        return analogRead(SENP_PIN_BASE + sensor);
    double T, P;
    if (bar_startTemperature() || bar_getTemperature(&T) || bar_startPressure(3) || bar_getPressure(&P, &T)) return -1;
    if (sensor == 8) return (int)(T * 1000);
    if (sensor == 9) return (int)P;
    return -1;
}

#endif

#endif
