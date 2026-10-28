// SPDX-License-Identifier: GPL-2.0-only
/* Shared deterministic input generator: no firmware or hardware access. */
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "prep.h"
#include "boot.h"
#include "vbios.h"
#ifdef TU_UPSTREAM_BOOT
#include <linux/firmware.h>
#endif

static void w32(u8 *p, u32 v) { unsigned i; for (i=0;i<4;i++) p[i]=(u8)(v>>(8*i)); }
static void layout(unsigned n, const struct turing_unlock_layout_in *in)
{
 struct turing_unlock_layout_out o; int r=turing_unlock_layout(in,&o);
 printf("layout %u %d",n,r);
 if (!r) printf(" %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
  (uint64_t)o.reserved_top,(uint64_t)o.wpr_end,(uint64_t)o.frts_offset,(uint64_t)o.boot_offset,(uint64_t)o.gsp_offset,(uint64_t)o.heap_offset,(uint64_t)o.heap_size,(uint64_t)o.wpr_start,(uint64_t)o.nonwpr_start);
 putchar('\n');
}
static void radix(unsigned n, u64 size)
{
 struct turing_unlock_radix_plan p; int r=turing_unlock_radix_plan(size,&p);
 printf("radix %u %d",n,r);
 if (!r) printf(" %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
  (uint64_t)p.pages[0],(uint64_t)p.pages[1],(uint64_t)p.pages[2],(uint64_t)p.pages[3],(uint64_t)p.table_pages,(uint64_t)p.table_bytes,(uint64_t)p.data_bytes);
 putchar('\n');
}
static void elf_case(unsigned n, const u8 *buf, size_t len, const char *name)
{
 struct turing_unlock_span s; int r=turing_unlock_elf_section(buf,len,name,&s);
 printf("elf %u %d",n,r);
 if (!r) printf(" %" PRIu64 " %" PRIu64,(uint64_t)s.offset,(uint64_t)s.size);
 putchar('\n');
}
static void booter(unsigned n, const u8 *data, size_t size)
{
 struct turing_unlock_booter_hdr h; int r;
#ifdef TU_UPSTREAM_BOOT
 struct firmware fw={size,data}; r=turing_unlock_booter_parse(&fw,&h);
#else
 r=turing_unlock_booter_parse(data,size,&h);
#endif
 printf("booter %u %d",n,r);
 if (!r) printf(" %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
  h.os_code_off,h.os_code_size,h.os_data_off,h.os_data_size,h.num_apps,h.app_code_off,h.app_code_size,h.app_data_off,h.app_data_size,h.patch_loc,h.ucode_id,h.engine_id,h.num_sigs,h.sig_bytes);
 putchar('\n');
}
static void desc(unsigned n, const u8 *data, size_t size, u32 image_size)
{
 struct turing_unlock_boot_desc d; int r=turing_unlock_boot_desc_v4(data,size,image_size,&d);
 printf("desc %u %d",n,r);
 if (!r) printf(" %u %u %u %u",d.code_offset,d.data_offset,d.manifest_offset,d.image_size);
 putchar('\n');
}
static void fwsec(unsigned n, const u8 *data, size_t size)
{
 struct turing_unlock_fwsec f; int r=turing_unlock_vbios_fwsec(data,size,&f);
 printf("fwsec %u %d",n,r);
 if (!r) printf(" %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
 f.desc_version,f.image_off,f.image_size,f.imem_pa,f.imem_size,f.imem_va,f.imem_sec_base,f.imem_sec_size,f.imem_ns_size,f.imem_sec_pa,f.dmem_pa,f.data_off,f.dmem_size,f.pkc_off,f.interface_off,f.ucode_id,f.engine_mask,f.sig_count,f.sig_versions,f.sig_off,f.sig_bytes);
 putchar('\n');
}
static void fw_fixture(u8 *r, unsigned version)
{
 unsigned i, sum=0;
 memset(r,0,2048); r[0]=0x55; r[1]=0xaa; r[24]=32; memcpy(r+32,"PCIR",4); r[48]=4; r[53]=0x80;
 r[64]=0xff; r[65]=0xb8; memcpy(r+66,"BIT",3); r[72]=12; r[73]=6; r[74]=1;
 for(i=64;i<75;i++) sum+=r[i];
 r[75]=(u8)(0-sum);
 r[76]=0x70; r[77]=2; r[78]=4; r[80]=96; w32(r+96,128);
 r[128]=1; r[129]=6; r[130]=6; r[131]=1; r[134]=0x85; w32(r+136,256);
 if(version==2) { w32(r+256,(60u<<16)|(2u<<8)|1); w32(r+260,512); w32(r+280,256); w32(r+292,256); w32(r+296,256); w32(r+304,256); }
 else { w32(r+256,(428u<<16)|(3u<<8)|1); w32(r+260,768); w32(r+276,256); w32(r+288,512); r[292]=1; r[294]=1; r[295]=1; r[296]=1; }
}
int main(void)
{
 static const u64 sizes[]={0,1,4095,4096,4097,(1ull<<21)-1,1ull<<21,(1ull<<21)+1,(1ull<<30)-1,1ull<<30,(1ull<<30)+1,(1ull<<39)-1,1ull<<39,(1ull<<39)+1,UINT64_MAX-4095,UINT64_MAX};
 struct turing_unlock_layout_in base={22ull<<30,21ull<<30,0,0,1ull<<20,4096,1ull<<20,1ull<<20,1ull<<20}, in;
 struct turing_unlock_layout_out out; struct turing_unlock_radix_plan p; struct turing_unlock_span span;
 u8 valid[256]={0}, e[256], rom[2048], original[2048];
 static u8 boot[72+TU_BOOTER_IMAGE_SIZE+TU_BOOTER_SIG_TOTAL+1];
 const size_t boot_size=sizeof boot-1;
 static const u32 fields[14]={0,256,34048,25088,1,256,33792,256,0,34560,13,1,1,16};
 u8 descriptor[76]={4}; unsigned i,j,n=0;
 /* Independent fixed examples catch identical regressions in both copies. */
 if (turing_unlock_radix_plan(4097,&p) || p.pages[3]!=2 || p.table_bytes!=12288 || p.data_bytes!=8192) return 2;
 if (turing_unlock_layout(&base,&out) || out.wpr_end!=(21ull<<30) || out.nonwpr_start!=(21ull<<30)-6291456) return 2;
 for (i=0;i<sizeof sizes/sizeof sizes[0];i++) radix(i,sizes[i]);
 layout(n++,&base);
 for(i=0;i<9;i++) for(j=0;j<sizeof sizes/sizeof sizes[0];j++) {
  in=base;
  switch(i) { case 0: in.usable_fb=sizes[j]; break; case 1: in.vga_base=sizes[j]; break;
   case 2: in.mmu_lock_valid=1; in.mmu_lock_low=sizes[j]; break; case 3: in.frts_size=sizes[j]; break;
   case 4: in.boot_size=sizes[j]; break; case 5: in.image_size=sizes[j]; break;
   case 6: in.nonwpr_size=sizes[j]; break; case 7: in.heap_request=sizes[j]; break;
   default: in.mmu_lock_valid=0; in.mmu_lock_low=sizes[j]; break; }
  layout(n++,&in);
 }
 memcpy(valid,"\177ELF",4); valid[4]=2; valid[5]=1; valid[6]=1; w32(valid+20,1); valid[52]=64;
 valid[40]=64; valid[58]=64; valid[60]=2; w32(valid+64+24,192); w32(valid+64+32,8);
 memcpy(valid+192,"\0.test\0",7); w32(valid+128,1); w32(valid+128+24,208); w32(valid+128+32,4);
 if (turing_unlock_elf_section(valid,sizeof valid,".test",&span) || span.offset!=208 || span.size!=4) return 2;
 n=0; elf_case(n++,valid,sizeof valid,".test"); elf_case(n++,valid,sizeof valid,".missing");
 for(i=0;i<=sizeof valid;i++) elf_case(n++,valid,i,".test");
 /* Mutations include headers, section bounds, names and section contents. */
 for(i=0;i<sizeof valid;i++) for(j=0;j<3;j++) {
  memcpy(e,valid,sizeof e); e[i]=(u8)(j==0 ? 0 : j==1 ? 0xff : e[i]^0x80);
  elf_case(n++,e,sizeof e,".test");
 }
 n=0; w32(boot,0x314c5442); w32(boot+4,1);
 for(i=0;i<14;i++) w32(boot+8+4*i,fields[i]);
 w32(boot+64,TU_BOOTER_IMAGE_SIZE); w32(boot+68,TU_BOOTER_SIG_TOTAL);
 booter(n++,boot,boot_size); for(i=0;i<80;i++) booter(n++,boot,i);
 booter(n++,boot,boot_size-1); booter(n++,boot,sizeof boot);
 for(i=0;i<72;i++) { boot[i]^=0x80; booter(n++,boot,boot_size); boot[i]^=0x80; }
 n=0; for(i=0;i<=77;i++) desc(n++,descriptor,i,4096);
 for(i=0;i<sizeof sizes/sizeof sizes[0];i++) desc(n++,descriptor,sizeof descriptor,(u32)sizes[i]);
 for(i=0;i<76;i++) { descriptor[i]^=0x80; desc(n++,descriptor,sizeof descriptor,4096); descriptor[i]^=0x80; }
 for(i=0;i<1024;i++) printf("ready %u %u\n",i,(unsigned)turing_unlock_fb_range_ready(i));
 printf("ready max %u\n",(unsigned)turing_unlock_fb_range_ready(UINT32_MAX));
 printf("ready badf %u\n",(unsigned)turing_unlock_fb_range_ready(0xbadf03f0u));
 n=0; for(j=2;j<=3;j++) {
  fw_fixture(original,j);
  { struct turing_unlock_fwsec f;
    if (turing_unlock_vbios_fwsec(original,sizeof original,&f) || f.desc_version!=j ||
        f.image_off!=(j==2?316u:684u) || f.image_size!=(j==2?512u:768u)) return 2;
  }
  fwsec(n++,original,sizeof original);
  for(i=0;i<=sizeof original;i++) fwsec(n++,original,i);
  for(i=0;i<512;i++) { memcpy(rom,original,sizeof rom); rom[i]^=0x80; fwsec(n++,rom,sizeof rom); }
 }
 return ferror(stdout) ? 1 : 0;
}
