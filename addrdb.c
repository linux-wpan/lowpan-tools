/*
 * Linux IEEE 802.15.4 userspace tools
 *
 * Copyright (C) 2008 Dmitry Eremin-Solenikov
 * Copyright (C) 2008 Sergey Lapin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <libcommon.h>
#include <ieee802154.h>

struct lease {
	uint8_t hwaddr[IEEE80215_ADDR_LEN];
	uint16_t short_addr;
	time_t time;
};

struct simple_hash *hwa_hash;
struct simple_hash *shorta_hash;

unsigned int hw_hash(const void *key)
{
	const uint8_t *hwa = key;
	int i;
	unsigned int val = 0;

	for (i = 0; i < IEEE80215_ADDR_LEN; i++) {
		val = (val * 31) | hwa[i];
	}

	return val;
}

int hw_eq(const void *key1, const void *key2)
{
	return memcmp(key1, key2, IEEE80215_ADDR_LEN);
}

unsigned int short_hash(const void *key)
{
	const uint16_t *addr = key;

	return *addr;
}

int short_eq(const void *key1, const void *key2)
{
	const uint16_t *addr1 = key1, *addr2 = key2;
	return addr1 - addr2;
}

static int last_addr = 0x8000;
uint16_t addrdb_alloc(uint8_t *hwa)
{
	struct lease *lease = shash_get(hwa_hash, hwa);
	if (lease) {
		lease->time = time(NULL);
		return lease->short_addr;
	}

	int addr = last_addr + 1;

	while (shash_get(shorta_hash, &addr)) {
		addr ++;
		if (addr == last_addr)
			return 0xffff;
		else if (addr == 0xfffe)
			addr = 0x8000;
	}

	lease = calloc(1, sizeof(*lease));
	memcpy(lease->hwaddr, hwa, IEEE80215_ADDR_LEN);
	lease->short_addr = addr;
	lease->time = time(NULL);

	last_addr = addr;

	shash_insert(hwa_hash, &lease->hwaddr, lease);
	shash_insert(shorta_hash, &lease->short_addr, lease);

	return addr;
}

static void addrdb_free(struct lease *lease)
{
	shash_drop(hwa_hash, &lease->hwaddr);
	shash_drop(shorta_hash, &lease->short_addr);
	free(lease);
}

void addrdb_free_hw(uint8_t *hwa)
{
	struct lease *lease = shash_get(hwa_hash, hwa);
	if (!lease) {
		fprintf(stderr, "Can't remove unknown HWA\n");
		return;
	}

	addrdb_free(lease);
}
void addrdb_free_short(uint16_t short_addr)
{
	struct lease *lease = shash_get(shorta_hash, &short_addr);
	if (!lease) {
		fprintf(stderr, "Can't remove unknown short address %04x\n", short_addr);
		return;
	}

	addrdb_free(lease);
}


void addrdb_init(/*uint8_t *hwa, uint16_t short_addr*/void)
{
	hwa_hash = shash_new(hw_hash, hw_eq);
	if (!hwa_hash) {
		fprintf(stderr, "Error initialising hash\n");
		exit(1);
	}

	shorta_hash = shash_new(short_hash, short_eq);
	if (!shorta_hash) {
		fprintf(stderr, "Error initialising hash\n");
		exit(1);
	}
}

