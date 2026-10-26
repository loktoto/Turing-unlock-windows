/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boot.h"
#include "prep.h"

#define CHECK(x) do { \
	if (!(x)) { \
		fprintf(stderr, "Failed line %d: %s\n", __LINE__, #x); \
		exit(1); \
	} \
} while (0)

#define BOOTER_SIZE (72u + TU_BOOTER_IMAGE_SIZE + TU_BOOTER_SIG_TOTAL)

static const u32 fields[] = {
	0x314c5442u, 1, 0, 256, 34048, 25088, 1, 256, 33792,
	256, 0, 34560, 13, 1, 1, TU_BOOTER_SIG_TOTAL,
	TU_BOOTER_IMAGE_SIZE, TU_BOOTER_SIG_TOTAL
};

static void write_le32(u8 *p, u32 value)
{
	unsigned int i;

	for (i = 0; i < 4; ++i)
		p[i] = (u8)(value >> (8 * i));
}

static void make_booter(u8 *data)
{
	size_t i;

	memset(data, 0, BOOTER_SIZE);
	for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
		write_le32(data + 4 * i, fields[i]);
}

static void reject_booter(const u8 *data, size_t size)
{
	struct turing_unlock_booter_hdr out, before;

	memset(&out, 0xa5, sizeof(out));
	memcpy(&before, &out, sizeof(out));
	CHECK(turing_unlock_booter_parse(data, size, &out) == -EINVAL);
	CHECK(memcmp(&out, &before, sizeof(out)) == 0);
}

static void test_booter(void)
{
	/* Deliberately unaligned input must also be portable. */
	u8 *allocation = malloc(BOOTER_SIZE + 2u);
	u8 *data;
	struct turing_unlock_booter_hdr out;
	size_t i;

	CHECK(allocation != NULL);
	data = allocation + 1;
	make_booter(data);
	CHECK(turing_unlock_booter_parse(data, BOOTER_SIZE, &out) == 0);
	CHECK(out.os_code_off == 0 && out.os_code_size == 256);
	CHECK(out.os_data_off == 34048 && out.os_data_size == 25088);
	CHECK(out.num_apps == 1 && out.app_code_off == 256);
	CHECK(out.app_code_size == 33792 && out.app_data_off == 256);
	CHECK(out.app_data_size == 0 && out.patch_loc == 34560);
	CHECK(out.ucode_id == 13 && out.engine_id == 1);
	CHECK(out.num_sigs == 1 && out.sig_bytes == TU_BOOTER_SIG_TOTAL);

	/* Every fixed word, including the envelope and both size copies. */
	for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
		write_le32(data + 4 * i, fields[i] ^ 1u);
		reject_booter(data, BOOTER_SIZE);
		write_le32(data + 4 * i, fields[i]);
	}
	/* Reject individually byte-swapped nonzero words and a BE header. */
	for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
		size_t j;

		if (!fields[i])
			continue;
		for (j = 0; j < 4; ++j)
			data[4 * i + j] = (u8)(fields[i] >> (8 * (3 - j)));
		reject_booter(data, BOOTER_SIZE);
		write_le32(data + 4 * i, fields[i]);
	}
	for (i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
		size_t j;

		for (j = 0; j < 4; ++j)
			data[4 * i + j] = (u8)(fields[i] >> (8 * (3 - j)));
	}
	reject_booter(data, BOOTER_SIZE);
	make_booter(data);

	/* A short actual allocation catches accidental reads before size checks. */
	for (i = 0; i < 72; ++i) {
		u8 *short_data = malloc(i ? i : 1);

		CHECK(short_data != NULL);
		memcpy(short_data, data, i);
		reject_booter(short_data, i);
		free(short_data);
	}
	reject_booter(data, 72);
	reject_booter(data, 72 + TU_BOOTER_IMAGE_SIZE - 1);
	reject_booter(data, 72 + TU_BOOTER_IMAGE_SIZE);
	reject_booter(data, BOOTER_SIZE - 1);
	reject_booter(data, BOOTER_SIZE + 1);
	reject_booter(data, SIZE_MAX);
	reject_booter(NULL, BOOTER_SIZE);
	CHECK(turing_unlock_booter_parse(data, BOOTER_SIZE, NULL) == -EINVAL);
	free(allocation);
}

static void reject_desc(const u8 *data, size_t size, u32 image_size)
{
	struct turing_unlock_boot_desc out, before;

	memset(&out, 0xa5, sizeof(out));
	memcpy(&before, &out, sizeof(out));
	CHECK(turing_unlock_boot_desc_v4(data, size, image_size, &out) == -EINVAL);
	CHECK(memcmp(&out, &before, sizeof(out)) == 0);
}

static void test_desc(void)
{
	u8 allocation[78], *data = allocation + 1;
	struct turing_unlock_boot_desc out;
	size_t i;

	/* V4 only checks the LE version; the remaining bytes are opaque. */
	memset(allocation, 0xa5, sizeof(allocation));
	write_le32(data, 4);
	CHECK(turing_unlock_boot_desc_v4(data, 76, 4096, &out) == 0);
	CHECK(out.code_offset == 0 && out.data_offset == 0);
	CHECK(out.manifest_offset == 0 && out.image_size == 4096);
	CHECK(turing_unlock_boot_desc_v4(data, 76, 0xfffff000u, &out) == 0);
	CHECK(out.image_size == 0xfffff000u);
	for (i = 0; i < 76; ++i)
		reject_desc(data, i, 4096);
	reject_desc(data, 77, 4096);
	reject_desc(data, SIZE_MAX, 4096);
	reject_desc(data, 76, 0);
	reject_desc(data, 76, 4095);
	reject_desc(data, 76, 4097);
	reject_desc(data, 76, UINT32_MAX);
	write_le32(data, 3);
	reject_desc(data, 76, 4096);
	write_le32(data, 0x04000000u);
	reject_desc(data, 76, 4096);
	reject_desc(NULL, 76, 4096);
	write_le32(data, 4);
	CHECK(turing_unlock_boot_desc_v4(data, 76, 4096, NULL) == -EINVAL);
}

int main(void)
{
	test_booter();
	test_desc();
	puts("Boot parser contracts passed (synthetic fixtures only)");
	return 0;
}
