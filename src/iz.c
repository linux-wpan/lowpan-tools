/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008, 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
 * Written-by: Sergey Lapin
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

#include <errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include <ieee802154.h>
#define u64 uint64_t
#include <nl802154.h>
#include <libcommon.h>

static int last_cmd = -1;
static int seq_expected;
static const char *iface;

static int scan_confirmation(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	uint8_t status, type;
	int i;
	uint8_t edl[27];

	if (!attrs[IEEE802154_ATTR_DEV_INDEX] ||
	    !attrs[IEEE802154_ATTR_STATUS] ||
	    !attrs[IEEE802154_ATTR_SCAN_TYPE])
		return -EINVAL;

	status = nla_get_u8(attrs[IEEE802154_ATTR_STATUS]);
	if (status != 0)
		printf("Scan failed: %02x\n", status);

	type = nla_get_u8(attrs[IEEE802154_ATTR_SCAN_TYPE]);

	switch (type) {
		case IEEE802154_MAC_SCAN_ED:
			if (!attrs[IEEE802154_ATTR_ED_LIST])
				return -EINVAL;

			nla_memcpy(edl, attrs[IEEE802154_ATTR_ED_LIST], 27);
			printf("ED Scan results:\n");
			for (i = 0; i < 27; i++)
				printf("  Ch%2d --- ED = %02x\n", i, edl[i]);
			return 0;

		case IEEE802154_MAC_SCAN_ACTIVE:
			printf("Started active scan. Will catch beacons from now\n");
			return 0;
		default:
			printf("Unsupported scan type: %d\n", type);
			break;
	}

	return 0;

}

static int beacon_indication(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	if (!attrs[IEEE802154_ATTR_DEV_INDEX] ||
	    !attrs[IEEE802154_ATTR_STATUS])
	    	return -EINVAL;
	printf("Got a beacon\n");
	return 0;
}

static int parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[IEEE802154_ATTR_MAX+1];
        struct genlmsghdr *ghdr;
	const char *name;

	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, IEEE802154_ATTR_MAX, ieee802154_policy);

        ghdr = nlmsg_data(nlh);

	if (!attrs[IEEE802154_ATTR_DEV_NAME])
		return -EINVAL;

	name = nla_get_string(attrs[IEEE802154_ATTR_DEV_NAME]);
	printf("Received command %d (%d) for interface %s\n", ghdr->cmd, ghdr->version, name);
	if (strcmp(iface, name))
		return 0;

	last_cmd = ghdr->cmd;

	if (ghdr->cmd == IEEE802154_ASSOCIATE_CONF ) {
		printf("Received short address %04hx, status %02hhx\n",
			nla_get_u16(attrs[IEEE802154_ATTR_SHORT_ADDR]),
			nla_get_u8(attrs[IEEE802154_ATTR_STATUS]));
	} else if (ghdr->cmd == IEEE802154_SCAN_CONF) {
		return scan_confirmation(ghdr, attrs);
	} else if (ghdr->cmd == IEEE802154_BEACON_NOTIFY_INDIC) {
		return beacon_indication(ghdr, attrs);
	}

	return 0;
}

static int seq_check(struct nl_msg *msg, void *arg) {
	if (nlmsg_get_src(msg)->nl_groups)
		return NL_OK;

	uint32_t seq = nlmsg_hdr(msg)->nlmsg_seq;

	if (seq == seq_expected) {
		seq_expected ++;
		return NL_OK;
	}

	fprintf(stderr, "Sequence number mismatch: %x != %x\n", seq, seq_expected);

	return NL_SKIP;
}

static int associate(struct nl_msg *msg, char **args) {
	char *dummy;
	uint16_t pan_id, coord_short_addr;
	uint8_t chan;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	int ret;
	uint8_t cap =  0
			| (1 << 1) /* FFD */
			| (1 << 3) /* Receiver ON */
			;
//			| (1 << 7) /* allocate short */

	if (!args[1])
		return -EINVAL;
	pan_id = strtol(args[1], &dummy, 16);
	if (*dummy) {
		printf("Bad PAN id\n");
		return -EINVAL;
	}

	if (!args[2])
		return -EINVAL;
	if (args[2][0] == 'H' || args[2][0] == 'h') {
		ret = parse_hw_addr(args[2]+1, hwa);

		if (ret) {
			printf("Bad CoordAddr\n");
			return ret;
		}

		NLA_PUT(msg, IEEE802154_ATTR_COORD_HW_ADDR, IEEE802154_ADDR_LEN, hwa);
	} else {
		coord_short_addr = strtol(args[2], &dummy, 16);
		if (*dummy) {
			printf("Bad CoordAddr\n");
			return -EINVAL;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_COORD_SHORT_ADDR, coord_short_addr);
	}


	if (!args[3])
		return -EINVAL;
	chan = strtol(args[3], &dummy, 16);
	if (*dummy) {
		printf("Bad chan#\n");
		return -EINVAL;
	}


	if (args[4]) {
		if (strcmp(args[4], "short") || args[5])
			return -EINVAL;
		else
			cap |= 1 << 7; /* Request short addr */
	}

	NLA_PUT_U8(msg, IEEE802154_ATTR_CHANNEL, chan);
	NLA_PUT_U16(msg, IEEE802154_ATTR_COORD_PAN_ID, pan_id);
	NLA_PUT_U8(msg, IEEE802154_ATTR_CAPABILITY, cap);

	return 0;

nla_put_failure:
	return -1;
}

static int disassociate(struct nl_msg *msg, char **args) {
	char *dummy;
	uint8_t reason;
	unsigned char hwa[IEEE802154_ADDR_LEN];
	uint16_t  short_addr;
	int ret;

	if (!args[1])
		return -EINVAL;
	if (args[1][0] == 'H' || args[1][0] == 'h') {
		ret = parse_hw_addr(args[1]+1, hwa);

		if (ret) {
			printf("Bad DestAddr\n");
			return ret;
		}

		NLA_PUT(msg, IEEE802154_ATTR_DEST_HW_ADDR, IEEE802154_ADDR_LEN, hwa);
	} else {
		short_addr = strtol(args[1], &dummy, 16);
		if (*dummy) {
			printf("Bad DestAddr\n");
			return -EINVAL;
		}
		NLA_PUT_U16(msg, IEEE802154_ATTR_DEST_SHORT_ADDR, short_addr);
	}

	if (!args[2])
		return -EINVAL;
	reason = strtol(args[2], &dummy, 16);
	if (*dummy) {
		printf("Bad reason\n");
		return -EINVAL;
	}

	if (args[3])
		return -EINVAL;

	NLA_PUT_U8(msg, IEEE802154_ATTR_REASON, reason);

	return 0;

nla_put_failure:
	return -1;
}

static int scan(struct nl_msg *msg, char **args) {
	int type;
	int duration;
	char *dummy;
	int channels;

	if (!args[1])
		return -EINVAL;

	switch (*args[1]) {
		case 'e':
			type = IEEE802154_MAC_SCAN_ED;
			break;
		case 'a':
			type = IEEE802154_MAC_SCAN_ACTIVE;
			break;
		case 'p':
			type = IEEE802154_MAC_SCAN_PASSIVE;
			break;
		case 'o':
			type = IEEE802154_MAC_SCAN_ORPHAN;
			break;
		default:
			printf("Invalid type\n");
			return -EINVAL;
	}

	if (!args[2])
		return -EINVAL;
	channels = strtol(args[2], &dummy, 16);
	if (*dummy) {
		printf("Bad channels\n");
		return -EINVAL;
	}

	if (!args[3])
		return -EINVAL;
	duration = strtol(args[3], &dummy, 10);
	if (*dummy) {
		printf("Bad duration\n");
		return -EINVAL;
	}

	if (args[4])
		return -EINVAL;

	NLA_PUT_U8(msg, IEEE802154_ATTR_SCAN_TYPE, type);
	NLA_PUT_U32(msg, IEEE802154_ATTR_CHANNELS, channels);
	NLA_PUT_U8(msg, IEEE802154_ATTR_DURATION, duration);

	return 0;

nla_put_failure:
	return -1;
}

static struct {
	const char *name;
	const char *usage;
	const char *usage_exp;
	int nl_cmd;
	int nl_resp;
	int (*fillmsg)(struct nl_msg *msg, char **args);
	int (*response)(void);
} commands[] = {
	{
		.name = "assoc",
		.usage = "PANid CoordAddr chan# [short]",
		.usage_exp =	"  associate with network\n"
				"    PANid - 16-bit hex PAN id\n\n"
				"    CoordAddr - 16-bit hex coordinator address\n\n"
				"    chan# - radio channel no, from 0 to 26 (radio hardware dependant)\n\n"
				"    short - add word 'short' to command line to get real address (not 0xfffe)\n\n",
		.nl_cmd = IEEE802154_ASSOCIATE_REQ,
		.nl_resp = IEEE802154_ASSOCIATE_CONF,
		.fillmsg = associate,
	},
	{
		.name = "disassoc",
		.usage = "DestAddr reason",
		.usage_exp = "  disassociate from the network\n"
				"    DestAddr - destination address\n\n"
				"    reason - disassociation reason\n\n",
		.nl_cmd = IEEE802154_DISASSOCIATE_REQ,
		.nl_resp = IEEE802154_DISASSOCIATE_CONF,
		.fillmsg = disassociate,
	},
	{
		.name = "scan",
		.usage = "[eapo] chans duration",
		.usage_exp = "  scan the network\n"
				"   scan modes:\n\n"
				"      e - ED scan\n\n"
				"      a - active scan\n\n"
				"      p - passive scan\n\n"
				"      o - orphan scan\n\n",
		.nl_cmd = IEEE802154_SCAN_REQ,
		.nl_resp = IEEE802154_SCAN_CONF,
		.fillmsg = scan,
	},
};

static int usage(char **args) {
	int i;
	printf("Usage: %s iface cmd args...\n", args[0]);
	printf("or: %s --help\n", args[0]);
	printf("or: %s --version\n\n", args[0]);
	printf("Command syntax:\n");
	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		printf("  %s%s%s", commands[i].name,
				commands[i].usage ? " " : "",
				commands[i].usage ?: "");
		printf("%s", commands[i].usage_exp);
	}
	printf("\n");
	return 1;
}


int main(int argc, char **argv) {
	int family;
	struct nl_handle *nl;
	int cmd;
	if (argc == 2) {
		if (!strcmp(argv[1], "--help"))
			return usage(argv);
		if (!strcmp(argv[1], "--version")) {
			printf("izconfig %s\nCopyright (C) 2008, 2009 by authors team\n", VERSION);
			return 0;
		}
	}

	if (argc < 3) {
		return usage(argv);
	}

	iface = argv[1];
	for (cmd = sizeof(commands)/sizeof(commands[0])-1; cmd >= 0; cmd--)
		if (!strcmp(commands[cmd].name, argv[2]))
			break;
	if (cmd < 0)
		return usage(argv);

	argv[2] = argv[0];
	argv += 2;
	argc -= 2;

	nl = nl_handle_alloc();

	if (!nl) {
		nl_perror("nl_handle_alloc");
		return 1;
	}

	genl_connect(nl);
	nl_perror("genl_connect");

	family = genl_ctrl_resolve(nl, IEEE802154_NL_NAME);
	nl_perror("genl_ctrl_resolve");

	nl_socket_add_membership(nl, nl_get_multicast_id(nl, IEEE802154_NL_NAME, IEEE802154_MCAST_COORD_NAME));

	seq_expected = nl_socket_use_seq(nl) + 1;

	nl_socket_modify_cb(nl, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
	nl_socket_modify_cb(nl, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, seq_check, NULL);

	struct nl_msg *msg = nlmsg_alloc();
	nl_perror("nlmsg_alloc");
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, commands[cmd].nl_cmd, /* vers */ 1);

	nla_put_string(msg, IEEE802154_ATTR_DEV_NAME, iface);

	int err = commands[cmd].fillmsg(msg, argv);
	if (err)
		return usage(argv);

	nl_send_auto_complete(nl, msg);
	nl_perror("nl_send_auto_complete");

	nlmsg_free(msg);

	while (last_cmd != commands[cmd].nl_resp) {
		nl_recvmsgs_default(nl);
		nl_perror("nl_recvmsgs_default");
	}


	nl_close(nl);

	return 0;
}
