/*
 * serial.c - trivial serial port program
 * Copyright (C) 2008 Sean MacLennan
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this project; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/poll.h>


static int set_speed(int speed)
{
	struct {
		int speed;
		int cflag;
	} speeds[] = {
		{    300, B300    },
		{   1200, B1200   },
		{   2400, B2400   },
		{   4800, B4800   },
		{   9600, B9600   },
		{  19200, B19200  },
		{  38400, B38400  },
		{  57600, B57600  },
		{ 115200, B115200 }
	};
	int i;

	for(i = 0; i < sizeof(speeds) / sizeof(*speeds); ++i)
		if(speeds[i].speed == speed)
			return speeds[i].cflag;

	printf("Invalid speed %d\n", speed);
	exit(1);
}


static struct termios oldtty, oldserial;
static int fd;

static void cleanup(void)
{   // Called at exit.
	tcsetattr(0,  TCSANOW, &oldtty); // restore stdin
	tcsetattr(fd, TCSANOW, &oldserial); // restore serial port
	close(fd);

	puts("\n\nBye!");
}


static void handle_stdin(void)
{
	static int exit_count = 0;
	char c;
	int n;

	read(0, &c, 1);

	// Two CTRL-Zs exits
	if(c == 0x1a) {
		if(++exit_count == 1) return;
		if(exit_count > 1) exit(0);
	} else if(exit_count == 1) {
		char c1 = 0x1a;
		write(fd, &c1, 1); //we should check return
	}
	exit_count = 0;

	if((n = write(fd, &c, 1)) != 1) {
		if(n < 0)
			perror("write serial");
		else
			printf("Unable to write to serial port\n");
	}
}


static void handle_serial(void)
{
	char c;
	int n;

	if((n = read(fd, &c, 1)) == 1)
		write(1, &c, 1);
	else if(n == 0) {
		printf("EOF on serial port\n");
		exit(1);
	} else
		perror("read serial");
}


int main(int argc, char *argv[])
{
	int c;
	char *device = "/dev/ttyUSB0"; // hardcoded for lappy
	int speed = B115200;
	struct pollfd ufds[2];
	static struct termios settty;

	while((c = getopt(argc, argv, "b:")) != EOF)
		switch(c) {
		case 'b': speed = set_speed(strtol(optarg, 0, 0)); break;
		default: puts("Sorry!"); exit(1);
		}

	if(optind < argc) device = argv[optind];

	if((fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
		perror(device);
		exit(1);
	}

	// Don't process signals on stdin
	tcgetattr(0, &oldtty);
	tcgetattr(0, &settty);
	settty.c_lflag = 0;
	tcsetattr(0, TCSANOW, &settty);

	/* Do a more complete setup on the serial device */
	tcgetattr(fd, &oldserial);
	tcgetattr(fd, &settty);
	settty.c_cflag = speed | CS8 | CREAD;
	settty.c_iflag = IGNPAR;
	settty.c_oflag = 0;
	settty.c_lflag = 0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &settty);

	ufds[0].fd = 0;
	ufds[0].events = POLLIN;
	ufds[1].fd = fd;
	ufds[1].events = POLLIN;

	atexit(cleanup);

	while(1)
		if(poll(ufds, 2, -1) > 0) {
			if(ufds[0].revents) handle_stdin();
			if(ufds[1].revents) handle_serial();
		} else
			perror("poll");
}
