#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net/if.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "ieee802154.h"

#define IOREQ(cmd)			\
	ret = ioctl(sd, cmd, &req);	\
	if (ret)			\
		perror( #cmd );		\
	else

int printinfo(const char *ifname) {
	struct ifreq req;
	int ret;
	unsigned is_mac;
	int sd;

	sd = socket(PF_IEEE80215, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		goto out_noclose;
	}

	strcpy(req.ifr_name, ifname);
	ret = ioctl(sd, SIOCGIFHWADDR, &req);
	if (ret) {
		perror("SIOCGIFHWADDR");
		goto out;
	}

	if (req.ifr_hwaddr.sa_family == ARPHRD_IEEE80215_PHY) {
		is_mac = 0;
		printf("%-8s  IEEE 802.15.4 master (PHY) interface", req.ifr_name);
	} else if (req.ifr_hwaddr.sa_family == ARPHRD_IEEE80215) {
		unsigned char *addr = (unsigned char *)(req.ifr_hwaddr.sa_data);
		is_mac = 1;
		printf("%-8s  IEEE 802.15.4 MAC interface", req.ifr_name);
		printf("\n        ");
		printf("  HwAddr: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				addr[0], addr[1], addr[2], addr[3],
				addr[4], addr[5], addr[6], addr[7]
				);
	} else {
		printf("%-8s  not a IEEE 802.15.4 interface\n", req.ifr_name);
		goto out;
	}

	printf("\n        ");
	IOREQ(SIOCGIFFLAGS)
		printf("  Flags:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s ",
			(req.ifr_flags & IFF_UP) ? " up" : "",
			(req.ifr_flags & IFF_BROADCAST) ? " broadcast" : "",
			(req.ifr_flags & IFF_DEBUG) ? " debug" : "",
			(req.ifr_flags & IFF_LOOPBACK) ? " loopback" : "",
			(req.ifr_flags & IFF_POINTOPOINT) ? " pointopoint" : "",
			(req.ifr_flags & IFF_NOTRAILERS) ? " notrailers" : "",
			(req.ifr_flags & IFF_RUNNING) ? " running" : "",
			(req.ifr_flags & IFF_NOARP) ? " noarp" : "",
			(req.ifr_flags & IFF_PROMISC) ? " promisc" : "",
			(req.ifr_flags & IFF_ALLMULTI) ? " allmulti" : "",
			(req.ifr_flags & IFF_MASTER) ? " master" : "",
			(req.ifr_flags & IFF_SLAVE) ? " slave" : "",
			(req.ifr_flags & IFF_MULTICAST) ? " multicast" : "",
			(req.ifr_flags & IFF_PORTSEL) ? " portsel" : "",
			(req.ifr_flags & IFF_AUTOMEDIA) ? " automedia" : "",
			(req.ifr_flags & IFF_DYNAMIC) ? " dynamic" : ""
		      );
	printf("\n        ");
	IOREQ(SIOCGIFMTU)
		printf("  MTU: %d", req.ifr_mtu);
	IOREQ(SIOCGIFTXQLEN)
		printf("  TxQueue: %d", req.ifr_mtu);
	if (is_mac) {
		ret = ioctl(sd, SIOCGIFADDR, &req);
		if (ret) {
			if (errno == EADDRNOTAVAIL)
				printf("  Not Associated");
			else
				perror("SIOCGIFADDR");
		} else {
			struct sockaddr_ieee80215 *sa = (struct sockaddr_ieee80215 *)&req.ifr_addr;
			printf("  PAN ID: %04x  Addr: %04x\n", sa->pan_id, sa->short_addr);
		}
	}
	printf("\n\n");

	ret = close(sd);
	if (ret != 0) {
		perror("close");
		goto out_noclose;
	}

	return 0;

out:
	close(sd);
out_noclose:
	return 1;
}

int main(int argc, char **argv) {
	return printinfo(argv[1] ? : "wpan0");
}

