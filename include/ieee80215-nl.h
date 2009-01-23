/*
 * ieee80215_nl.h
 *
 * Copyright (C) 2007, 2008 Siemens AG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef IEEE80215_NL_H
#define IEEE80215_NL_H

#define IEEE80215_NL_NAME "802.15.4 MAC"
#define IEEE80215_MCAST_COORD_NAME "coordinator"

enum {
	__IEEE80215_ATTR_INVALID,

	IEEE80215_ATTR_DEV_NAME,
	IEEE80215_ATTR_DEV_INDEX,

	IEEE80215_ATTR_STATUS,

	IEEE80215_ATTR_SHORT_ADDR,
	IEEE80215_ATTR_HW_ADDR,
	IEEE80215_ATTR_PAN_ID,

	IEEE80215_ATTR_CHANNEL,

	IEEE80215_ATTR_COORD_SHORT_ADDR,
	IEEE80215_ATTR_COORD_HW_ADDR,
	IEEE80215_ATTR_COORD_PAN_ID,

	IEEE80215_ATTR_SRC_SHORT_ADDR,
	IEEE80215_ATTR_SRC_HW_ADDR,
	IEEE80215_ATTR_SRC_PAN_ID,

	IEEE80215_ATTR_DEST_SHORT_ADDR,
	IEEE80215_ATTR_DEST_HW_ADDR,
	IEEE80215_ATTR_DEST_PAN_ID,

	IEEE80215_ATTR_CAPABILITY, // FIXME: this is association
	IEEE80215_ATTR_REASON,
	IEEE80215_ATTR_SCAN_TYPE,
	IEEE80215_ATTR_CHANNELS,
	IEEE80215_ATTR_DURATION,

	__IEEE80215_ATTR_MAX,
};

#define IEEE80215_ATTR_MAX (__IEEE80215_ATTR_MAX - 1)
#define NLA_HW_ADDR	NLA_U64
#define NLA_GET_HW_ADDR(attr, addr) do { u64 _temp = nla_get_u64(attr); memcpy(addr, &_temp, 8); } while (0)
#define NLA_PUT_HW_ADDR(msg, attr, addr) do { u64 _temp; memcpy(&_temp, addr, 8); NLA_PUT_U64(msg, attr, _temp); } while (0)

#ifdef IEEE80215_NL_WANT_POLICY
static struct nla_policy ieee80215_policy[IEEE80215_ATTR_MAX + 1] = {
	[IEEE80215_ATTR_DEV_NAME] = { .type = NLA_STRING, },
	[IEEE80215_ATTR_DEV_INDEX] = { .type = NLA_U32, },

	[IEEE80215_ATTR_STATUS] = { .type = NLA_U8, },
	[IEEE80215_ATTR_SHORT_ADDR] = { .type = NLA_U16, },
	[IEEE80215_ATTR_HW_ADDR] = { .type = NLA_HW_ADDR, },
	[IEEE80215_ATTR_PAN_ID] = { .type = NLA_U16, },
	[IEEE80215_ATTR_CHANNEL] = { .type = NLA_U8, },
	[IEEE80215_ATTR_COORD_SHORT_ADDR] = { .type = NLA_U16, },
	[IEEE80215_ATTR_COORD_HW_ADDR] = { .type = NLA_HW_ADDR, },
	[IEEE80215_ATTR_COORD_PAN_ID] = { .type = NLA_U16, },
	[IEEE80215_ATTR_SRC_SHORT_ADDR] = { .type = NLA_U16, },
	[IEEE80215_ATTR_SRC_HW_ADDR] = { .type = NLA_HW_ADDR, },
	[IEEE80215_ATTR_SRC_PAN_ID] = { .type = NLA_U16, },
	[IEEE80215_ATTR_DEST_SHORT_ADDR] = { .type = NLA_U16, },
	[IEEE80215_ATTR_DEST_HW_ADDR] = { .type = NLA_HW_ADDR, },
	[IEEE80215_ATTR_DEST_PAN_ID] = { .type = NLA_U16, },

	[IEEE80215_ATTR_CAPABILITY] = { .type = NLA_U8, },
	[IEEE80215_ATTR_REASON] = { .type = NLA_U8, },
	[IEEE80215_ATTR_SCAN_TYPE] = { .type = NLA_U8, },
	[IEEE80215_ATTR_CHANNELS] = { .type = NLA_U32, },
	[IEEE80215_ATTR_DURATION] = { .type = NLA_U8, },
};
#endif

/* commands */
/* REQ should be responded with CONF
 * and INDIC with RESP
 */
enum {
	__IEEE80215_COMMAND_INVALID,

	IEEE80215_ASSOCIATE_REQ,
	IEEE80215_ASSOCIATE_CONF,
	IEEE80215_DISASSOCIATE_REQ,
	IEEE80215_DISASSOCIATE_CONF,
	IEEE80215_GET_REQ,
	IEEE80215_GET_CONF,
//	IEEE80215_GTS_REQ,
//	IEEE80215_GTS_CONF,
	IEEE80215_RESET_REQ,
	IEEE80215_RESET_CONF,
//	IEEE80215_RX_ENABLE_REQ,
//	IEEE80215_RX_ENABLE_CONF,
	IEEE80215_SCAN_REQ,
	IEEE80215_SCAN_CONF,
	IEEE80215_SET_REQ,
	IEEE80215_SET_CONF,
	IEEE80215_START_REQ,
	IEEE80215_START_CONF,
	IEEE80215_SYNC_REQ,
	IEEE80215_POLL_REQ,
	IEEE80215_POLL_CONF,

	IEEE80215_ASSOCIATE_INDIC,
	IEEE80215_ASSOCIATE_RESP,
	IEEE80215_DISASSOCIATE_INDIC,
	IEEE80215_BEACON_NOTIFY_INDIC,
//	IEEE80215_GTS_INDIC,
	IEEE80215_ORPHAN_INDIC,
	IEEE80215_ORPHAN_RESP,
	IEEE80215_COMM_STATUS_INDIC,
	IEEE80215_SYNC_LOSS_INDIC,

	__IEEE80215_CMD_MAX,
};

#define IEEE80215_CMD_MAX (__IEEE80215_CMD_MAX - 1)


#ifdef __KERNEL__
int ieee80215_nl_init(void);
void ieee80215_nl_exit(void);

int ieee80215_nl_assoc_indic(struct net_device *dev, struct ieee80215_addr *addr, u8 cap);
int ieee80215_nl_assoc_confirm(struct net_device *dev, u16 short_addr, u8 status);
int ieee80215_nl_disassoc_indic(struct net_device *dev, struct ieee80215_addr *addr, u8 reason);
int ieee80215_nl_disassoc_confirm(struct net_device *dev, u8 status);
#endif

#endif
