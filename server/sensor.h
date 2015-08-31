#ifndef SENSOR_HEADER
#define SENSOR_HEADER

#define SEN_LIGHT 0
#define SEN_METHANE 1

// Returns the value indicated by the specified sensor
int sen_GetSensorVal(int sensor);

#ifdef SENSOR_IMPLEMENTATION

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <mcp3004.h>
#include <stdio.h>
#include <stdlib.h>

#define SEN_PIN_BASE 100

static int senp_isInit = 0;

void senp_init() {
    mcp3004Setup(SEN_PIN_BASE, 0);
}

int sen_GetSensorVal(int sensor) {
    if (!senp_isInit) {
        senp_init();
        senp_isInit = 1;
    }

    return analogRead(SEN_PIN_BASE + sensor);
}

#endif

#endif
