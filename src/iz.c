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

#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <getopt.h>

#include <ieee802154.h>
#define u64 uint64_t
#include <nl802154.h>
#include <libcommon.h>


#define IZ_CONT_OK	0
#define IZ_STOP_OK	1
#define IZ_STOP_ERR	2

struct iz_cmd;

static int iz_cb_seq_check(struct nl_msg *msg, void *arg);
static int iz_cb_valid(struct nl_msg *msg, void *arg);
static void iz_help(void);

/* Command specific declarations */

/* HELP */
static int help_parse(struct iz_cmd *cmd, int argc, char **argv);

/* LIST */
static int list_parse(struct iz_cmd *cmd, int argc, char **argv);
static int list_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv);
static int list_response(struct genlmsghdr *ghdr, struct nlattr **attrs);

/* EVENT */
static int event_parse(struct iz_cmd *cmd, int argc, char **argv);
static int event_response(struct genlmsghdr *ghdr, struct nlattr **attrs);

/* TODO: ADD */

/* TODO: DEL */

/* SCAN */
static int scan_parse(struct iz_cmd *cmd, int argc, char **argv);
static int scan_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv);
static int scan_response(struct genlmsghdr *ghdr, struct nlattr **attrs);

/* ASSOCIATE */
static int assoc_parse(struct iz_cmd *cmd, int argc, char **argv);
static int assoc_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv);
static int assoc_response(struct genlmsghdr *ghdr, struct nlattr **attrs);

/* DISASSOCIATE */
static int disassoc_parse(struct iz_cmd *cmd, int argc, char **argv);
static int disassoc_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv);
static int disassoc_response(struct genlmsghdr *ghdr, struct nlattr **attrs);


/* Parsed command results */
struct iz_cmd {
	int argn;	/* Filled in by main, points to command in argv */

	/* Fields below are prepared by parse function */
	int listen_idx;	/* Index to iz_commands */
			/* Do not look at nl_resp, listen for all messages */
	int flags;	/* NL message flags */
	int cmd;	/* NL command ID */
	char *iface;	/* Interface for a command */
};

/* iz command descriptor */
struct iz_cmd_desc {
	const char *name;	/* Name (as in command line) */
	const char *usage;	/* Arguments list */
	const char *doc;	/* One line command description */
	int type;		/* Command type */
	int nl_cmd;		/* NL command ID */
	int nl_resp;		/* NL command response ID (optional) */

	/* Parse command line, fill in iz_cmd struct. */
	/* You must set cmd->flags here! */
	/* Returns 1 to stop with error, -1 to stop with Ok; 0 to continue */
	int (*parse)(struct iz_cmd *cmd, int argc, char **argv);

	/* Prepare an outgoing netlink message */
	/* Returns 0 in case of success, in other case error */
	/* If request is not defined, we go to receive wo sending message */
	int (*request)(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv);

	/* Handle an incoming netlink message */
	/* Returns 1 to stop with error, -1 to stop with Ok; 0 to continue */
	int (*response)(struct genlmsghdr *ghdr, struct nlattr **attrs);

	/* Print detailed help information */
	void (*help)(void);
};

/* Descriptors for command type */
struct iz_cmd_type {
	int type;
	const char * doc;
};

/* Version */
const char *iz_version = "iz 0.3; Copyright (C) 2008, 2009 Siemens AG";

/* Options description */
static const struct option iz_long_opts[] = {
	{
			.name = "debug",
			.has_arg = 2,
			.flag = NULL,
			.val = 0,
	},
	{
			.name = "version",
			.has_arg = 0,
			.flag = NULL,
			.val = 0,
	},
	{
			.name = "help",
			.has_arg = 0,
			.flag = NULL,
			.val = 0,
	},
	{ 0, 0, 0, 0 }
};
#define DEBUG_NUM	0
#define VERSION_NUM	1
#define HELP_NUM	2

/* Expected sequence number */
static int iz_seq = 0;

/* Parsed options */
static int iz_debug = 0;

/* Command classes (device types) */
#define IZ_C_COMMON	0x01
#define IZ_C_PHY	0x02
#define IZ_C_MAC	0x04
#define IZ_C_ZB		0x08

/* Command descriptors */
static struct iz_cmd_desc iz_commands[] = {
	{
		.name	= "help",
		.usage	= "[command]",
		.doc	= "Print detailed help for a command.",
		.type	= IZ_C_COMMON,
		.parse	= help_parse,
	},
	{
		.name	= "list",
		.usage	= "[iface]",
		.doc	= "List interface(s).",
		.type	= IZ_C_COMMON,
		.nl_cmd		= IEEE802154_LIST_IFACE,
		.nl_resp	= IEEE802154_LIST_IFACE,
		.parse		= list_parse,
		.request	= list_request,
		.response	= list_response,
		.help		= NULL,
	},
	{
		.name	= "event",
		.usage	= "",
		.doc	= "Monitor events from the kernel (^C to stop).",
		.type	= IZ_C_COMMON,
		.parse		= event_parse,
		.request	= NULL,
		.response	= event_response,
		.help		= NULL,
	},
	/*{
		.name	= "add",
		.usage	= "<master> <iface> <hwaddr>",
		.doc	= "Add interface.",
		.type	= IZ_C_COMMON,
	},
	{
		.name	= "del",
		.usage	= "<iface>",
		.doc	= "Delete interface.",
		.type	= IZ_C_COMMON,
	},*/
	{
		.name 	= "scan",
		.usage 	= "<iface> <ed|active|passive|orphan> <channels> <duration>",
		.doc 	= "Perform network scanning on specified channels.",
		.type	= IZ_C_MAC,
		.nl_cmd		= IEEE802154_SCAN_REQ,
		.nl_resp 	= IEEE802154_SCAN_CONF,
		.parse 		= scan_parse,
		.request 	= scan_request,
		.response	= scan_response,
		.help		= NULL,
	},
	{
		.name 	= "assoc",
		.usage 	= "<iface> <pan> <coord> <chan> ['short']",
		.doc 	= "Associate with a given network via coordinator.",
		.type	= IZ_C_MAC,
		.nl_cmd		= IEEE802154_ASSOCIATE_REQ,
		.nl_resp 	= IEEE802154_ASSOCIATE_CONF,
		.parse 		= assoc_parse,
		.request 	= assoc_request,
		.response	= assoc_response,
		.help		= NULL,
	},
	{
		.name 	= "disassoc",
		.usage 	= "<iface> <addr> <reason>",
		.doc 	= "Disassociate from a network.",
		.type	= IZ_C_MAC,
		.nl_cmd		= IEEE802154_DISASSOCIATE_REQ,
		.nl_resp 	= IEEE802154_DISASSOCIATE_CONF,
		.parse 		= disassoc_parse,
		.request 	= disassoc_request,
		.response	= disassoc_response,
		.help		= NULL,
	},
};
#define IZ_COMMANDS_NUM (sizeof(iz_commands) / sizeof(iz_commands[0]))

static struct iz_cmd_type iz_types[] = {
	{
		.type 	= IZ_C_COMMON,
		.doc	= "Common commands",
	},
	/*{
		.type	= IZ_C_PHY,
		.doc	= "IEEE 802.15.4 PHY specific commands",
	},*/
	{
		.type	= IZ_C_MAC,
		.doc	= "IEEE 802.15.4 MAC specific commands",
	},
	/*{
		.type	= IZ_C_ZB,
		.doc	= "ZigBee specific commands",
	}*/
};
#define IZ_TYPES_NUM (sizeof(iz_types) / sizeof(iz_types[0]))

static char *iz_cmd_names[__IEEE802154_CMD_MAX + 1] = {
	[__IEEE802154_COMMAND_INVALID] = "IEEE802154_COMMAND_INVALID",

	[IEEE802154_ASSOCIATE_REQ] = "IEEE802154_ASSOCIATE_REQ",
	[IEEE802154_ASSOCIATE_CONF] = "IEEE802154_ASSOCIATE_CONF",
	[IEEE802154_DISASSOCIATE_REQ] = "IEEE802154_DISASSOCIATE_REQ",
	[IEEE802154_DISASSOCIATE_CONF] = "IEEE802154_DISASSOCIATE_CONF",
	[IEEE802154_GET_REQ] = "IEEE802154_GET_REQ",
	[IEEE802154_GET_CONF] = "IEEE802154_GET_CONF",
	[IEEE802154_RESET_REQ] = "IEEE802154_RESET_REQ",
	[IEEE802154_RESET_CONF] = "IEEE802154_RESET_CONF",
	[IEEE802154_SCAN_REQ] = "IEEE802154_SCAN_REQ",
	[IEEE802154_SCAN_CONF] = "IEEE802154_SCAN_CONF",
	[IEEE802154_SET_REQ] = "IEEE802154_SET_REQ",
	[IEEE802154_SET_CONF] = "IEEE802154_SET_CONF",
	[IEEE802154_START_REQ] = "IEEE802154_START_REQ",
	[IEEE802154_START_CONF] = "IEEE802154_START_CONF",
	[IEEE802154_SYNC_REQ] = "IEEE802154_SYNC_REQ",
	[IEEE802154_POLL_REQ] = "IEEE802154_POLL_REQ",
	[IEEE802154_POLL_CONF] = "IEEE802154_POLL_CONF",

	[IEEE802154_ASSOCIATE_INDIC] = "IEEE802154_ASSOCIATE_INDIC",
	[IEEE802154_ASSOCIATE_RESP] = "IEEE802154_ASSOCIATE_RESP",
	[IEEE802154_DISASSOCIATE_INDIC] = "IEEE802154_DISASSOCIATE_INDIC",
	[IEEE802154_BEACON_NOTIFY_INDIC] = "IEEE802154_BEACON_NOTIFY_INDIC",
	[IEEE802154_ORPHAN_INDIC] = "IEEE802154_ORPHAN_INDIC",
	[IEEE802154_ORPHAN_RESP] = "IEEE802154_ORPHAN_RESP",
	[IEEE802154_COMM_STATUS_INDIC] = "IEEE802154_COMM_STATUS_INDIC",
	[IEEE802154_SYNC_LOSS_INDIC] = "IEEE802154_SYNC_LOSS_INDIC",

	[IEEE802154_GTS_REQ] = "IEEE802154_GTS_REQ",
	[IEEE802154_GTS_INDIC] = "IEEE802154_GTS_INDIC",
	[IEEE802154_GTS_CONF] = "IEEE802154_GTS_CONF",
	[IEEE802154_RX_ENABLE_REQ] = "IEEE802154_RX_ENABLE_REQ",
	[IEEE802154_RX_ENABLE_CONF] = "IEEE802154_RX_ENABLE_CONF",

	[IEEE802154_LIST_IFACE] = "IEEE802154_LIST_IFACE",
	[IEEE802154_ADD_IFACE] = "IEEE802154_ADD_IFACE",
	[IEEE802154_DEL_IFACE] = "IEEE802154_DEL_IFACE",
};


/* Exit from receive loop (set from receive callback) */
static int iz_exit = 0;

/* Currently processed command info */
static struct iz_cmd cmd;


int main(int argc, char **argv)
{
	int opt_idx;
	int cmd_idx;
	int c;
	int i;
	int family;
	struct nl_handle *nl;
	struct nl_msg *msg;
	char *dummy = NULL;

	/* Parse options */
	opt_idx = 1;
	c = -1;
	while(1) {
		c = getopt_long_only(argc, argv, "", iz_long_opts, &opt_idx);
		if (c == -1) break;
		switch(c) {
		case 0:
			switch(opt_idx) {
			case DEBUG_NUM:
				iz_debug = nl_debug = 1;
				if (optarg) {
					i = strtol(optarg, &dummy, 10);
					if (*dummy == '\0')
						iz_debug = nl_debug = i;
				}
				break;
			case VERSION_NUM:
				printf("%s\n", iz_version);
				return 0;
			case HELP_NUM:
				iz_help();
				return 0;
			}
		}
	}
	if (opt_idx >= argc) {
		printf("%s\n", iz_version);
		printf("Usage: iz [options] [command]\n");
		return 1;
	}
	cmd.argn = optind;

	/* Parse command */
	cmd_idx = -1;
	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (!strcmp(argv[cmd.argn], iz_commands[i].name)) {
			cmd_idx = i;
			break;
		}
	}
	if (cmd_idx == -1) {
		printf("Unknown command %s!\n", argv[cmd.argn]);
		return 1;
	}
	if (iz_commands[cmd_idx].parse) {
		i = iz_commands[cmd_idx].parse(&cmd, argc, argv);
		if (i == IZ_STOP_OK) {
			return 0;
		} else if (i == IZ_STOP_ERR) {
			printf("Command line parsing error!\n");
			return 1;
		}
	}

	/* Prepare NL command */
	nl = nl_handle_alloc();
	if (!nl) {
		nl_perror("Could not allocate NL handle");
		return 1;
	}
	genl_connect(nl);
	family = genl_ctrl_resolve(nl, IEEE802154_NL_NAME);
	nl_socket_add_membership(nl,
		nl_get_multicast_id(nl,
			IEEE802154_NL_NAME, IEEE802154_MCAST_COORD_NAME));
	iz_seq = nl_socket_use_seq(nl) + 1;
	nl_socket_modify_cb(nl, NL_CB_VALID, NL_CB_CUSTOM,
		iz_cb_valid, (void*)&cmd);
	nl_socket_modify_cb(nl, NL_CB_SEQ_CHECK, NL_CB_CUSTOM,
		iz_cb_seq_check, (void*)&cmd);

	/* Send request, if necessary */
	if (iz_commands[cmd_idx].request) {
		msg = nlmsg_alloc();
		if (!msg) {
			nl_perror("Could not allocate NL message!\n");
			return 1;
		}
		genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0,
			cmd.flags, iz_commands[cmd_idx].nl_cmd, 1);

		if (iz_commands[cmd_idx].request(msg,
				&cmd, argc, argv) != IZ_CONT_OK) {
			printf("Request processing error!\n");
			return 1;
		}

		if (iz_debug) printf("nl_send_auto_complete\n");
		nl_send_auto_complete(nl, msg);

		if (iz_debug) printf("nlmsg_free\n");
		nlmsg_free(msg);
	}

	/* Received message handling loop */
	while (!iz_exit) {
		if(nl_recvmsgs_default(nl)) {
			nl_perror("Receive failed");
			return 1;
		}
	}
	nl_close(nl);

	if (iz_exit == IZ_STOP_ERR)
		return 1;

	return 0;
}

static void iz_help(void)
{
	int i, j;

	printf("%s\n\n", iz_version);
	printf("Usage:\n");
	printf("  iz [options] [command]\n");

	printf("\nOptions:\n");
	printf("  --debug[=N] - print netlink messages and other debug information\n");
	printf("  --version - print version\n");
	printf("  --help - print help\n");

	/* Print short help for available commands */
	for (i = 0; i < IZ_TYPES_NUM; i++) {
		printf("\n%s:\n", iz_types[i].doc);
		for (j = 0; j < IZ_COMMANDS_NUM; j++) {
			if (iz_types[i].type == iz_commands[j].type) {
				printf("  %s %s\n\t%s\n", iz_commands[j].name,
					iz_commands[j].usage,
					iz_commands[j].doc);
			}
		}
	}
}

/* Callback for sequence number check */
static int iz_cb_seq_check(struct nl_msg *msg, void *arg)
{
	uint32_t seq;

	if(nlmsg_get_src(msg)->nl_groups)
		return NL_OK;

	seq = nlmsg_hdr(msg)->nlmsg_seq;
	if (seq == iz_seq) {
		if (!(nlmsg_hdr(msg)->nlmsg_flags & NLM_F_MULTI))
			iz_seq ++;
		return NL_OK;
	}
	printf("Sequence number mismatch (%i, %i)!", seq, iz_seq);
	return NL_OK;
}

/* Callback for received valid messages */
static int iz_cb_valid(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[IEEE802154_ATTR_MAX+1];
        struct genlmsghdr *ghdr;
	int i, cmd_idx;

	/* Validate message and parse attributes */
	genlmsg_parse(nlh, 0, attrs, IEEE802154_ATTR_MAX, ieee802154_policy);

        ghdr = nlmsg_data(nlh);

	if (iz_debug)
		printf("Received command %d (%d) for interface\n",
			ghdr->cmd, ghdr->version);

	/* Process response if we don't know nl_resp */
	if (arg && ((struct iz_cmd*)arg)->listen_idx) {
		cmd_idx = ((struct iz_cmd*)arg)->listen_idx;
		i = iz_commands[cmd_idx].response(ghdr, attrs);
		if (i != 0)
			iz_exit = i;
		return 0;
	}

	/* Handle known nl_resp'onces */
	cmd_idx = -1;
	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (ghdr->cmd == iz_commands[i].nl_resp) {
			cmd_idx = i;
			break;
		}
	}
	if (cmd_idx == -1)
		i = 0;
	else
		if (iz_commands[cmd_idx].response)
			i = iz_commands[cmd_idx].response(ghdr, attrs);

	if (i != 0)
		iz_exit = i;

	return 0;
}

/******************/
/* HELP handling  */
/******************/

static int help_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	int i;
	int cmd_idx;

	if (argc > cmd->argn + 2) {
		printf("Too many arguments!\n");
		return IZ_STOP_ERR;
	} else if (argc == cmd->argn + 1) {
		iz_help();
		return IZ_STOP_OK;
	}

	/* Search for command help */
	cmd_idx = -1;
	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (!strcmp(argv[cmd->argn + 1], iz_commands[i].name)) {
			cmd_idx = i;
			break;
		}
	}
	if (cmd_idx == -1) {
		printf("Unknown command %s!\n", argv[cmd->argn + 1]);
		return IZ_STOP_ERR;
	} else {
		printf("%s %s\n\t%s\n\n", iz_commands[cmd_idx].name,
			iz_commands[cmd_idx].usage,
			iz_commands[cmd_idx].doc);
		if (iz_commands[cmd_idx].help)
			iz_commands[cmd_idx].help();
		else
			printf("Detailed help is not available.\n");
	}
	return IZ_STOP_OK;
}

/*****************/
/* SCAN handling */
/*****************/

static int scan_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	cmd->flags = NLM_F_REQUEST;
	return IZ_CONT_OK;
}

static int scan_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv) {
	int type;
	int duration;
	char *dummy;
	int channels;

	if (argc != cmd->argn + 5) {
		printf("Incorrect number of arguments!\n");
		return IZ_STOP_ERR;
	}

	if (!argv[cmd->argn + 1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, argv[cmd->argn + 1]);

	if (!argv[cmd->argn + 2])
		return IZ_STOP_ERR;
	if (!strcmp(argv[cmd->argn + 2], "ed")) {
		type = IEEE802154_MAC_SCAN_ED;
	} else if (!strcmp(argv[cmd->argn + 2], "active")) {
		type = IEEE802154_MAC_SCAN_ACTIVE;
	} else if (!strcmp(argv[cmd->argn + 2], "passive")) {
		type = IEEE802154_MAC_SCAN_PASSIVE;
	} else if (!strcmp(argv[cmd->argn + 2], "orphan")) {
		type = IEEE802154_MAC_SCAN_ORPHAN;
	} else {
		printf("Unknown scan type %s!\n", argv[cmd->argn + 2]);
		return IZ_STOP_ERR;
	}

	if (!argv[cmd->argn + 3])
		return IZ_STOP_ERR;
	channels = strtol(argv[cmd->argn + 3], &dummy, 16);
	if (*dummy) {
		printf("Bad channels %s!\n", argv[cmd->argn + 3]);
		return IZ_STOP_ERR;
	}

	if (!argv[cmd->argn + 4])
		return IZ_STOP_ERR;
	duration = strtol(argv[cmd->argn + 4], &dummy, 10);
	if (*dummy) {
		printf("Bad duration %s!\n", argv[cmd->argn + 4]);
		return IZ_STOP_ERR;
	}

	NLA_PUT_U8(msg, IEEE802154_ATTR_SCAN_TYPE, type);
	NLA_PUT_U32(msg, IEEE802154_ATTR_CHANNELS, channels);
	NLA_PUT_U8(msg, IEEE802154_ATTR_DURATION, duration);

	return IZ_CONT_OK;

nla_put_failure:
	return IZ_STOP_ERR;
}

static int scan_response(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	uint8_t status, type;
	int i;
	uint8_t edl[27];

	if (!attrs[IEEE802154_ATTR_DEV_INDEX] ||
	    !attrs[IEEE802154_ATTR_STATUS] ||
	    !attrs[IEEE802154_ATTR_SCAN_TYPE])
		return IZ_STOP_ERR;

	status = nla_get_u8(attrs[IEEE802154_ATTR_STATUS]);
	if (status != 0)
		printf("Scan failed: %02x\n", status);

	type = nla_get_u8(attrs[IEEE802154_ATTR_SCAN_TYPE]);
	switch (type) {
		case IEEE802154_MAC_SCAN_ED:
			if (!attrs[IEEE802154_ATTR_ED_LIST])
				return IZ_STOP_ERR;

			nla_memcpy(edl, attrs[IEEE802154_ATTR_ED_LIST], 27);
			printf("ED Scan results:\n");
			for (i = 0; i < 27; i++)
				printf("  Ch%2d --- ED = %02x\n", i, edl[i]);
			return IZ_STOP_OK;

		case IEEE802154_MAC_SCAN_ACTIVE:
			printf("Started active (beacons) scan...\n");
			return IZ_CONT_OK;
		default:
			printf("Unsupported scan type: %d\n", type);
			break;
	}

	return IZ_STOP_OK;
}

/******************/
/* LIST handling  */
/******************/

static int list_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	if (argc > cmd->argn + 2) {
		printf("Incorrect number of arguments!\n");
		return IZ_STOP_ERR;
	}

	/* iz list wpan0 */
	if (argc == cmd->argn + 2) {
		cmd->iface = argv[cmd->argn + 1];
		cmd->flags = NLM_F_REQUEST;
	} else {
		/* iz list */
		cmd->iface = NULL;
		cmd->flags = NLM_F_REQUEST | NLM_F_DUMP;
	}

	return IZ_CONT_OK;
}

static int list_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv)
{
	/* List single interface */
	if (cmd->iface)
		nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->iface);

	return IZ_CONT_OK;
}

static int list_response(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	char * dev_name;
	uint32_t dev_index;
	uint16_t dev_type;
	unsigned char hw_addr[IEEE802154_ADDR_LEN];
	uint16_t short_addr;
	uint16_t pan_id;
	char * dev_type_str;

	/* Check for mandatory attributes */
	if (!attrs[IEEE802154_ATTR_DEV_NAME]     ||
	    !attrs[IEEE802154_ATTR_DEV_INDEX]    ||
	    !attrs[IEEE802154_ATTR_DEV_TYPE]     ||
	    !attrs[IEEE802154_ATTR_HW_ADDR]      )
		return IZ_STOP_ERR;

	/* Get attribute values from the message */
	dev_name = nla_get_string(attrs[IEEE802154_ATTR_DEV_NAME]);
	dev_index = nla_get_u32(attrs[IEEE802154_ATTR_DEV_INDEX]);
	dev_type = nla_get_u16(attrs[IEEE802154_ATTR_DEV_TYPE]);
	nla_memcpy(hw_addr, attrs[IEEE802154_ATTR_HW_ADDR],
		IEEE802154_ADDR_LEN);
	if (dev_type == ARPHRD_IEEE802154) {
		short_addr = nla_get_u16(attrs[IEEE802154_ATTR_SHORT_ADDR]);
		pan_id = nla_get_u16(attrs[IEEE802154_ATTR_PAN_ID]);
	}

	/* Display information about interface */
	printf("%i: %s\n", dev_index, dev_name);
	if (dev_type == ARPHRD_IEEE802154_PHY)
		dev_type_str = "IEEE 802.15.4 master (PHY) interface";
	else if (dev_type == ARPHRD_IEEE802154)
		dev_type_str = "IEEE 802.15.4 MAC interface";
	else
		dev_type_str = "Not an IEEE 802.15.4 interface";
	printf("    link: %s\n", dev_type_str);

	if (dev_type == ARPHRD_IEEE802154) {
		printf("    hw %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3],
			hw_addr[4], hw_addr[5], hw_addr[6], hw_addr[7]);
		printf(" pan 0x%04hx short 0x%04hx\n", pan_id, short_addr);
	} else
		printf("    hw: N/A pan N/A short N/A\n");

	return IZ_STOP_OK;

}

/******************/
/* EVENT handling */
/******************/

static int event_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	int i;

	cmd->flags = 0;

	/* Search for our index in iz_commands and save it for receive */
	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (!strcmp(argv[cmd->argn], iz_commands[i].name)) {
			cmd->listen_idx = i;
			break;
		}
	}
	return IZ_CONT_OK;
}

static int event_response(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	if (ghdr->cmd < __IEEE802154_CMD_MAX)
		printf("%s (%i)\n", iz_cmd_names[ghdr->cmd], ghdr->cmd);
	else
		printf("UNKNOWN (%i)\n", ghdr->cmd);

	return IZ_CONT_OK;
}

/************************/
/* ASSOCIATE handling   */
/************************/

static int assoc_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	cmd->flags = NLM_F_REQUEST;
	cmd->listen_idx = 0;
	return IZ_CONT_OK;
}

static int assoc_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv)
{
	char *dummy;
	uint16_t pan_id, coord_short_addr;
	uint8_t chan;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	int ret;
	uint8_t cap =  0
			| (1 << 1) /* FFD */
			| (1 << 3) /* Receiver ON */
			/*| (1 << 7) *//* allocate short */
			;

	if (!argv[cmd->argn + 1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, argv[cmd->argn + 1]);

	if (!argv[cmd->argn + 2])
		return IZ_STOP_ERR;
	pan_id = strtol(argv[cmd->argn + 2], &dummy, 16);
	if (*dummy) {
		printf("Bad PAN ID!\n");
		return IZ_STOP_ERR;
	}

	if (!argv[cmd->argn + 3])
		return IZ_STOP_ERR;
	if (argv[cmd->argn + 3][0] == 'H' || argv[cmd->argn + 3][0] == 'h') {
		ret = parse_hw_addr(argv[cmd->argn + 3]+1, hwa);
		if (ret) {
			printf("Bad coordinator address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT(msg, IEEE802154_ATTR_COORD_HW_ADDR,
			IEEE802154_ADDR_LEN, hwa);
	} else {
		coord_short_addr = strtol(argv[cmd->argn + 3], &dummy, 16);
		if (*dummy) {
			printf("Bad coordinator address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_COORD_SHORT_ADDR,
			coord_short_addr);
	}


	if (!argv[cmd->argn + 4])
		return IZ_STOP_ERR;
	chan = strtol(argv[cmd->argn + 4], &dummy, 16);
	if (*dummy) {
		printf("Bad channel number!\n");
		return IZ_STOP_ERR;
	}

	if (argv[cmd->argn + 5]) {
		if (strcmp(argv[cmd->argn + 5], "short") ||
			argv[cmd->argn + 6])
			return IZ_STOP_ERR;
		else
			cap |= 1 << 7; /* Request short addr */
	}

	NLA_PUT_U8(msg, IEEE802154_ATTR_CHANNEL, chan);
	NLA_PUT_U16(msg, IEEE802154_ATTR_COORD_PAN_ID, pan_id);
	NLA_PUT_U8(msg, IEEE802154_ATTR_CAPABILITY, cap);

	return IZ_CONT_OK;

nla_put_failure:
	return IZ_STOP_ERR;
}

static int assoc_response(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	if (!attrs[IEEE802154_ATTR_SHORT_ADDR] ||
		!attrs[IEEE802154_ATTR_STATUS] )
		return IZ_STOP_ERR;

	printf("Received short address %04hx, status %02hhx\n",
		nla_get_u16(attrs[IEEE802154_ATTR_SHORT_ADDR]),
		nla_get_u8(attrs[IEEE802154_ATTR_STATUS]));

	return IZ_STOP_OK;
}

/*************************/
/* DISASSOCIATE handling */
/*************************/

static int disassoc_parse(struct iz_cmd *cmd, int argc, char **argv)
{
	cmd->flags = NLM_F_REQUEST;
	cmd->listen_idx = 0;
	return IZ_CONT_OK;
}

static int disassoc_request(struct nl_msg *msg, struct iz_cmd *cmd,
			int argc, char **argv)
{
	char *dummy;
	uint8_t reason;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	uint16_t  short_addr;
	int ret;

	if (!argv[cmd->argn + 1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, argv[cmd->argn + 1]);

	if (!argv[cmd->argn + 2])
		return IZ_STOP_ERR;
	if (argv[cmd->argn + 2][0] == 'H' || argv[cmd->argn + 2][0] == 'h') {
		ret = parse_hw_addr(argv[cmd->argn + 2]+1, hwa);
		if (ret) {
			printf("Bad destination address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT(msg, IEEE802154_ATTR_DEST_HW_ADDR,
			IEEE802154_ADDR_LEN, hwa);
	} else {
		short_addr = strtol(argv[cmd->argn + 2], &dummy, 16);
		if (*dummy) {
			printf("Bad destination address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_DEST_SHORT_ADDR, short_addr);
	}

	if (!argv[cmd->argn + 3])
		return IZ_STOP_ERR;
	reason = strtol(argv[cmd->argn + 3], &dummy, 16);
	if (*dummy) {
		printf("Bad disassociation reason!\n");
		return IZ_STOP_ERR;
	}

	if (argv[cmd->argn + 4])
		return IZ_STOP_ERR;

	NLA_PUT_U8(msg, IEEE802154_ATTR_REASON, reason);

	return IZ_CONT_OK;

nla_put_failure:
	return IZ_STOP_ERR;

}

static int disassoc_response(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	/* Hmm... TODO? */
	printf("Done.\n");

	return IZ_STOP_OK;
}

