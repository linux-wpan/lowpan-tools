#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define N_ZB 19

int main(int argc, char **argv) {
	int fd, ret;

	if (argc != 2) {
		fprintf(stderr, "usage: %s SERIAL_DEV\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("open");
		return 2;
	}

	struct termios tbuf;

	memset(&tbuf, 0, sizeof(tbuf));

	tbuf.c_iflag |= IGNBRK;		/* Ignores break condition */
	tbuf.c_iflag |= INPCK;		/* Enable input parity check */

	tbuf.c_cflag |= CS8;		/* 8 bits */
	tbuf.c_cflag |= CREAD;		/* Enables receiver */
	tbuf.c_cflag |= CRTSCTS;	/* not in POSIX. Enable RTS/CTS (hardware) flow control. requires _BSD_SOURCE or _SVID_SOURCE */
	/*
	tbuf.c_cflag |= CLOCAL;
	tbuf.c_lflag = 0;
	tbuf.c_cc[0] = 0;
	tbuf.c_cc[1] = 0;
	tbuf.c_cc[2] = 0;
	tbuf.c_cc[3] = 0;
	tbuf.c_cc[4] = 0;
	tbuf.c_cc[5] = 0;
	tbuf.c_cc[6] = 0;
	tbuf.c_cc[7] = 0;
	tbuf.c_cc[VMIN] = 1;
	*/
	cfsetospeed(&tbuf, B115200);
	cfsetispeed(&tbuf, B115200);

	if (tcsetattr(fd, TCSANOW, &tbuf) < 0) {
		perror("tcsetattr");
		return 3;
	}
	memset(&tbuf, 0, sizeof(tbuf));
	if (tcgetattr(fd, &tbuf) < 0) {
		perror("tcgetattr");
		return 4;
	}
	if (0 == (tbuf.c_cflag & CRTSCTS)) {
		fprintf(stderr, "failed to set CRTSCTS\n");
		return 5;
	}

	int arg;
	arg = N_ZB;
	ret = ioctl(fd, TIOCSETD, &arg);
	if (ret < 0) {
		perror("ioctl: TIOCSETD");
		return 6;
	}

	while (1);

	close(fd);

	return 0;
}
