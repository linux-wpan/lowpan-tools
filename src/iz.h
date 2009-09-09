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

#ifndef IZ_H
#define IZ_H

struct iz_cmd;
struct nl_msg;
struct nlattr;
struct genlmsghdr;

typedef enum {
	IZ_CONT_OK,
	IZ_STOP_OK,
	IZ_STOP_ERR,
} iz_res_t;

/* iz command descriptor */
struct iz_cmd_desc {
	const char *name;	/* Name (as in command line) */
	const char *usage;	/* Arguments list */
	const char *doc;	/* One line command description */
	unsigned char nl_cmd;	/* NL command ID */
	unsigned char nl_resp;	/* NL command response ID (optional) */

	/* Parse command line, fill in iz_cmd struct. */
	/* You must set cmd->flags here! */
	iz_res_t (*parse)(struct iz_cmd *cmd);

	/* Prepare an outgoing netlink message */
	/* Returns 0 in case of success, in other case error */
	/* If request is not defined, we go to receive wo sending message */
	iz_res_t (*request)(struct iz_cmd *cmd, struct nl_msg *msg);

	/* Handle an incoming netlink message */
	/* Returns 1 to stop with error, -1 to stop with Ok; 0 to continue */
	iz_res_t (*response)(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

	/* Print detailed help information */
	void (*help)(void);
};

/* Parsed command results */
struct iz_cmd {
	int argc;	/* number of arguments to the command */
	char **argv;	/* NULL-terminated arrays of arguments */

	struct iz_cmd_desc *desc;

	/* Fields below are prepared by parse function */
	struct iz_cmd_desc *listener;	/* Do not look at nl_resp, listen for all messages */
	int flags;	/* NL message flags */
	int cmd;	/* NL command ID */
	char *iface;	/* Interface for a command */
};

extern struct iz_cmd_desc mac_commands[];

#endif
