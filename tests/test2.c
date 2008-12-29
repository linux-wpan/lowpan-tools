#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define PF_IEEE80215 36
#define AF_IEEE80215 PF_IEEE80215

struct sockaddr_ieee80215 {
	sa_family_t family; /* AF_IEEE80215 */
	int ifindex;
	uint64_t addr; /* little endian */
};


int main() {
	int ret;
	struct sockaddr_ieee80215 sa = {};

	char buf[] = {0x01, 0x80, 0xa5, 0x5a};
	int sd = socket(PF_IEEE80215, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return 1;
	}

	sa.family = AF_IEEE80215;
//	sa.ifindex = 2;
	sa.addr = 0xbebafecaafbeaddeLL;
	ret = bind(sd, (struct sockaddr*)&sa, sizeof(sa));
	if (ret < 0)
		perror("bind");

	ret = send(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		perror("send");

	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 0)
		perror("recv");

	ret = shutdown(sd, SHUT_RDWR);
	if (ret < 0)
		perror("shutdown");

	ret = close(sd);
	if (ret < 0)
		perror("close");

	return 0;

}
