#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "ieee802154.h"

int main() {
	int ret;
	struct sockaddr_ieee80215 sa = {};

	char buf[] = {0x40, 0x00, 0x56};
	int sd = socket(PF_IEEE80215, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	sa.family = AF_IEEE80215;
//	sa.ifindex = 2;
	sa.addr = 0xbebafecaafbeaddeLL;
	ret = bind(sd, (struct sockaddr*)&sa, sizeof(sa));
	if (ret < 0)
		perror("bind");

	ret = send(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		perror("send");

	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		perror("recv");

	ret = shutdown(sd, SHUT_RDWR);
	if (ret < 0)
		perror("shutdown");

	ret = close(sd);
	if (ret < 0)
		perror("close");

	return 0;

}
