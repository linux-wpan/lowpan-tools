#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

void printbuf(const unsigned char *buf, int len);

int parse_hw_addr(const char *addr, unsigned char *buf);

struct nl_handle;
int nl_get_multicast_id(struct nl_handle *handle, const char *family, const char *group);

#endif
