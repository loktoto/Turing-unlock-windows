/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prep.h"
#include "vbios.h"
#include "boot.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void w32(u8 *p,u32 v) { unsigned i; for(i=0;i<4;i++) p[i]=(u8)(v>>(8*i)); }
int main(void) {
 struct turing_unlock_radix_plan p;
 struct turing_unlock_layout_in in={0};
 struct turing_unlock_layout_out out;
 struct turing_unlock_fwsec fw;
 struct turing_unlock_booter_hdr h;
 struct turing_unlock_boot_desc desc;
 struct turing_unlock_span span;
 u8 rom[512]={0}, elf[256]={0}, d[76]={4};
 u8 *b=calloc(1,72+TU_BOOTER_IMAGE_SIZE+TU_BOOTER_SIG_TOTAL);
 u32 fields[14]={0,256,34048,25088,1,256,33792,256,0,34560,13,1,1,16};
 size_t i;
 CHECK(b);
 CHECK(turing_unlock_radix_plan(4097,&p)==0);
 CHECK(p.pages[0]==1 && p.pages[1]==1 && p.pages[2]==1 && p.pages[3]==2 && p.table_bytes==12288 && p.data_bytes==8192);
 CHECK(turing_unlock_radix_plan(0,&p)!=0);
 CHECK(turing_unlock_radix_plan(UINT64_MAX,&p)!=0);
 CHECK(turing_unlock_radix_plan((1ull<<39)+1,&p)!=0);
 in.usable_fb=22ull<<30; in.vga_base=21ull<<30; in.frts_size=1<<20;
 in.boot_size=4096; in.image_size=1<<20; in.nonwpr_size=1<<20; in.heap_request=1<<20;
 CHECK(turing_unlock_layout(&in,&out)==0);
 CHECK(out.wpr_end==(21ull<<30));
 CHECK(out.frts_offset==out.wpr_end-(1<<20));
 CHECK(out.boot_offset==out.frts_offset-4096);
 CHECK(out.gsp_offset==out.wpr_end-2162688);
 CHECK(out.heap_offset==out.wpr_end-4194304);
 CHECK(out.wpr_start==out.wpr_end-5242880);
 CHECK(out.nonwpr_start==out.wpr_end-6291456);
 in.heap_request=UINT64_MAX; CHECK(turing_unlock_layout(&in,&out)!=0);
 in.heap_request=1<<20; in.usable_fb=4096; CHECK(turing_unlock_layout(&in,&out)!=0);
 CHECK(turing_unlock_layout(NULL,&out)!=0);
 /* Synthetic ROM container: valid chain, deliberately no BIT table. */
 rom[0]=0x55;rom[1]=0xaa;rom[0x18]=0x20;memcpy(rom+0x20,"PCIR",4);rom[0x30]=1;rom[0x35]=0x80;
 CHECK(turing_unlock_vbios_fwsec(rom,sizeof rom,&fw)==-ENOENT);
 for(i=0;i<sizeof rom;i++) CHECK(turing_unlock_vbios_fwsec(rom,i,&fw)!=0);
 CHECK(turing_unlock_vbios_fwsec(NULL,0,&fw)!=0);
 w32(b,0x314c5442);w32(b+4,1);
 for(i=0;i<14;i++) w32(b+8+4*i,fields[i]);
 w32(b+64,TU_BOOTER_IMAGE_SIZE);w32(b+68,TU_BOOTER_SIG_TOTAL);
 CHECK(turing_unlock_booter_parse(b,72+TU_BOOTER_IMAGE_SIZE+16,&h)==0);
 CHECK(h.patch_loc==34560 && h.os_data_size==25088);
 CHECK(turing_unlock_booter_parse(b,71,&h)!=0);
 b[48]=12; CHECK(turing_unlock_booter_parse(b,72+TU_BOOTER_IMAGE_SIZE+16,&h)!=0);free(b);
 CHECK(turing_unlock_boot_desc_v4(d,76,4096,&desc)==0);
 CHECK(desc.image_size==4096 && desc.code_offset==0);
 CHECK(turing_unlock_boot_desc_v4(d,75,4096,&desc)!=0);
 CHECK(turing_unlock_boot_desc_v4(d,76,4097,&desc)!=0);
 memcpy(elf,"\177ELF",4);elf[4]=2;elf[5]=1;elf[6]=1;w32(elf+20,1);elf[52]=64;
 elf[40]=64;elf[58]=64;elf[60]=2;elf[62]=0;
 w32(elf+64+24,192);w32(elf+64+32,8);
 memcpy(elf+192,"\0.test\0",7);w32(elf+128,1);w32(elf+128+24,208);w32(elf+128+32,4);
 CHECK(turing_unlock_elf_section(elf,sizeof elf,".test",&span)==0);
 CHECK(span.offset==208 && span.size==4);
 w32(elf+128+32,UINT32_MAX);CHECK(turing_unlock_elf_section(elf,sizeof elf,".test",&span)!=0);
 puts("Host contracts passed (synthetic fixtures only)");return 0;
}
