#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ncurses.h>

#define TCP_NCURSES
#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"
#define PACK_IMPLEMENTATION
#include "../shared/package.h"

#define MAX_LEFT 10
#define MAX_RIGHT 10

// Port used
int port = 0;
int isConnected = 0;

int left = 0;
int right = 0;
int claw = 0;
int stepAngle = 0;
int stepOldAngle = 0;

// Sensor functions
int (* senFunc[8])(char *, int, int);
int senFunc1(char * s, int n, int a) {
	return snprintf(s, n, "None: %d", a);
}
int senFunc2(char * s, int n, int a) {
	return snprintf(s, n, "Light: %d", a);
}
int senFunc3(char * s, int n, int a) {
	return snprintf(s, n, "None: %d", a);
}
int senFunc4(char * s, int n, int a) {
	return snprintf(s, n, "Force: %lf grams", 9.29319 * exp(0.6222 * a * 0.00488));
}
int senFunc5(char * s, int n, int a) {
	return snprintf(s, n, "Humidity: %lf%% ", a * 100 / 1023.0);
}
int senFunc6(char * s, int n, int a) {
	return snprintf(s, n, "CO: %lf ppm", 0.282806 * exp(1.62986 * a * 0.00488));
}
int senFunc7(char * s, int n, int a) {
	return snprintf(s, n, "H2: %lf ppm", 0.387551 * exp(2.48982 * a * 0.00488));
}
int senFunc8(char * s, int n, int a) {
	return snprintf(s, n, "CH4: %lf ppm", 4.0371 * exp(2.06903 * a * 0.00488));
}


void m_perror(char * msg);
void m_error(char * msg);
void init(void);
void finish(int sig);
int connectToServer(void);
int sendCmd(int cmd, int data);


void m_perror(char * msg) {
	printw("### Client Error: %s: %s.\n", msg, strerror(errno)); refresh();
	getch();

	endwin();
	exit(EXIT_FAILURE);
}

void m_error(char * msg) {
	printw("### Client Error: %s: %s.\n", msg, strerror(errno)); refresh();
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

	// Set up sensor functions
	senFunc[0] = senFunc1; senFunc[1] = senFunc2; senFunc[2] = senFunc3; senFunc[3] = senFunc4;
	senFunc[4] = senFunc5; senFunc[5] = senFunc6; senFunc[6] = senFunc7; senFunc[7] = senFunc8;

	// Initialize ncurses
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	// Set TCP log level
	tcp_SetLogLevel(1);

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
	if (isConnected) {
		sendCmd(CMD_DISCONNECT, 0);
		tcp_CloseClient();
	}
	endwin();
	exit(0);
}

int connectToServer(void) {
	if (isConnected) tcp_CloseClient();
	isConnected = 0;
	char * hostName = (char *) malloc(32 * sizeof(char));
	while (1) {
		echo();
		nocbreak();
		printw("Enter server name: "); refresh();
		scanw("%31s", hostName);
		cbreak();
		noecho();
		if (strcmp(hostName, "exit") == 0) break;
		if (!tcp_ConnectToServer(hostName, port)) {
			isConnected = 1;
			break;
		}
	}
	free(hostName);
	if (!isConnected) return -1;
	return 0;
}

int sendCmd(int cmd, int data) {
	pck_Client cliPack;
	pck_Server servPack = pck_NewServerPck(-1);
	cliPack = pck_NewClientPck(cmd, data);
	if (tcp_WriteToServer(&cliPack, sizeof(pck_Client))) {
		connectToServer();
		return -1;
	}
	if (tcp_ReadFromServer(&servPack, sizeof(pck_Server))) {
		connectToServer();
		return -1;
	}
	//if (servPack.return_val < 0) printw("Command failed!\n");
	return servPack.return_val;
}

int main(int argc, char * argv[]) {
	init();

	printw("=======================================\n");
	printw("======== AstroLab Rover Client ========\n");
	printw("=======================================\n");

	move(17, 0);

	if (connectToServer()) finish(0);

	int run = 1;
	int ch, x, y, i;
	while (run) {
		// Print speed
		getyx(stdscr, y, x);

		move(3, 0); clrtoeol();
		printw("| L    | R    | Arm  | Claw |\n");
		clrtoeol();
		mvprintw(4, 0, "| %d", left);
		mvprintw(4, 7, "| %d", right);
		mvprintw(4, 14, "| %d", stepAngle);
		mvprintw(4, 21, "| %d", claw);
		mvprintw(4, 28, "|\n\n");

		move(y, x);
		refresh();

		ch = getch();
		switch (ch) {
			case '7':
				if (left < MAX_LEFT) left++;
				break;
			case '1':
				if (left > -MAX_LEFT) left--;
				break;
			case '9':
				if (right < MAX_RIGHT) right++;
				break;
			case '3':
				if (right > -MAX_RIGHT) right--;
				break;
			case '6':
				if (claw < 90) claw += 5;
				break;
			case '4':
				if (claw > 0) claw -= 5;
				break;
			case '8':
				if (stepAngle < 3600) stepAngle += 9;
				break;
			case '2':
				if (stepAngle > -3600) stepAngle -= 9;
				break;
			case KEY_F(7):
				run = 0;
				break;
			case KEY_F(8):
				sendCmd(CMD_EXIT, 0);
				tcp_CloseClient();
				isConnected = 0;
				run = 0;
				break;
			default:

				break;
		}
		switch (ch) {
			case '7':
			case '1':
				sendCmd(CMD_SET_MOTOR_L, ((double)left / MAX_LEFT) * (1 << 12));
				break;
			case '9':
			case '3':
				sendCmd(CMD_SET_MOTOR_R, ((double)right / MAX_RIGHT) * (1 << 12));
				break;
			case '4':
			case '6':
				sendCmd(CMD_CLAW, claw);
				break;
			case '8':
			case '2':
				if (stepAngle - stepOldAngle) {
					sendCmd(CMD_STEP_STEP, (stepAngle - stepOldAngle) / 0.9);
					stepOldAngle = stepAngle;
				}
				break;
			case '5':
				sendCmd(CMD_SET_MOTOR_L, 0);
				sendCmd(CMD_SET_MOTOR_R, 0);
				sendCmd(CMD_STEP_RELEASE, 0);
				sendCmd(CMD_SET_MOTOR_L, 0);
				left = right = claw = stepAngle = stepOldAngle = 0;
				break;
			case '0':
				getyx(stdscr, y, x);

				move(6, 0); clrtoeol();
				// Get sensors
				int maxLength = 40; maxLength++;
				char * s = (char *)malloc(maxLength * sizeof(char));
				for (i = 0; i < 8; i++) {
					clrtoeol();
					if ((*senFunc[i])(s, maxLength, sendCmd(CMD_SENSOR, i)) < 0) strcpy(s, "Error");
					printw("%s\n", s);
				}
				free(s);
				clrtoeol();
				printw("Temperature: %.3f C\n", sendCmd(CMD_SENSOR, 8) / 1000.0);
				clrtoeol();
				printw("Pressure: %d Pa\n", sendCmd(CMD_SENSOR, 9));

				move(y, x);
				refresh();
				break;
		}

	}

	finish(0);

	return 0;
}
