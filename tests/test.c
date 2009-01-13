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
	char *iface = argv[1] ?: "mwpan0";

	char buf[] = {0x40, 0x00, 0x56};
	int sd = socket(PF_IEEE80215, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	ret = setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, iface, strlen(iface) + 1);
	if (ret < 0)
		perror("setsockopt: BINDTODEVICE");

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
