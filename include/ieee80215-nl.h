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

enum {
	IEEE80215_ATTR_SHORT_ADDR,
	IEEE80215_ATTR_HW_ADDR,
	IEEE80215_ATTR_PAN_ID,
	IEEE80215_ATTR_DEV_NAME,
	__IEEE80215_ATTR_MAX,
};

#define IEEE80215_ATTR_MAX (__IEEE80215_ATTR_MAX - 1)

#ifdef IEEE80215_NL_WANT_POLICY
static struct nla_policy ieee80215_policy[IEEE80215_ATTR_MAX + 1] = {
	[IEEE80215_ATTR_SHORT_ADDR] = { .type = NLA_U16, },
	[IEEE80215_ATTR_HW_ADDR] = { .type = NLA_U64, },
	[IEEE80215_ATTR_PAN_ID] = { .type = NLA_U16, },
	[IEEE80215_ATTR_DEV_NAME] = { .type = NLA_STRING, },
};
#endif

/* commands */
/* REQ should be responded with CONF
 * and INDIC with RESP
 */
enum {
	IEEE80215_ASSOCIATE_REQ,
	IEEE80215_ASSOCIATE_CONF,
	IEEE80215_DISASSOCIATE_REQ,
	IEEE80215_DISASSOCIATE_CONF,

	IEEE80215_ASSOCIATE_INDIC,
	IEEE80215_ASSOCIATE_RESP,
	IEEE80215_DISASSOCIATE_INDIC,

	__IEEE80215_CMD_MAX,
};

#define IEEE80215_CMD_MAX (__IEEE80215_CMD_MAX - 1)


#ifdef __KERNEL__
int ieee80215_nl_init(void);
void ieee80215_nl_exit(void);
#endif

#endif
