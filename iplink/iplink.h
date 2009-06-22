/*
 * Copyright (C) 2009 Siemens AG
 *
 * Written-by: Dmitry Eremin-Solenikov
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

#ifndef IPLINK_H
#define IPLINK_H

extern const char *ll_addr_n2a(unsigned char *addr, int alen, int type, char *buf, int blen);

struct link_util {
	struct link_util *next;
	const char *id;
	int maxattr;
	int (*parse_opt)(struct link_util *, int, char **,
			struct nlmsghdr *);
	void (*print_opt)(struct link_util *, FILE *,
			struct rtattr *[]);
	void (*print_xstats)(struct link_util *, FILE *,
			struct rtattr *);
};

#endif
