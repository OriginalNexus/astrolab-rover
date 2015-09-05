#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()
#include <time.h>
#include <signal.h>
#include <string.h>
#include <ncurses.h>
#include <errno.h>

#include <wiringPi.h>

#define TCP_NCURSES
#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"
#define MC_LOG
#define MC_NCURSES
#define MOTOR_CONTROL_IMPLEMENTATION
#include "MotorControl.h"
#define DRIVE_IMPLEMENTATION
#include "drive.h"
#define PACK_IMPLEMENTATION
#include "../shared/package.h"
#define SENSOR_IMPLEMENTATION
#include "sensor.h"


// Port used
int port = 0;


void m_perror(char * msg);
void m_error(char * msg);
void init(void);
void finish(int sig);


void m_perror(char * msg) {
	printw("### Server Error: %s: %s.\n", msg, strerror(errno)); refresh();
	getch();

	endwin();
	exit(EXIT_FAILURE);
}

void m_error(char * msg) {
	printw("### Server Error: %s.\n", msg); refresh();
	getch();

	endwin();
	exit(EXIT_FAILURE);
}

void init(void) {
	// Handle SIGTERM and SIGINT
	struct sigaction sigAction;
	memset (&sigAction, '\0', sizeof(sigAction));
	sigAction.sa_handler = finish;
	if (sigaction(SIGTERM, &sigAction, NULL) < 0)
		m_perror("Function sigaction(SIGTERM) failed");
	if (sigaction(SIGINT, &sigAction, NULL) < 0)
		m_perror("Function sigaction(SIGINT) failed");
	// Ignore SIGPIPE
	struct sigaction sig_ign;
	memset (&sig_ign, '\0', sizeof(sig_ign));
	sig_ign.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sig_ign, NULL) < 0)
		m_perror("Function sigaction(SIGPIPE) failed");

	// Initialize ncurses
	initscr();
	cbreak();
	noecho();
	nonl();
	keypad(stdscr, TRUE);

	// Setup wiring pi (used by drive.h)
	wiringPiSetup();

	// Set TCP log level
	tcp_SetLogLevel(2);

	// Get port
	FILE * fp = fopen("../shared/port", "rt");
	if (fp == NULL) {
		m_error("Failed to open ../shared/port");
	}
	if (fscanf(fp, "%d", &port) < 1) {
		m_error("Failed to read port from file ../shared/port");
	}
	fclose(fp);
}

void finish(int sig) {
	mc_Finish();
	tcp_CloseServer();
	endwin();
	exit(0);
}

int main(int argc, char * argv[]) {
	init();

	printw("=======================================\n");
	printw("======== AstroLab Rover Server ========\n");
	printw("=======================================\n");

	refresh();

	while (0) {
		move(3, 0);
		clrtobot();
		mvprintw(3, 0, "Sensor 0: %d", sen_GetSensorVal(0));
		mvprintw(4, 0, "Sensor 1: %d", sen_GetSensorVal(1));
		mvprintw(5, 0, "Sensor 2: %d", sen_GetSensorVal(2));
		mvprintw(6, 0, "Sensor 3: %d", sen_GetSensorVal(3));
		mvprintw(7, 0, "Sensor 4: %d", sen_GetSensorVal(4));
		mvprintw(8, 0, "Sensor 5: %d", sen_GetSensorVal(5));
		mvprintw(9, 0, "Sensor 6: %d", sen_GetSensorVal(6));
		mvprintw(10, 0, "Sensor 7: %d", sen_GetSensorVal(7));

		refresh();

		usleep(100000);
	}

	if (tcp_ConnectToClient(port)) m_error("Failed to connect");

	pck_Client cliPack;
	pck_Server servPack;
	int run = 1, reset = 0;
	while (run) {
		if (tcp_ReadFromClient(&cliPack, sizeof(pck_Client))) continue;
		switch (cliPack.cmd) {
			case CMD_EXIT:
				servPack = pck_NewServerPck(0);
				run = 0;
				break;
			case CMD_DISCONNECT:
				servPack = pck_NewServerPck(0);
				reset = 1;
				break;
			// case CMD_FORWARD:
			// 	servPack = pck_NewServerPck(drv_Forward(cliPack.data));
			// 	break;
			// case CMD_BACKWARD:
			// 	servPack = pck_NewServerPck(drv_Backward(cliPack.data));
			// 	break;
			// case CMD_LEFT:
			// 	servPack = pck_NewServerPck(drv_Left(cliPack.data));
			// 	break;
			// case CMD_RIGHT:
			// 	servPack = pck_NewServerPck(drv_Right(cliPack.data));
			// 	break;
			case CMD_CLAW:
				servPack = pck_NewServerPck(drv_Claw(cliPack.data));
				break;
			case CMD_STEP_STEP:
				servPack = pck_NewServerPck(drv_Step_Step(cliPack.data));
				break;
			case CMD_STEP_RELEASE:
				servPack = pck_NewServerPck(drv_Step_Release());
				break;
			case CMD_SENSOR:
				servPack = pck_NewServerPck(sen_GetSensorVal(cliPack.data));
				break;
			case CMD_SET_MOTOR_L:
				servPack = pck_NewServerPck(drv_SetMotor(DRV_L_NUM, cliPack.data));
				break;
			case CMD_SET_MOTOR_R:
				servPack = pck_NewServerPck(drv_SetMotor(DRV_R_NUM, cliPack.data));
				break;
			default:
				servPack = pck_NewServerPck(-1);
				break;
		}
		if (tcp_WriteToClient(&servPack, sizeof(pck_Server))) m_error("Failed to write to client");

		if (reset) {
			mc_StopAll();
			tcp_CloseServer();
			printw("Client disconnected.\n"); refresh();
			if (tcp_ConnectToClient(port)) m_error("Failed to connect to client");
			reset = 0;
		}
	}

	finish(0);

	return 0;
}
