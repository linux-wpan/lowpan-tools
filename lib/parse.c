#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libcommon.h>
#include <errno.h>
#include <stdio.h>

int parse_hw_addr(const char *hw, unsigned char *buf) {
	int i = 0;

	while (*hw) {
		unsigned char c = *(hw++);
		switch (c) {
			case '0'...'9':
				c -= '0';
				break;
			case 'a'...'f':
				c -= 'a' - 10;
				break;
			case 'A'...'F':
				c -= 'A' - 10;
				break;
			case ':':
			case '.':
				continue;
			default:
				fprintf(stderr, "Bad HW address encountered (%c)\n", c);
				return -EINVAL;
		}
		buf[i / 2] = (buf[i/2] & (0xf << (4 * (i % 2)))) | (c << 4 * (1 -i % 2));

		i++;
		if (i == 16)
			return 0;
	}

	return -EINVAL;
}

