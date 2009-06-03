/*
 * IEEE802.15.4-2003 implementation user-space header.
 *
 * Copyright (C) 2008, 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
 * Written-by: Sergey Lapin
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

#ifdef HAVE_NET_IEEE802154_H
#include <net/ieee802154.h>
#endif
#ifdef HAVE_NET_AF_IEEE802154_H
#include <net/af_ieee802154.h>
#endif

#ifndef IEEE802154_ADDR_LEN
#define IEEE802154_ADDR_LEN 8
#endif

#include <sys/socket.h>
#include <stdint.h>
#ifndef HAVE_STRUCT_SOCKADDR_IEEE802154

enum {
	IEEE802154_ADDR_NONE = 0x0,
	// RESERVER = 0x01,
	IEEE802154_ADDR_SHORT = 0x2, /* 16-bit address + PANid */
	IEEE802154_ADDR_LONG = 0x3, /* 64-bit address + PANid */
};

struct ieee802154_addr {
	int addr_type;
	uint16_t pan_id;
	union {
		uint8_t hwaddr[IEEE802154_ADDR_LEN];
		uint16_t short_addr;
	};
};

struct sockaddr_ieee802154 {
	sa_family_t family; /* AF_IEEE802154 */
	struct ieee802154_addr addr;
};
#endif

#ifndef N_IEEE802154
#define N_IEEE802154 19
#endif

#ifndef PF_IEEE802154
#define PF_IEEE802154 36
#define AF_IEEE802154 PF_IEEE802154
#endif

#include <net/if_arp.h>
#ifndef ARPHRD_IEEE802154
#define ARPHRD_IEEE802154	  804
#define ARPHRD_IEEE802154_PHY	  805
#endif

#ifndef IEEE802154_MAC_SCAN_ED
#define IEEE802154_MAC_SCAN_ED		0x0
#define IEEE802154_MAC_SCAN_ACTIVE	0x1
#define IEEE802154_MAC_SCAN_PASSIVE	0x2
#define IEEE802154_MAC_SCAN_ORPHAN	0x3
#endif

/* PF_IEEE802154, SOCK_DGRAM */
#define IEEE802154_SIOC_NETWORK_DISCOVERY	(SIOCPROTOPRIVATE + 0)
#define IEEE802154_SIOC_NETWORK_FORMATION	(SIOCPROTOPRIVATE + 1)
#define IEEE802154_SIOC_PERMIT_JOINING		(SIOCPROTOPRIVATE + 2)
#define IEEE802154_SIOC_START_ROUTER		(SIOCPROTOPRIVATE + 3)
#define IEEE802154_SIOC_JOIN			(SIOCPROTOPRIVATE + 4)
#define IEEE802154_SIOC_MAC_CMD			(SIOCPROTOPRIVATE + 5)

/* master device */
#define IEEE802154_SIOC_ADD_SLAVE		(SIOCDEVPRIVATE + 0)

#include <linux/if_ether.h>
#ifndef ETH_P_IEEE802154
#define ETH_P_IEEE802154 0x00F6		/* IEEE802.15.4 frame		*/
#endif
#ifndef HAVE_STRUCT_USER_DATA
#ifndef IFNAMSIZE
#include <net/if.h>
#endif
struct ieee802154_user_data {
	/* This is used as ifr_name */
	union
	{
		char	ifrn_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	} ifr_ifrn;
	int channels;
	int channel;
	int duration;
	int rejoin;
	int rxon;
	int as_router;
	int power;
	int mac_security;
	int16_t panid;
	int cmd;
//	struct ieee802154_dev_address addr; /**< Peer address */
};
#endif
#endif
