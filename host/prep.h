/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TURING_UNLOCK_PREP_H
#define TURING_UNLOCK_PREP_H

#include "types.h"

#include "layout.h"

#define GU_WPR_META_SIZE 256u
#define GU_WPR_META_MAGIC 0xdc3aae21371a60b3ull
#define GU_RADIX_PAGE 4096u

struct turing_unlock_booter_hdr {
	u32 os_code_off;
	u32 os_code_size;
	u32 os_data_off;
	u32 os_data_size;
	u32 num_apps;
	u32 app_code_off;
	u32 app_code_size;
	u32 app_data_off;
	u32 app_data_size;
	u32 patch_loc;
	u32 ucode_id;
	u32 engine_id;
	u32 num_sigs;
	u32 sig_bytes;
};

struct turing_unlock_span {
	u64 offset;
	u64 size;
};

struct turing_unlock_radix_plan {
	u64 pages[4];
	u64 table_pages;
	u64 table_bytes;
	u64 data_bytes;
};

/* Boot-image offsets carried in WPR metadata (version 4 descriptor). */
struct turing_unlock_boot_desc {
	u32 code_offset;
	u32 data_offset;
	u32 manifest_offset;
	u32 image_size;
};

struct turing_unlock_wpr_args {
	u64 fb_size, vga_offset, frts_size;
	u64 signature_dma, signature_size;
	u64 radix_dma, image_size;
	u64 boot_dma;
	struct turing_unlock_boot_desc boot;
};

int turing_unlock_elf_section(const u8 *elf, size_t length, const char *name,
			   struct turing_unlock_span *span);

int turing_unlock_radix_plan(u64 size, struct turing_unlock_radix_plan *plan);


#endif
