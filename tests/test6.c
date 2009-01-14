#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ieee802154.h"

int main(int argc, char **argv) {
	int ret;
	struct ifreq req = {};

	char buf[] = {0x01, 0x80, 0xa5, 0x5a};
	int sd = socket(PF_IEEE80215, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	strncpy(req.ifr_name, argv[1] ?: "wpan0", IF_NAMESIZE);
	ret = ioctl(sd, SIOCGIFADDR, &req);
	if (ret < 0) {
		perror("ioctl: SIOCGIFADDR");
		return 1;
	}

	ret = bind(sd, &req.ifr_addr, sizeof(struct sockaddr_ieee80215));
	if (ret < 0)
		perror("bind");

	ret = connect(sd, &req.ifr_addr, sizeof(struct sockaddr_ieee80215));
	if (ret < 0)
		perror("connect");

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
