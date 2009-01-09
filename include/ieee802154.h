/*
 * IEEE802.15.4-2003 implementation user-space header.
 * Copyright (C) 2008 Dmitry Eremin-Solenikov
 * Copyright (C) 2008 Sergey Lapin
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __IEEE802154_H
#define __IEEE802154_H

#ifdef HAVE_NET_IEEE80215_H
#include <net/ieee80215.h>
#endif
#ifdef HAVE_NET_AF_IEEE80215_H
#include <net/af_ieee80215.h>
#endif

#include <sys/socket.h>
#include <stdint.h>
#ifndef HAVE_STRUCT_SOCKADDR_IEEE80215
enum {
	IEEE80215_ADDR_LONG, /* 64-bit address */
	IEEE80215_ADDR_SHORT, /* 16-bit address + PANid */
	IEEE80215_ADDR_IFINDEX, /* interface index, mainly for debugging only */
};

struct sockaddr_ieee80215 {
	sa_family_t family; /* AF_IEEE80215 */
	int addr_type;
	union {
		uint64_t hwaddr;
		struct {
			uint16_t pan_id;
			uint16_t short_addr;
		};
		int ifindex;
	};
};
#endif

#ifndef N_IEEE80215
#define N_IEEE80215 19
#endif

#ifndef PF_IEEE80215
#define PF_IEEE80215 36
#define AF_IEEE80215 PF_IEEE80215
#endif

#include <net/if_arp.h>
#ifndef ARPHRD_IEEE80215
#define ARPHRD_IEEE80215	  804
#define ARPHRD_IEEE80215_PHY	  805
#endif

/* PF_IEEE80215, SOCK_DGRAM */
#define IEEE80215_SIOC_NETWORK_DISCOVERY	(SIOCPROTOPRIVATE + 0)
#define IEEE80215_SIOC_NETWORK_FORMATION	(SIOCPROTOPRIVATE + 1)
#define IEEE80215_SIOC_PERMIT_JOINING		(SIOCPROTOPRIVATE + 2)
#define IEEE80215_SIOC_START_ROUTER		(SIOCPROTOPRIVATE + 3)
#define IEEE80215_SIOC_JOIN			(SIOCPROTOPRIVATE + 4)

/* master device */
#define IEEE80215_SIOC_ADD_SLAVE		(SIOCDEVPRIVATE + 0)

#include <linux/if_ether.h>
#ifndef ETH_P_IEEE80215
#define ETH_P_IEEE80215 0x00F6		/* IEEE802.15.4 frame		*/
#endif

#endif
