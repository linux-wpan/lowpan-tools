#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <errno.h>

#include <ieee802154.h>
#define IEEE80215_NL_WANT_POLICY
#include <ieee80215-nl.h>
#include <libcommon.h>

static int end = 0;
static int seq_expected;

static int parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[IEEE80215_ATTR_MAX+1];
        struct genlmsghdr *ghdr;

	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, IEEE80215_ATTR_MAX, ieee80215_policy);

        ghdr = nlmsg_data(nlh);

	printf("Received command %d (%d)\n", ghdr->cmd, ghdr->version);

	if (ghdr->cmd == IEEE80215_ASSOCIATE_CONF ) {
		end = 1;
		printf("Received short address %04hx, status %02hhx\n",
			nla_get_u16(attrs[IEEE80215_ATTR_SHORT_ADDR]),
			nla_get_u8(attrs[IEEE80215_ATTR_STATUS]));
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

int main(int argc, char **argv) {
//	static int seq_expected;
	static int family;
	static struct nl_handle *nl;
	char *name;
	char *dummy;
	uint16_t pan_id, coord_short_addr;

	if (argc != 4) {
		printf("Usage: %s iface PANid CoordAddr\n", argv[0]);
		return 1;
	}

	name = argv[1];

	pan_id = strtol(argv[2], &dummy, 16);
	if (*dummy) {
		printf("Bad PAN id\n");
		return 1;
	}

	coord_short_addr = strtol(argv[3], &dummy, 16);
	if (*dummy) {
		printf("Bad CoordAddr\n");
		return 1;
	}

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
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, IEEE80215_ASSOCIATE_REQ, /* vers */ 1);
	nla_put_string(msg, IEEE80215_ATTR_DEV_NAME, name);
	nla_put_u8(msg, IEEE80215_ATTR_CHANNEL, 16);
	nla_put_u16(msg, IEEE80215_ATTR_COORD_PAN_ID, pan_id);
	nla_put_u16(msg, IEEE80215_ATTR_COORD_SHORT_ADDR, coord_short_addr);
	nla_put_u8(msg, IEEE80215_ATTR_CAPABILITY, 0
						| (1 << 1) /* FFD */
						| (1 << 3) /* Receiver ON */
//						| (1 << 7) /* allocate short */
						);

	nl_send_auto_complete(nl, msg);
	nl_perror("nl_send_auto_complete");

	nlmsg_free(msg);

	while (!end) {
		nl_recvmsgs_default(nl);
	}


	nl_close(nl);

	return 0;
}
