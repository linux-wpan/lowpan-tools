#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define ATTR_MAX 0

struct nla_policy policy[] = {
};

static int parse_cb(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[ATTR_MAX+1];
        struct genlmsghdr *ghdr;


	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, ATTR_MAX, policy);

        ghdr = nlmsg_data(nlh);

	printf("Received command %d (%d)\n", ghdr->cmd, ghdr->version);

	return 0;
}


int main(void) {

	struct nl_handle *nl = nl_handle_alloc();

	if (!nl) {
		nl_perror("nl_handle_alloc");
		return 1;
	}

	genl_connect(nl);
	nl_perror("genl_connect");

	int family = genl_ctrl_resolve(nl, "802.15.4 MAC");
	nl_perror("genl_ctrl_resolve");

	struct nl_msg *msg = nlmsg_alloc();
	nl_perror("nlmsg_alloc");
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, NLM_F_ECHO, /* cmd */ 0, /* vers */ 1);
	nla_put_u32(msg, /* attr */0, 123);

	nl_send_auto_complete(nl, msg);
	nl_perror("nl_send_auto_complete");

	nlmsg_free(msg);

	nl_socket_modify_cb(nl, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);

	// Wait for the answer and receive it
	nl_recvmsgs_default(nl);


	nl_close(nl);

	return 0;
}
