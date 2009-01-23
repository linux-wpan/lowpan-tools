#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <errno.h>

#include <ieee802154.h>
#define IEEE80215_NL_WANT_POLICY
#include <ieee80215-nl.h>
#include <libcommon.h>

static int seq_expected;
static int family;
static struct nl_handle *nl;
static const char *iface;

static int coordinator_associate(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	printf("Associate requested\n");

	if (!attrs[IEEE80215_ATTR_DEV_INDEX] ||
	    !attrs[IEEE80215_ATTR_SRC_HW_ADDR])
		return -EINVAL;

	// FIXME: checks!!!

	struct nl_msg *msg = nlmsg_alloc();

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_REQUEST, IEEE80215_ASSOCIATE_RESP, /* vers */ 1);

	nla_put_u32(msg, IEEE80215_ATTR_DEV_INDEX, nla_get_u32(attrs[IEEE80215_ATTR_DEV_INDEX]));
	nla_put_u32(msg, IEEE80215_ATTR_STATUS, 0x0 /* FIXME: status */);
	nla_put_u64(msg, IEEE80215_ATTR_DEST_HW_ADDR, nla_get_u64(attrs[IEEE80215_ATTR_SRC_HW_ADDR]));
	nla_put_u16(msg, IEEE80215_ATTR_DEST_SHORT_ADDR, 0xfffe); // FIXME: allocate short address

	nl_send_auto_complete(nl, msg);
	nl_perror("nl_send_auto_complete");
	return 0;
}

static int coordinator_disassociate(struct genlmsghdr *ghdr, struct nlattr **attrs)
{
	printf("Disassociate requested\n");

	if (!attrs[IEEE80215_ATTR_DEV_INDEX] ||
	    !attrs[IEEE80215_ATTR_REASON])
		return -EINVAL;

	// FIXME: checks!!!
	// FIXME: disassociate device

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

	name = nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]);
	if (strcmp(name, iface)) {
		return 0;
	}

	switch (ghdr->cmd) {
		case IEEE80215_ASSOCIATE_INDIC:
			return coordinator_associate(ghdr, attrs);
		case IEEE80215_DISASSOCIATE_INDIC:
			return coordinator_disassociate(ghdr, attrs);
	}

	if (!attrs[IEEE80215_ATTR_HW_ADDR])
		return -EINVAL;

	uint64_t addr = nla_get_u64(attrs[IEEE80215_ATTR_HW_ADDR]);
	uint8_t buf[8];
	memcpy(buf, &addr, 8);

	printf("Addr for %s is %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			nla_get_string(attrs[IEEE80215_ATTR_DEV_NAME]),
			buf[0], buf[1],	buf[2], buf[3],
			buf[4], buf[5],	buf[6], buf[7]);

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

	if (argc != 2) {
		printf("Usage: %s iface\n", argv[0]);
		return 1;
	}

	iface = argv[1];

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

	while (1) {
		nl_recvmsgs_default(nl);
	}


	nl_close(nl);

	return 0;
}
