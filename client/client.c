#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ncurses.h>

#define TCP_NCURSES
#define TCP_IMPLEMENTATION
#include "../shared/tcp.h"
#define PACK_IMPLEMENTATION
#include "../shared/package.h"

#define MAX_UP 10
#define MAX_LEFT 10

// Port used
int port = 0;

int forward = 0;
int left = 0;
int claw = 0;
int stepAngle = 0;
int stepOldAngle = 0;


int isConnected = 0;

void m_perror(char * msg);
void m_error(char * msg);
void init(void);
void finish(int sig);
void connectToServer(void);
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
}

void finish(int sig) {
	if (isConnected) {
		sendCmd(CMD_DISCONNECT, 0);
		tcp_CloseClient();
	}
	endwin();
	exit(0);
}

void connectToServer(void) {
	if (isConnected) tcp_CloseClient();
	isConnected = 0;
	char * hostName = (char *) malloc(32 * sizeof(char));
	int ch;
	nocbreak();
	echo();
	while (1) {
		printw("Enter server name: "); refresh();
		scanw("%31s", hostName);
		if (!tcp_ConnectToServer(hostName, port)) {
			cbreak();
			noecho();
			isConnected = 1;
			break;
		}
		printw("Try to start server and connect? (y / n): "); refresh();
		char * p = (char *) malloc(256 * sizeof(char));
		ch = getch(); getstr(p); free(p);
		if (ch == 'y') {
			char * cmd = (char *) malloc(256 * sizeof(char));
			snprintf(cmd, 256, "ssh -n -f nexus@%s \"sh -c 'cd ~/github/astrolab-rover/server/; nohup ./bin/server.out > /dev/null 2> /dev/null < /dev/null &'\"", hostName);
			system(cmd);
			free(cmd);
			if (!tcp_ConnectToServer(hostName, port)) {
				cbreak();
				noecho();
				isConnected = 1;
				break;
			}
			printw("Failed. Try again\n"); refresh();
		}
	}
	free(hostName);
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
	if (servPack.return_val < 0) printw("Command failed!\n");
	return servPack.return_val;
}

int main(int argc, char * argv[]) {
	init();

	printw("=======================================\n");
	printw("======== AstroLab Rover Client ========\n");
	printw("=======================================\n");

	move(8, 0);

	connectToServer();

	int run = 1;
	int ch, x, y;
	int fwSpeed, lfSpeed;
	while (run) {
		// Print speed
		getyx(stdscr, y, x);

		move(3, 0); clrtoeol();
		mvprintw(3, 0, "Forward Speed: %d\n", forward);

		clrtoeol();
		if (left > 0) printw("Left Speed: %d\n", left);
		if (left < 0) printw("Right Speed: %d\n", -left);

		move (5, 0); clrtoeol();
		printw("Claw angle: %d\n", claw);

		clrtoeol();
		printw("Arm angle: %d\n", stepAngle);

		move(y, x);
		refresh();

		ch = getch();
		switch (ch) {
			case KEY_UP:
				if (forward < MAX_UP) forward++;
				break;
			case KEY_DOWN:
				if (forward > -MAX_UP) forward--;
				break;
			case KEY_LEFT:
				if (left < MAX_LEFT) left++;
				break;
			case KEY_RIGHT:
				if (left > -MAX_LEFT) left--;
				break;
			case 'd':
			case 'D':
				if (claw < 90) claw += 5;
				break;
			case 'a':
			case 'A':
				if (claw > 0) claw -= 5;
				break;
			case 'w':
			case 'W':
				if (stepAngle < 270) stepAngle += 9;
				break;
			case 's':
			case 'S':
				if (stepAngle > 0) stepAngle -= 9;
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
			case KEY_UP:
			case KEY_DOWN:
			case KEY_LEFT:
			case KEY_RIGHT:
				fwSpeed = ((double)forward / MAX_UP) * (1 << 12);
				lfSpeed = ((double)left / MAX_LEFT) * (1 << 12);

				if (left == 0) {
					if (forward > 0) sendCmd(CMD_FORWARD, fwSpeed);
					else if (forward < 0) sendCmd(CMD_BACKWARD, -fwSpeed);
					else sendCmd(CMD_FORWARD, 0);
				}
				else if (left > 0) sendCmd(CMD_LEFT, lfSpeed);
				else if (left < 0) sendCmd(CMD_RIGHT, -lfSpeed);
				break;
			case 'd':
			case 'D':
			case 'a':
			case 'A':
				sendCmd(CMD_CLAW, claw);
				break;
			case 'w':
			case 'W':
			case 's':
			case 'S':
				if (stepAngle - stepOldAngle) {
					sendCmd(CMD_STEP_STEP, (stepAngle - stepOldAngle) / 0.9);
					stepOldAngle = stepAngle;
					if (stepAngle == 0)
						sendCmd(CMD_STEP_RELEASE, 0);
				}
				break;
		}
	}

	finish(0);

	return 0;
}
