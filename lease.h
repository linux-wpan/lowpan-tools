#ifndef LEASE_H
#include <ieee802154.h>
#define LEASE_H
struct lease {
	uint8_t hwaddr[IEEE80215_ADDR_LEN];
	uint16_t short_addr;
	time_t time;
};
#endif
