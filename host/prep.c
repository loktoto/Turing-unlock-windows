// SPDX-License-Identifier: GPL-2.0-only
#include "diagnostic.h"
#ifdef __KERNEL__
#include <linux/errno.h>
#include <linux/string.h>
#else
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#endif

#include "prep.h"

static u16 rd16(const u8 *p)
{
	return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 rd32(const u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u64 rd64(const u8 *p)
{
	return (u64)rd32(p) | ((u64)rd32(p + 4) << 32);
}

int turing_unlock_elf_section(const u8 *elf, size_t length, const char *name,
			   struct turing_unlock_span *span)
{
	u64 shoff;
	u16 shentsize, shnum, shstrndx, i;
	u32 name_off;
	const u8 *shstr;
	u64 shstr_off, shstr_size;
	bool found = false;

	if (!elf || !name || !span || length < 64)
		return GU_DIAG(-EINVAL);
	if (elf[0] != 0x7f || elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F')
		return GU_DIAG(-EINVAL);
	if (elf[4] != 2 || elf[5] != 1 || elf[6] != 1 ||
	    rd32(elf + 20) != 1 || rd16(elf + 52) != 64)
		return GU_DIAG(-EINVAL);
	shoff = rd64(elf + 40);
	shentsize = rd16(elf + 58);
	shnum = rd16(elf + 60);
	shstrndx = rd16(elf + 62);
	if (shentsize != 64 || !shnum || shstrndx >= shnum)
		return GU_DIAG(-EINVAL);
	if (shoff > length || (u64)shnum * shentsize > length - shoff)
		return GU_DIAG(-EINVAL);
	shstr_off = rd64(elf + shoff + (u64)shstrndx * shentsize + 24);
	shstr_size = rd64(elf + shoff + (u64)shstrndx * shentsize + 32);
	if (shstr_off > length || shstr_size > length - shstr_off)
		return GU_DIAG(-EINVAL);
	shstr = elf + shstr_off;
	for (i = 0; i < shnum; i++) {
		const u8 *sh = elf + shoff + (u64)i * shentsize;
		u64 off = rd64(sh + 24);
		u64 size = rd64(sh + 32);
		size_t namelen = strlen(name);

		name_off = rd32(sh);
		if ((u64)name_off >= shstr_size)
			return GU_DIAG(-EINVAL);
		if (!memchr(shstr + name_off, 0, shstr_size - name_off))
			return GU_DIAG(-EINVAL);
		if (namelen >= shstr_size - name_off ||
		    memcmp(shstr + name_off, name, namelen) ||
		    shstr[name_off + namelen] != '\0')
			continue;
		if (found || off > length || size > length - off)
			return GU_DIAG(-EINVAL);
		span->offset = off;
		span->size = size;
		found = true;
	}
	return GU_DIAG(found ? 0 : -ENOENT);
}

int turing_unlock_radix_plan(u64 size, struct turing_unlock_radix_plan *plan)
{
	u64 pages[4];
	unsigned int level;
	u64 tables = 0;

	if (!plan || !size)
		return GU_DIAG(-EINVAL);
	if (size > ~(u64)0 - (GU_RADIX_PAGE - 1))
		return GU_DIAG(-EOVERFLOW);
	memset(plan, 0, sizeof(*plan));
	pages[3] = (size + GU_RADIX_PAGE - 1) >> 12;
	for (level = 3; level > 0; level--)
		pages[level - 1] = ((pages[level] - 1) >> 9) + 1;
	if (pages[0] != 1)
		return GU_DIAG(-ERANGE);
	for (level = 0; level < 3; level++)
		tables += pages[level];
	plan->pages[0] = pages[0];
	plan->pages[1] = pages[1];
	plan->pages[2] = pages[2];
	plan->pages[3] = pages[3];
	plan->table_pages = tables;
	plan->table_bytes = tables << 12;
	plan->data_bytes = pages[3] << 12;
	return 0;
}
