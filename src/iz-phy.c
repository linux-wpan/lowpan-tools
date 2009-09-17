/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008, 2009 Siemens AG
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
 *
 * Written-by:
 * Dmitry Eremin-Solenikov <dbaryshkov@gmail.com>
 * Sergey Lapin <slapin@ossfans.org>
 * Maxim Osipov <maxim.osipov@siemens.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include <ieee802154.h>
#include <nl802154.h>
#include <libcommon.h>

#include "iz.h"


/******************/
/* LIST handling  */
/******************/

static iz_res_t list_phy_parse(struct iz_cmd *cmd)
{
	if (cmd->argc > 2) {
		printf("Incorrect number of arguments!\n");
		return IZ_STOP_ERR;
	}

	/* iz list wpan0 */
	if (cmd->argc == 2) {
		cmd->iface = cmd->argv[1];
		cmd->flags = NLM_F_REQUEST;
	} else {
		/* iz list */
		cmd->iface = NULL;
		cmd->flags = NLM_F_REQUEST | NLM_F_DUMP;
	}

	return IZ_CONT_OK;
}

static iz_res_t list_phy_request(struct iz_cmd *cmd, struct nl_msg *msg)
{
	/* List single interface */
	if (cmd->iface)
		nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->iface);

	return IZ_CONT_OK;
}

static iz_res_t list_phy_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	char * dev_name;

	/* Check for mandatory attributes */
	if (!attrs[IEEE802154_ATTR_DEV_NAME] ||
	    !attrs[IEEE802154_ATTR_CHANNEL] ||
	    !attrs[IEEE802154_ATTR_PAGE])
		return IZ_STOP_ERR;

	/* Get attribute values from the message */
	dev_name = nla_get_string(attrs[IEEE802154_ATTR_DEV_NAME]);

	/* Display information about interface */
	printf("%-10s IEEE 802.15.4 PHY object\n", dev_name);
	printf("    page: %d  channel: %d\n",
			nla_get_u8(attrs[IEEE802154_ATTR_PAGE]),
			nla_get_u8(attrs[IEEE802154_ATTR_CHANNEL]));
	printf("\n");

	return (cmd->flags & NLM_F_MULTI) ? IZ_CONT_OK : IZ_STOP_OK;
}

static iz_res_t list_phy_finish(struct iz_cmd *cmd)
{
	return IZ_STOP_OK;
}


const struct iz_cmd_desc phy_commands[] = {
	{
		.name		= "listphy",
		.usage		= "[phy]",
		.doc		= "List phys(s).",
		.nl_cmd		= IEEE802154_LIST_PHY,
		.nl_resp	= IEEE802154_LIST_PHY,
		.parse		= list_phy_parse,
		.request	= list_phy_request,
		.response	= list_phy_response,
		.finish		= list_phy_finish,
	},
	{}
};

