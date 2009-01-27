#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/if.h>

#include <ieee802154.h>
#include <libcommon.h>

int main(int argc, char **argv) {
	int sd;
	int ret;
	struct sockaddr_ieee80215 a;

	if (argc != 4) {
		printf("Usage: %s PANid sourceAddr destAddr\n", argv[0]);
		return 1;
	}

	sd = socket(PF_IEEE80215, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	a.family = AF_IEEE80215;
	a.addr.addr_type = IEEE80215_ADDR_SHORT;

	a.addr.pan_id = strtol(argv[1], NULL, 16);

	a.addr.short_addr = (strtol(argv[2], NULL, 16));
	ret = bind(sd, (struct sockaddr *)&a, sizeof(a));
	if (ret) {
		perror("bind");
		return 1;
	}

	a.addr.short_addr = (strtol(argv[3], NULL, 16));
	ret = connect(sd, (struct sockaddr *)&a, sizeof(a));
	if (ret) {
		perror("connect");
		return 1;
	}

	while (1) {
		fd_set rs, ws, xs;
		char buf[80];
		int r, w;

		FD_ZERO(&rs);
		FD_ZERO(&ws);
		FD_ZERO(&xs);
		FD_SET(sd, &rs);
//		FD_SET(sd, &ws);
		FD_SET(sd, &xs);

		FD_SET(0, &rs);
//		FD_SET(1, &ws);

		ret = select(sd + 1, &rs, &ws, &xs, NULL);
		if (FD_ISSET(0, &rs)) {
			r = 0;
			w = sd;
		} else if (FD_ISSET(sd, &rs)) {
			r = sd;
			w = 1;
			write(w, "> ", 2);
		} else {
			printf("Something bad happened!\n");
			continue;
		}

		ret = read(r, &buf, sizeof(buf));
		if (ret < 0) {
			perror("read");
		} else if (ret > 0) {
			write(w, &buf, ret);
		}
	}
	return 0;
}
