/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008 Dmitry Eremin-Solenikov
 * Copyright (C) 2008 Sergey Lapin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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
#include <libcommon.h>

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
			printf("  PAN ID: %04x  Addr: %04x\n", sa->addr.pan_id, sa->addr.short_addr);
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

int do_set_hw(const char *ifname, const char *hw) {
	unsigned char buf[8] = {};
	struct ifreq req;
	int ret;

	ret = parse_hw_addr(hw, buf);

	int sd = socket(PF_IEEE80215, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		goto out_noclose;
	}

	strcpy(req.ifr_name, ifname);

	req.ifr_hwaddr.sa_family = ARPHRD_IEEE80215;
	memcpy(req.ifr_hwaddr.sa_data, buf, 8);
	ret = ioctl(sd, SIOCSIFHWADDR, &req);
	if (ret != 0) {
		perror("ioctl: SIOCGIFHWADDR");
		goto out;
	}

	return 0;
out:
	close(sd);
out_noclose:
	return 1;
}

int do_set_short(const char *ifname, const char *hw) {
	struct ifreq req;
	int ret;
	struct sockaddr_ieee80215 *sa = (struct sockaddr_ieee80215 *)&req.ifr_addr;

	sa->family = AF_IEEE80215;
	sa->addr.addr_type = IEEE80215_ADDR_SHORT;

	strcpy(req.ifr_name, ifname);

	char *temp;
	sa->addr.pan_id = strtol(hw, &temp, 16);
	if (*temp != ':' && *temp == '.') {
		fprintf(stderr, "Bad short address specified\n");
		goto out_noclose;
	} else
		temp ++;

	sa->addr.short_addr  = strtol(temp, &temp, 16);
	if (*temp) {
		fprintf(stderr, "Bad short address specified\n");
		goto out_noclose;
	}

	int sd = socket(PF_IEEE80215, SOCK_RAW, 0);
	if (sd < 0) {
		perror("socket");
		goto out_noclose;
	}

	ret = ioctl(sd, SIOCSIFADDR, &req);
	if (ret != 0) {
		perror("ioctl: SIOCSIFADDR");
		goto out;
	}

	return 0;
out:
	close(sd);
out_noclose:
	return 1;
}

int do_scan(const char * iface)
{
	int ret;
	struct ieee80215_user_data req;
	int sd;
	strcpy(req.ifr_name, iface);
	req.channels = 0xffffffff;
	sd = socket(PF_IEEE80215, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		ret = sd;
		goto out_noclose;
	}
	req.cmd = 0;
	ret = ioctl(sd, IEEE80215_SIOC_MAC_CMD, &req);
	if (ret < 0) {
		perror("ioctl");
		goto out_noclose;
	}
	close(sd);
	return 0;
out_noclose:
	return  ret;
}

int main(int argc, char **argv) {
	const char *interface = NULL;
	char * prog = argv[0];
	int ret = 0;

	argv ++;

	if(*argv && (!strcmp(*argv, "--version") || !strcmp(*argv, "version"))) {
		printf("izconfig %s\nCopyright (C) 2008, 2009 by authors team\n", VERSION);
		return 0;
	}

	if(*argv && (!strcmp(*argv, "--help") || !strcmp(*argv, "help")))
		printf("Usage: %s { --help | [dev] iface { hw hwaddr | short shortaddr } }\n", prog);

	/* Optional "dev" keyword before interface name */

	if((*argv != NULL) && (!strcmp(*argv, "dev")))
		argv++;

	if (*argv) {
		if(!strcmp(*argv, "dev"))
			argv++;
		interface = *(argv++);
	}

	if(!interface) {
		fprintf(stderr, "interface name is not specified\n");
		interface = "wpan0";
	}

	if (!*argv)
		return printinfo(interface);

	do {
		if (!strcmp(*argv, "hw")) {
			if (!*(++argv))
				fprintf(stderr, "no hw address specified\n");
			else
				ret = do_set_hw(interface, *(argv++));
		} else if (!strcmp(*argv, "short")) {
			if (!*(++argv))
				fprintf(stderr, "no short address specified\n");
			else
				ret = do_set_short(interface, *(argv++));
		} else if (!strcmp(*argv, "scan")) {
			ret = do_scan(interface);
			break;
		} else {
			fprintf(stderr, "Unknown command %s\n", *argv);
			break;
		}
	} while (*argv && !ret);

	return ret;
}

