#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "addrdb.h"

void dump_leases();
uint16_t addrdb_alloc(uint8_t *hwa);
extern int yydebug;

int main()
{
	void do_parse();
	unsigned char gwa[8];
	int i;
//	yydebug = 1;
	memcpy(gwa, "whack000", 8);
	addrdb_init();
	for (i = 0; i < 80; i++) {
		gwa[0] = i;
		printf("allocating %d\n", addrdb_alloc(gwa));
	}
	dump_leases();
	do_parse();
	return 0;
}

