#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <errno.h>

#include <ieee802154.h>
#define IEEE80215_NL_WANT_POLICY
#define u64 uint64_t
#include <ieee80215-nl.h>
#include <libcommon.h>

static int last_cmd = -1;
static int seq_expected;
static const char *iface;

static int scan_confirmation(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	uint8_t status, type;
	int i;
	uint8_t edl[27];

	if (!attrs[IEEE80215_ATTR_DEV_INDEX] ||
	    !attrs[IEEE80215_ATTR_STATUS] ||
	    !attrs[IEEE80215_ATTR_SCAN_TYPE])
		return -EINVAL;

	status = nla_get_u8(attrs[IEEE80215_ATTR_STATUS]);
	if (status != 0)
		printf("Scan failed: %02x\n", status);

	type = nla_get_u8(attrs[IEEE80215_ATTR_SCAN_TYPE]);

	switch (type) {
		case IEEE80215_MAC_SCAN_ED:
			if (!attrs[IEEE80215_ATTR_ED_LIST])
				return -EINVAL;

			nla_memcpy(edl, attrs[IEEE80215_ATTR_ED_LIST], 27);
			printf("ED Scan results:\n");
			for (i = 0; i < 27; i++)
				printf("\tCh%2d --- ED = %02x\n", i, edl[i]);
			return 0;

		default:
			printf("Unsupported scan type: %d\n", type);
			break;
	}

	return 0;

}


static int parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[IEEE80215_ATTR_MAX+1];
        struct genlmsghdr *ghdr;
	const char *name;

	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, IEEE80215_ATTR_MAX, ieee80215_policy);

        ghdr = nlmsg_data(nlh);

	if (!attrs[IEEE80215_ATTR_DEV_NAME])
		return -EINVAL;

	name = nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]);
	printf("Received command %d (%d) for interface %s\n", ghdr->cmd, ghdr->version, name);
	if (strcmp(iface, name))
		return 0;

	last_cmd = ghdr->cmd;

	if (ghdr->cmd == IEEE80215_ASSOCIATE_CONF ) {
		printf("Received short address %04hx, status %02hhx\n",
			nla_get_u16(attrs[IEEE80215_ATTR_SHORT_ADDR]),
			nla_get_u8(attrs[IEEE80215_ATTR_STATUS]));
	} else if (ghdr->cmd == IEEE80215_SCAN_CONF) {
		return scan_confirmation(ghdr, attrs);
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

	if (!args[1])
		return -EINVAL;
	pan_id = strtol(args[1], &dummy, 16);
	if (*dummy) {
		printf("Bad PAN id\n");
		return -EINVAL;
	}

	if (!args[2])
		return -EINVAL;
	coord_short_addr = strtol(args[2], &dummy, 16);
	if (*dummy) {
		printf("Bad CoordAddr\n");
		return -EINVAL;
	}

	if (!args[3])
		return -EINVAL;
	chan = strtol(args[3], &dummy, 16);
	if (*dummy) {
		printf("Bad chan#\n");
		return -EINVAL;
	}


	if (args[4])
		return -EINVAL;

	NLA_PUT_U8(msg, IEEE80215_ATTR_CHANNEL, chan);
	NLA_PUT_U16(msg, IEEE80215_ATTR_COORD_PAN_ID, pan_id);
	NLA_PUT_U16(msg, IEEE80215_ATTR_COORD_SHORT_ADDR, coord_short_addr);
	NLA_PUT_U8(msg, IEEE80215_ATTR_CAPABILITY, 0
						| (1 << 1) /* FFD */
						| (1 << 3) /* Receiver ON */
//						| (1 << 7) /* allocate short */
						);

	return 0;

nla_put_failure:
	return -1;
}

static int disassociate(struct nl_msg *msg, char **args) {
	char *dummy;
	uint8_t reason;
	unsigned char hwa[IEEE80215_ADDR_LEN];
	int ret;

	if (!args[1])
		return -EINVAL;
	ret = parse_hw_addr(args[1], hwa);
	if (ret) {
		printf("Bad DestAddr\n");
		return ret;
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

	NLA_PUT_HW_ADDR(msg, IEEE80215_ATTR_DEST_HW_ADDR, hwa);
	NLA_PUT_U8(msg, IEEE80215_ATTR_REASON, reason);

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
			type = IEEE80215_MAC_SCAN_ED;
			break;
		case 'a':
			type = IEEE80215_MAC_SCAN_ACTIVE;
			break;
		case 'p':
			type = IEEE80215_MAC_SCAN_PASSIVE;
			break;
		case 'o':
			type = IEEE80215_MAC_SCAN_ORPHAN;
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

	NLA_PUT_U8(msg, IEEE80215_ATTR_SCAN_TYPE, type);
	NLA_PUT_U32(msg, IEEE80215_ATTR_CHANNELS, channels);
	NLA_PUT_U8(msg, IEEE80215_ATTR_DURATION, duration);

	return 0;

nla_put_failure:
	return -1;
}

struct {
	const char *name;
	const char *usage;
	int nl_cmd;
	int nl_resp;
	int (*fillmsg)(struct nl_msg *msg, char **args);
	int (*response)(void);
} commands[] = {
	{
		.name = "assoc",
		.usage = "PANid CoordAddr chan#",
		.nl_cmd = IEEE80215_ASSOCIATE_REQ,
		.nl_resp = IEEE80215_ASSOCIATE_CONF,
		.fillmsg = associate,
	},
	{
		.name = "disassoc",
		.usage = "DestAddr reason",
		.nl_cmd = IEEE80215_DISASSOCIATE_REQ,
		.nl_resp = IEEE80215_DISASSOCIATE_CONF,
		.fillmsg = disassociate,
	},
	{
		.name = "scan",
		.usage = "[eapo] chans duration",
		.nl_cmd = IEEE80215_SCAN_REQ,
		.nl_resp = IEEE80215_SCAN_CONF,
		.fillmsg = scan,
	},
};

static int usage(char **args) {
	int i;
	printf("Usage: %s iface cmd args...\n", args[0]);
	printf("\tcmd is one of:\n");
	for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
		printf("\t%s%s%s\n", commands[i].name,
				commands[i].usage ? " " : "",
				commands[i].usage ?: "");
	}
	printf("\n");
	return 1;
}


int main(int argc, char **argv) {
	int family;
	struct nl_handle *nl;
	int cmd;

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

	family = genl_ctrl_resolve(nl, IEEE80215_NL_NAME);
	nl_perror("genl_ctrl_resolve");

	nl_socket_add_membership(nl, nl_get_multicast_id(nl, IEEE80215_NL_NAME, IEEE80215_MCAST_COORD_NAME));

	seq_expected = nl_socket_use_seq(nl) + 1;

	nl_socket_modify_cb(nl, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
	nl_socket_modify_cb(nl, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, seq_check, NULL);

	struct nl_msg *msg = nlmsg_alloc();
	nl_perror("nlmsg_alloc");
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, commands[cmd].nl_cmd, /* vers */ 1);

	nla_put_string(msg, IEEE80215_ATTR_DEV_NAME, iface);

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
