/* SPDX-License-Identifier: GPL-2.0-only */
#include "prep.h"
#include "vbios.h"
#include "boot.h"
int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size) {
 struct turing_unlock_fwsec fw;
 struct turing_unlock_span span;
 struct turing_unlock_booter_hdr hdr;
 struct turing_unlock_boot_desc desc;
 turing_unlock_vbios_fwsec(data,size,&fw);
 turing_unlock_elf_section(data,size,".text",&span);
 turing_unlock_booter_parse(data,size,&hdr);
 turing_unlock_boot_desc_v4(data,size,4096,&desc);
 return 0;
}
