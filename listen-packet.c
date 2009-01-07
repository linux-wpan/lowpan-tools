#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <linux/sockios.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ieee802154.h"
#include "libcommon.h"

int main(int argc, char **argv) {
	int ret;
	char buf[256];
	struct sockaddr_ll sa = {};
	struct ifreq req = {};

	int sd = socket(PF_PACKET, (argv[2] && argv[2][0] == 'd') ? SOCK_DGRAM : SOCK_RAW , htons(ETH_P_ALL));
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	strncpy(req.ifr_name, argv[1], IF_NAMESIZE);
	ret = ioctl(sd, SIOCGIFINDEX, &req);
	if (ret < 0)
		perror("ioctl: SIOCGIFINDEX");

	sa.sll_family = AF_PACKET;
	sa.sll_ifindex = req.ifr_ifindex;
	ret = bind(sd, (struct sockaddr*)&sa, sizeof(sa));
	if (ret < 0)
		perror("bind");

	do {
		struct sockaddr_storage sas;
		socklen_t saslen = sizeof(sas);
		ret = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&sas, &saslen);
		if (ret < 0)
			perror("recv");
		else {
			printf("packet len %d (%x)\n", ret, ret);
			printbuf(buf, ret);
		}
	} while (ret >= 0);

	ret = shutdown(sd, SHUT_RDWR);
	if (ret < 0)
		perror("shutdown");

	ret = close(sd);
	if (ret < 0)
		perror("close");

	return 0;

}

