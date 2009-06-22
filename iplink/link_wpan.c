/*
 * Copyright (C) 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
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

#include <stdint.h>
#include <ieee802154.h>

#include <netlink/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/mac802154.h>
#include "iplink.h"

static void wpan_print_opt(struct link_util *lu, FILE *f,
			struct rtattr *tb[])
{
	if (!tb)
		return;

	if (tb[IFLA_WPAN_CHANNEL]) {
		uint16_t *chan = RTA_DATA(tb[IFLA_WPAN_CHANNEL]);
		fprintf(f, "chan %d", *chan);
	}

	if (tb[IFLA_WPAN_PAN_ID]) {
		uint16_t *panid = RTA_DATA(tb[IFLA_WPAN_PAN_ID]);
		fprintf(f, "pan %04x ", *panid);
	}

	if (tb[IFLA_WPAN_SHORT_ADDR]) {
		uint16_t *addr = RTA_DATA(tb[IFLA_WPAN_SHORT_ADDR]);
		fprintf(f, "addr %04x ", *addr);
	}

	if (tb[IFLA_WPAN_COORD_SHORT_ADDR]) {
		uint16_t *coord = RTA_DATA(tb[IFLA_WPAN_COORD_SHORT_ADDR]);
		fprintf(f, "coord %04x ", *coord);
	}

	if (tb[IFLA_WPAN_COORD_EXT_ADDR]) {
		char buf[24];
		fprintf(f, "%s\n", ll_addr_n2a(
				RTA_DATA(tb[IFLA_WPAN_COORD_EXT_ADDR]),
				RTA_PAYLOAD(tb[IFLA_WPAN_COORD_EXT_ADDR]),
				ARPHRD_IEEE802154, buf, sizeof(buf)));
	}
}

struct link_util wpan_link_util = {
	.id		= "wpan",
	.maxattr	= IFLA_WPAN_MAX,
	.print_opt	= wpan_print_opt,
};

