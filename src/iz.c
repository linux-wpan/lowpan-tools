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
static void iz_help(const char *pname);

/* Command specific declarations */

/* HELP */
static int help_parse(struct iz_cmd *cmd);

/* LIST */
static int list_parse(struct iz_cmd *cmd);
static int list_request(struct iz_cmd *cmd, struct nl_msg *msg);
static int list_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

/* EVENT */
static int event_parse(struct iz_cmd *cmd);
static int event_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

/* SCAN */
static int scan_parse(struct iz_cmd *cmd);
static int scan_request(struct iz_cmd *cmd, struct nl_msg *msg);
static int scan_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

/* ASSOCIATE */
static int assoc_parse(struct iz_cmd *cmd);
static int assoc_request(struct iz_cmd *cmd, struct nl_msg *msg);
static int assoc_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

/* DISASSOCIATE */
static int disassoc_parse(struct iz_cmd *cmd);
static int disassoc_request(struct iz_cmd *cmd, struct nl_msg *msg);
static int disassoc_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);


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

/* iz command descriptor */
struct iz_cmd_desc {
	const char *name;	/* Name (as in command line) */
	const char *usage;	/* Arguments list */
	const char *doc;	/* One line command description */
	unsigned char nl_cmd;	/* NL command ID */
	unsigned char nl_resp;	/* NL command response ID (optional) */

	/* Parse command line, fill in iz_cmd struct. */
	/* You must set cmd->flags here! */
	/* Returns 1 to stop with error, -1 to stop with Ok; 0 to continue */
	int (*parse)(struct iz_cmd *cmd);

	/* Prepare an outgoing netlink message */
	/* Returns 0 in case of success, in other case error */
	/* If request is not defined, we go to receive wo sending message */
	int (*request)(struct iz_cmd *cmd, struct nl_msg *msg);

	/* Handle an incoming netlink message */
	/* Returns 1 to stop with error, -1 to stop with Ok; 0 to continue */
	int (*response)(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs);

	/* Print detailed help information */
	void (*help)(void);
};

static const struct option iz_long_opts[] = {
	{ "debug", optional_argument, NULL, 'd' },
	{ "version", no_argument, NULL, 'v' },
	{ "help", no_argument, NULL, 'h' },
	{ NULL, 0, NULL, 0 },
};

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
		.name		= "help",
		.usage		= "[command]",
		.doc		= "Print detailed help for a command.",
		.parse		= help_parse,
	},
	{
		.name		= "list",
		.usage		= "[iface]",
		.doc		= "List interface(s).",
		.nl_cmd		= IEEE802154_LIST_IFACE,
		.nl_resp	= IEEE802154_LIST_IFACE,
		.parse		= list_parse,
		.request	= list_request,
		.response	= list_response,
		.help		= NULL,
	},
	{
		.name		= "event",
		.usage		= "",
		.doc		= "Monitor events from the kernel (^C to stop).",
		.parse		= event_parse,
		.request	= NULL,
		.response	= event_response,
		.help		= NULL,
	},
};
#define IZ_COMMANDS_NUM (sizeof(iz_commands) / sizeof(iz_commands[0]))

static struct iz_cmd_desc mac_commands[] = {
	{
		.name		= "scan",
		.usage		= "<iface> <ed|active|passive|orphan> <channels> <duration>",
		.doc		= "Perform network scanning on specified channels.",
		.nl_cmd		= IEEE802154_SCAN_REQ,
		.nl_resp	= IEEE802154_SCAN_CONF,
		.parse		= scan_parse,
		.request	= scan_request,
		.response	= scan_response,
		.help		= NULL,
	},
	{
		.name		= "assoc",
		.usage		= "<iface> <pan> <coord> <chan> ['short']",
		.doc		= "Associate with a given network via coordinator.",
		.nl_cmd		= IEEE802154_ASSOCIATE_REQ,
		.nl_resp	= IEEE802154_ASSOCIATE_CONF,
		.parse		= assoc_parse,
		.request	= assoc_request,
		.response	= assoc_response,
		.help		= NULL,
	},
	{
		.name		= "disassoc",
		.usage		= "<iface> <addr> <reason>",
		.doc		= "Disassociate from a network.",
		.nl_cmd		= IEEE802154_DISASSOCIATE_REQ,
		.nl_resp	= IEEE802154_DISASSOCIATE_CONF,
		.parse		= disassoc_parse,
		.request	= disassoc_request,
		.response	= disassoc_response,
		.help		= NULL,
	},
};
#define MAC_COMMANDS_NUM (sizeof(mac_commands) / sizeof(mac_commands[0]))

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
};


/* Exit from receive loop (set from receive callback) */
static int iz_exit = 0;

struct iz_cmd_desc *get_cmd(const char *name)
{
	int i;

	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (!strcmp(name, iz_commands[i].name)) {
			return &iz_commands[i];
		}
	}

	for (i = 0; i < MAC_COMMANDS_NUM; i++) {
		if (!strcmp(name, mac_commands[i].name)) {
			return &mac_commands[i];
		}
	}

	return NULL;
}

struct iz_cmd_desc *get_resp(unsigned char nl_resp)
{
	int i;

	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		if (iz_commands[i].nl_resp == nl_resp) {
			return &iz_commands[i];
		}
	}

	for (i = 0; i < MAC_COMMANDS_NUM; i++) {
		if (mac_commands[i].nl_resp == nl_resp) {
			return &mac_commands[i];
		}
	}

	return NULL;
}


int main(int argc, char **argv)
{
	int opt_idx;
	int c;
	int i;
	int family;
	struct nl_handle *nl;
	struct nl_msg *msg;
	char *dummy = NULL;

	/* Currently processed command info */
	struct iz_cmd cmd;


	/* Parse options */
	opt_idx = -1;
	while (1) {
		c = getopt_long(argc, argv, "d::vh", iz_long_opts, &opt_idx);
		if (c == -1)
			break;

		switch(c) {
		case 'd':
			if (optarg) {
				i = strtol(optarg, &dummy, 10);
				if (*dummy == '\0')
					iz_debug = nl_debug = i;
				else {
					fprintf(stderr, "Error: incorrect debug level: '%s'\n", optarg);
					exit(1);
				}
			} else
				iz_debug = nl_debug = 1;
			break;
		case 'v':
			printf("iz %s\nCopyright (C) 2008, 2009 by authors team\n", VERSION);
			return 0;
		case 'h':
			iz_help(argv[0]);
			return 0;
		default:
			iz_help(argv[0]);
			return 1;
		}
	}
	if (optind >= argc) {
		printf("iz %s\nCopyright (C) 2008, 2009 by authors team\n", VERSION);
		printf("Usage: iz [options] [command]\n");
		return 1;
	}

	memset(&cmd, 0, sizeof(cmd));

	cmd.argc = argc - optind;
	cmd.argv = argv + optind;

	/* Parse command */
	cmd.desc = get_cmd(argv[optind]);
	if (!cmd.desc) {
		printf("Unknown command %s!\n", argv[optind]);
		return 1;
	}
	if (cmd.desc->parse) {
		i = cmd.desc->parse(&cmd);
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
	if (cmd.desc->request) {
		msg = nlmsg_alloc();
		if (!msg) {
			nl_perror("Could not allocate NL message!\n");
			return 1;
		}
		genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0,
			cmd.flags, cmd.desc->nl_cmd, 1);

		if (cmd.desc->request(&cmd, msg) != IZ_CONT_OK) {
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

static void iz_help(const char *pname)
{
	int i;

	printf("Usage: %s [options] [command]\n", pname);
	printf("Manage IEEE 802.15.4 network interfaces\n\n");

	printf("Options:\n");
	printf("  -d, --debug[=N]                print netlink messages and other debug information\n");
	printf("  -v, --version                  print version\n");
	printf("  -h, --help                      print help\n");

	/* Print short help for available commands */
	printf("\nCommon commands:\n");
	for (i = 0; i < IZ_COMMANDS_NUM; i++) {
		printf("  %s %s\n\t%s\n", iz_commands[i].name,
			iz_commands[i].usage,
			iz_commands[i].doc);
	}

	printf("\nMAC 802.15.4 commands:\n");
	for (i = 0; i < MAC_COMMANDS_NUM; i++) {
		printf("  %s %s\n\t%s\n", mac_commands[i].name,
			mac_commands[i].usage,
			mac_commands[i].doc);
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
	struct iz_cmd_desc *desc;
	struct iz_cmd *cmd = arg;

	/* Validate message and parse attributes */
	genlmsg_parse(nlh, 0, attrs, IEEE802154_ATTR_MAX, ieee802154_policy);

        ghdr = nlmsg_data(nlh);

	if (iz_debug)
		printf("Received command %d (%d) for interface\n",
			ghdr->cmd, ghdr->version);

	if (cmd->listener) {
		/* Process response if we don't know nl_resp */
		desc = cmd->listener;
	} else {
		/* Handle known nl_resp'onces */
		desc = get_resp(ghdr->cmd);
	}
	if (desc && desc->response)
		iz_exit = desc->response(cmd, ghdr, attrs);

	return 0;
}

/******************/
/* HELP handling  */
/******************/

static int help_parse(struct iz_cmd *cmd)
{
	struct iz_cmd_desc *desc;

	if (cmd->argc > 2) {
		printf("Too many arguments!\n");
		return IZ_STOP_ERR;
	} else if (cmd->argc == 1) {
		iz_help("iz");
		return IZ_STOP_OK;
	}

	/* Search for command help */
	desc = get_cmd(cmd->argv[1]);
	if (!desc) {
		printf("Unknown command %s!\n", cmd->argv[1]);
		return IZ_STOP_ERR;
	} else {
		printf("%s %s\n\t%s\n\n", desc->name,
			desc->usage,
			desc->doc);
		if (desc->help)
			desc->help();
		else
			printf("Detailed help is not available.\n");
	}
	return IZ_STOP_OK;
}

/*****************/
/* SCAN handling */
/*****************/

static int scan_parse(struct iz_cmd *cmd)
{
	cmd->flags = NLM_F_REQUEST;
	return IZ_CONT_OK;
}

static int scan_request(struct iz_cmd *cmd, struct nl_msg *msg) {
	int type;
	int duration;
	char *dummy;
	int channels;

	if (cmd->argc != 5) {
		printf("Incorrect number of arguments!\n");
		return IZ_STOP_ERR;
	}

	if (!cmd->argv[1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->argv[1]);

	if (!cmd->argv[2])
		return IZ_STOP_ERR;
	if (!strcmp(cmd->argv[2], "ed")) {
		type = IEEE802154_MAC_SCAN_ED;
	} else if (!strcmp(cmd->argv[2], "active")) {
		type = IEEE802154_MAC_SCAN_ACTIVE;
	} else if (!strcmp(cmd->argv[2], "passive")) {
		type = IEEE802154_MAC_SCAN_PASSIVE;
	} else if (!strcmp(cmd->argv[2], "orphan")) {
		type = IEEE802154_MAC_SCAN_ORPHAN;
	} else {
		printf("Unknown scan type %s!\n", cmd->argv[2]);
		return IZ_STOP_ERR;
	}

	if (!cmd->argv[3])
		return IZ_STOP_ERR;
	channels = strtol(cmd->argv[3], &dummy, 16);
	if (*dummy) {
		printf("Bad channels %s!\n", cmd->argv[3]);
		return IZ_STOP_ERR;
	}

	if (!cmd->argv[4])
		return IZ_STOP_ERR;
	duration = strtol(cmd->argv[4], &dummy, 10);
	if (*dummy) {
		printf("Bad duration %s!\n", cmd->argv[4]);
		return IZ_STOP_ERR;
	}

	NLA_PUT_U8(msg, IEEE802154_ATTR_SCAN_TYPE, type);
	NLA_PUT_U32(msg, IEEE802154_ATTR_CHANNELS, channels);
	NLA_PUT_U8(msg, IEEE802154_ATTR_DURATION, duration);

	return IZ_CONT_OK;

nla_put_failure:
	return IZ_STOP_ERR;
}

static int scan_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
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

static int list_parse(struct iz_cmd *cmd)
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

static int list_request(struct iz_cmd *cmd, struct nl_msg *msg)
{
	/* List single interface */
	if (cmd->iface)
		nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->iface);

	return IZ_CONT_OK;
}

static int list_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	char * dev_name;
	uint32_t dev_index;
	unsigned char hw_addr[IEEE802154_ADDR_LEN];
	uint16_t short_addr;
	uint16_t pan_id;
	char * dev_type_str;

	/* Check for mandatory attributes */
	if (!attrs[IEEE802154_ATTR_DEV_NAME]     ||
	    !attrs[IEEE802154_ATTR_DEV_INDEX]    ||
	    !attrs[IEEE802154_ATTR_HW_ADDR]      )
		return IZ_STOP_ERR;

	/* Get attribute values from the message */
	dev_name = nla_get_string(attrs[IEEE802154_ATTR_DEV_NAME]);
	dev_index = nla_get_u32(attrs[IEEE802154_ATTR_DEV_INDEX]);
	nla_memcpy(hw_addr, attrs[IEEE802154_ATTR_HW_ADDR],
		IEEE802154_ADDR_LEN);
	short_addr = nla_get_u16(attrs[IEEE802154_ATTR_SHORT_ADDR]);
	pan_id = nla_get_u16(attrs[IEEE802154_ATTR_PAN_ID]);

	/* Display information about interface */
	printf("%s\n", dev_name);
	dev_type_str = "IEEE 802.15.4 MAC interface";
	printf("    link: %s\n", dev_type_str);

	printf("    hw %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3],
			hw_addr[4], hw_addr[5], hw_addr[6], hw_addr[7]);
	printf(" pan 0x%04hx short 0x%04hx\n", pan_id, short_addr);

	return IZ_STOP_OK;

}

/******************/
/* EVENT handling */
/******************/

static int event_parse(struct iz_cmd *cmd)
{
	cmd->flags = 0;

	cmd->listener = cmd->desc;

	return IZ_CONT_OK;
}

static int event_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	if (ghdr->cmd < __IEEE802154_CMD_MAX)
		fprintf(stdout, "%s (%i)\n", iz_cmd_names[ghdr->cmd], ghdr->cmd);
	else
		fprintf(stdout, "UNKNOWN (%i)\n", ghdr->cmd);

	fflush(stdout);

	return IZ_CONT_OK;
}

/************************/
/* ASSOCIATE handling   */
/************************/

static int assoc_parse(struct iz_cmd *cmd)
{
	cmd->flags = NLM_F_REQUEST;
	return IZ_CONT_OK;
}

static int assoc_request(struct iz_cmd *cmd, struct nl_msg *msg)
{
	char *dummy;
	uint16_t pan_id, coord_short_addr;
	uint8_t chan;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	int ret;
	uint8_t cap =  0
			| (1 << 1) /* FFD */
			| (1 << 3) /* Receiver ON */
			/*| (1 << 7) */ /* allocate short */
			;

	if (!cmd->argv[1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->argv[1]);

	if (!cmd->argv[2])
		return IZ_STOP_ERR;
	pan_id = strtol(cmd->argv[2], &dummy, 16);
	if (*dummy) {
		printf("Bad PAN ID!\n");
		return IZ_STOP_ERR;
	}

	if (!cmd->argv[3])
		return IZ_STOP_ERR;
	if (cmd->argv[3][0] == 'H' || cmd->argv[3][0] == 'h') {
		ret = parse_hw_addr(cmd->argv[3]+1, hwa);
		if (ret) {
			printf("Bad coordinator address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT(msg, IEEE802154_ATTR_COORD_HW_ADDR,
			IEEE802154_ADDR_LEN, hwa);
	} else {
		coord_short_addr = strtol(cmd->argv[3], &dummy, 16);
		if (*dummy) {
			printf("Bad coordinator address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_COORD_SHORT_ADDR,
			coord_short_addr);
	}


	if (!cmd->argv[4])
		return IZ_STOP_ERR;
	chan = strtol(cmd->argv[4], &dummy, 16);
	if (*dummy) {
		printf("Bad channel number!\n");
		return IZ_STOP_ERR;
	}

	if (cmd->argv[5]) {
		if (strcmp(cmd->argv[5], "short") ||
			cmd->argv[6])
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

static int assoc_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
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

static int disassoc_parse(struct iz_cmd *cmd)
{
	cmd->flags = NLM_F_REQUEST;
	return IZ_CONT_OK;
}

static int disassoc_request(struct iz_cmd *cmd, struct nl_msg *msg)
{
	char *dummy;
	uint8_t reason;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	uint16_t  short_addr;
	int ret;

	if (!cmd->argv[1])
		return IZ_STOP_ERR;
	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, cmd->argv[1]);

	if (!cmd->argv[2])
		return IZ_STOP_ERR;
	if (cmd->argv[2][0] == 'H' || cmd->argv[2][0] == 'h') {
		ret = parse_hw_addr(cmd->argv[2]+1, hwa);
		if (ret) {
			printf("Bad destination address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT(msg, IEEE802154_ATTR_DEST_HW_ADDR,
			IEEE802154_ADDR_LEN, hwa);
	} else {
		short_addr = strtol(cmd->argv[2], &dummy, 16);
		if (*dummy) {
			printf("Bad destination address!\n");
			return IZ_STOP_ERR;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_DEST_SHORT_ADDR, short_addr);
	}

	if (!cmd->argv[3])
		return IZ_STOP_ERR;
	reason = strtol(cmd->argv[3], &dummy, 16);
	if (*dummy) {
		printf("Bad disassociation reason!\n");
		return IZ_STOP_ERR;
	}

	if (cmd->argv[4])
		return IZ_STOP_ERR;

	NLA_PUT_U8(msg, IEEE802154_ATTR_REASON, reason);

	return IZ_CONT_OK;

nla_put_failure:
	return IZ_STOP_ERR;

}

static int disassoc_response(struct iz_cmd *cmd, struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	/* Hmm... TODO? */
	printf("Done.\n");

	return IZ_STOP_OK;
}

