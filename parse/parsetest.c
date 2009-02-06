#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "addrdb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


void dump_leases();
uint16_t addrdb_alloc(uint8_t *hwa);
extern int yydebug;

int main()
{
	void do_parse();
	unsigned char gwa[8];
	int i, fd;
//	yydebug = 1;
	fd = open("/dev/urandom", O_RDONLY);
	if(fd < 0)
		goto method2;
	if(read(fd, gwa, 8) < 0)
		goto method2;
	close(fd);
	goto testing;
method2:
	memcpy(gwa, "whack000", 8);
testing:
	addrdb_init();
	for (i = 0; i < 80; i++) {
		gwa[0] = i;
		printf("allocating %d\n", addrdb_alloc(gwa));
	}
	dump_leases();
	do_parse();
	return 0;
}

