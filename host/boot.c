// SPDX-License-Identifier: GPL-2.0-only
#include <errno.h>

#include "boot.h"
#include "diagnostic.h"
#include "prep.h"

#define BOOTER_MAGIC 0x314c5442u
#define BOOTER_HEADER_SIZE 72u

static u32 read_le32(const u8 *p)
{
	return (u32)p[0] | (u32)p[1] << 8 |
	       (u32)p[2] << 16 | (u32)p[3] << 24;
}

int turing_unlock_booter_parse(const u8 *data, size_t size,
				  struct turing_unlock_booter_hdr *hdr)
{
	struct turing_unlock_booter_hdr parsed;
	u32 image_size, sig_bytes;

	if (!data || !hdr || size < BOOTER_HEADER_SIZE)
		return GU_DIAG(-EINVAL);
	if (read_le32(data) != BOOTER_MAGIC || read_le32(data + 4) != 1)
		return GU_DIAG(-EINVAL);

	/* The on-disk header is little endian, independent of host layout. */
	parsed.os_code_off = read_le32(data + 8);
	parsed.os_code_size = read_le32(data + 12);
	parsed.os_data_off = read_le32(data + 16);
	parsed.os_data_size = read_le32(data + 20);
	parsed.num_apps = read_le32(data + 24);
	parsed.app_code_off = read_le32(data + 28);
	parsed.app_code_size = read_le32(data + 32);
	parsed.app_data_off = read_le32(data + 36);
	parsed.app_data_size = read_le32(data + 40);
	parsed.patch_loc = read_le32(data + 44);
	parsed.ucode_id = read_le32(data + 48);
	parsed.engine_id = read_le32(data + 52);
	parsed.num_sigs = read_le32(data + 56);
	parsed.sig_bytes = read_le32(data + 60);
	image_size = read_le32(data + 64);
	sig_bytes = read_le32(data + 68);
	if (image_size != TU_BOOTER_IMAGE_SIZE || sig_bytes != TU_BOOTER_SIG_TOTAL ||
	    sig_bytes != parsed.sig_bytes ||
	    size != (size_t)BOOTER_HEADER_SIZE + image_size + sig_bytes)
		return GU_DIAG(-EINVAL);
	if (parsed.ucode_id != 13 || parsed.engine_id != 1 || parsed.num_apps != 1 ||
	    parsed.num_sigs != 1 || parsed.os_code_off != 0 ||
	    parsed.os_code_size != 256 || parsed.os_data_off != 34048 ||
	    parsed.os_data_size != 25088 || parsed.app_code_off != 256 ||
	    parsed.app_code_size != 33792 || parsed.app_data_off != 256 ||
	    parsed.app_data_size != 0 || parsed.patch_loc != 34560)
		return GU_DIAG(-EINVAL);
	*hdr = parsed;
	GU_NOTE("booter geometry ok: image=%u sig=%u patch_loc=%#x\n",
		image_size, sig_bytes, hdr->patch_loc);
	return 0;
}

int turing_unlock_boot_desc_v4(const u8 *data, size_t size, u32 image_size,
				 struct turing_unlock_boot_desc *out)
{
	u32 version;

	if (!data || !out || size != 76 || !image_size || image_size & 4095)
		return GU_DIAG(-EINVAL);
	version = read_le32(data);
	if (version != 4)
		return GU_DIAG(-EINVAL);
	out->code_offset = 0;
	out->data_offset = 0;
	out->manifest_offset = 0;
	out->image_size = image_size;
	return 0;
}

bool turing_unlock_fb_range_ready(u32 range)
{
	return range != ~0u && (range & 0xffff0000u) != 0xbadf0000u &&
		((range >> 4) & 0x3fu);
}
