#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <libcommon.h>

void printbuf(const unsigned char *buf, int len) {
	char outbuf[1024], outbuf2[1024];
	int olen = 0, olen2 = 0;
	int i;
	for (i = 0; i < len; ) {
		if (i % 8 == 0) {
			olen = olen2 = 0;
			outbuf[olen] = outbuf2[olen2] = '\0';
			olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "%03x: ", i);
			if (i > len - 8) {
				int j;
				for (j = len % 8; j < 8; j++)
					olen2 += snprintf(outbuf2 + olen2, sizeof(outbuf2) - olen2, "   ");
			}
			olen2 += snprintf(outbuf2 + olen2, sizeof(outbuf2) - olen2, "| ");
		}

		outbuf2[olen2++] = (buf[i] > ' ' && buf[i] < 0x7f) ? buf[i] : '.';
		olen += snprintf(outbuf + olen, sizeof(outbuf) - olen, "%02x ", buf[i++]);

		if ((i % 8 == 0) || (i == len)) {
			outbuf2[olen2] = '\0';
			printf("%s%s\n", outbuf, outbuf2);
		}
	}
}



