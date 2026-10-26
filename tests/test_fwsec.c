/* SPDX-License-Identifier: GPL-2.0-only */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vbios.h"

#define CHECK(x) do { if (!(x)) { \
 fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static unsigned checks;
static void put16(u8 *p, unsigned v) { p[0]=(u8)v; p[1]=(u8)(v>>8); }
static void put(u8 *p, u32 v)
{
 unsigned i;
 for (i=0; i<4; i++) p[i]=(u8)(v>>(8*i));
}

/* All containers, descriptors and image bodies are synthetic zero-filled data. */
static void image(u8 *r, unsigned blocks, unsigned type, unsigned last)
{
 r[0]=0x55; r[1]=0xaa; r[24]=32;
 memcpy(r+32,"PCIR",4); put16(r+48,blocks);
 r[52]=(u8)type; r[53]=(u8)last;
}

static void bit_table(u8 *r, unsigned bit, unsigned ptr,
                      unsigned table, unsigned descriptor, unsigned adjust)
{
 unsigned i, sum=0;
 r[bit]=0xff; r[bit+1]=0xb8; memcpy(r+bit+2,"BIT",3);
 r[bit+8]=12; r[bit+9]=6; r[bit+10]=1;
 for (i=0; i<11; i++) sum+=r[bit+i];
 r[bit+11]=(u8)(0-sum);
 r[bit+12]=0x70; r[bit+13]=2; r[bit+14]=4;
 put16(r+bit+16,ptr);
 put(r+ptr,table-adjust);
 r[table]=1; r[table+1]=6; r[table+2]=6; r[table+3]=1;
 r[table+6]=0x85; put(r+table+8,descriptor-adjust);
}

static void descriptor(u8 *d, unsigned version)
{
 if (version==2) {
  put(d,(60u<<16)|(2<<8)|1); put(d+4,512);
  put(d+24,256); put(d+36,256); put(d+40,256); put(d+48,256);
 } else {
  put(d,(428u<<16)|(3<<8)|1); put(d+4,768);
  put(d+20,256); put(d+32,512);
  d[36]=1; d[38]=1; d[39]=1; d[40]=1;
 }
}

static void fixture(u8 *r, unsigned version)
{
 memset(r,0,2048); image(r,4,0,0x80);
 bit_table(r,64,96,128,256,0); descriptor(r+256,version);
}

/* NPDE splits the advertised 1024-byte base at 512. A further image creates
 * ext-base = 1024-512 = 512, applied to BOTH table and descriptor pointers.
 * Decoy-free raw pointers land on zero bytes, so omitting either adjustment fails.
 */
static void chain_fixture(u8 *r, unsigned version, unsigned revision, unsigned len)
{
 memset(r,0,4096); image(r,2,0,0x80); put16(r+42,22);
 memcpy(r+64,"NPDE",4); put16(r+68,revision); put16(r+70,len);
 put16(r+72,1); r[74]=0;
 image(r+512,1,3,0); r[512]=0x56; r[513]=0x4e;
 memcpy(r+544,"NPDS",4);
 image(r+1024,6,0xe0,0x80); r[1024]=0x77; r[1025]=0xbb;
 memcpy(r+1056,"RGIS",4);
 bit_table(r,128,160,1152,1280,512); descriptor(r+1280,version);
}

static void success(const u8 *r, size_t n, unsigned version, unsigned off)
{
 struct turing_unlock_fwsec f;
 CHECK(turing_unlock_vbios_fwsec(r,n,&f)==0); checks++;
 CHECK(f.desc_version==version && f.imem_size==256 && f.data_off==256);
 CHECK(f.image_off==off+(version==2?60u:428u));
 CHECK(f.image_size==(version==2?512u:768u));
 CHECK(f.dmem_size==(version==2?256u:512u));
 if (version==2) {
  CHECK(f.imem_sec_size==256 && f.imem_ns_size==0 && f.imem_sec_pa==0);
 } else {
  CHECK(f.sig_bytes==384 && f.sig_off==off+44 && f.sig_count==1);
  CHECK(f.engine_mask==1 && f.ucode_id==1 && f.sig_versions==1);
 }
}

static void rejected(const u8 *r, size_t n, int expected)
{
 struct turing_unlock_fwsec f, zero;
 int ret;
 memset(&f,0xa5,sizeof f); memset(&zero,0,sizeof zero);
 ret=turing_unlock_vbios_fwsec(r,n,&f); checks++;
 CHECK(ret==expected); CHECK(memcmp(&f,&zero,sizeof f)==0);
}

struct bad_field { unsigned off; u32 value; int error; };
static void malformed_descriptors(void)
{
 static const struct bad_field common[]={
  {0,0,-EOPNOTSUPP}, {0,(60u<<16)|(4<<8)|1,-EOPNOTSUPP},
  {4,0,-EINVAL}, {4,UINT32_MAX,-EINVAL}, {4,2048,-EINVAL}
 };
 static const struct bad_field v2[]={
  {0,(59u<<16)|(2<<8)|1,-EINVAL}, {0,(2048u<<16)|(2<<8)|1,-EINVAL},
  {16,253,-EINVAL}, /* interface extends beyond DMEM */
  {20,1,-EINVAL},   /* IMEM physical range exceeds aligned code allocation */
  {24,0,-EINVAL}, {24,UINT32_MAX,-EINVAL},
  {28,1,-EINVAL},   /* secure base below virtual base */
  {32,UINT32_MAX,-EINVAL}, {36,257,-EINVAL},
  {40,257,-EINVAL}, {44,1,-EINVAL}, {48,0,-EINVAL}, {48,UINT32_MAX,-EINVAL}
 };
 static const struct bad_field v3[]={
  {0,(43u<<16)|(3<<8)|1,-EINVAL}, {0,(2048u<<16)|(3<<8)|1,-EINVAL},
  {8,129,-EINVAL}, /* 384-byte PKC extends beyond 512-byte DMEM */
  {8,UINT32_MAX,-EINVAL}, {12,509,-EINVAL}, {20,0,-EINVAL},
  {20,UINT32_MAX,-EINVAL}, {32,0,-EINVAL}, {32,513,-EINVAL},
  {36,0x01010000,-EINVAL}, /* zero engine mask */
  {36,0x01000001,-EINVAL}, /* zero ucode ID */
  {36,0x01110001,-EINVAL}, /* ucode ID > 16 */
  {36,0x00010001,-EINVAL}, /* zero signature count */
  {36,0x02010001,-EINVAL}, /* signature count differs from mask population */
  {40,0,-EINVAL}, {40,3,-EINVAL},
  {0,(427u<<16)|(3<<8)|1,-EINVAL} /* short signature bytes */
 };
 u8 r[2048]; unsigned v; size_t i;
 for (v=2; v<=3; v++) {
  const struct bad_field *cases=v==2?v2:v3;
  size_t count=v==2?sizeof v2/sizeof *v2:sizeof v3/sizeof *v3;
  for (i=0; i<sizeof common/sizeof *common; i++) {
   fixture(r,v); put(r+256+common[i].off,common[i].value);
   rejected(r,sizeof r,common[i].error);
  }
  for (i=0; i<count; i++) {
   fixture(r,v); put(r+256+cases[i].off,cases[i].value);
   rejected(r,sizeof r,cases[i].error);
  }
  /* An exact interface boundary and rounded stored size remain valid. */
  fixture(r,v); put(r+256+(v==2?16:12),v==2?252:508);
  put(r+260,v==2?511:767); success(r,sizeof r,v,256);
 }
 /* Two signature versions require two complete 384-byte signature records. */
 fixture(r,3); put(r+256,(812u<<16)|(3<<8)|1);
 r[295]=2; put16(r+296,0x8001);
 {
  struct turing_unlock_fwsec f;
  CHECK(turing_unlock_vbios_fwsec(r,sizeof r,&f)==0); checks++;
  CHECK(f.sig_count==2 && f.sig_versions==0x8001 && f.sig_bytes==768);
  CHECK(f.image_off==1068 && f.image_size==768);
 }
}

static void chains(void)
{
 u8 r[4096]; unsigned v, rev, len; size_t n;
 for (v=2; v<=3; v++) {
  for (rev=0x100; rev<=0x101; rev++) {
   for (len=10; len<=11; len++) {
    chain_fixture(r,v,rev,len); success(r,sizeof r,v,1280);
   }
  }
  chain_fixture(r,v,0x101,11);
  for (n=0; n<sizeof r; n++) rejected(r,n,-EINVAL);
  chain_fixture(r,v,0x101,9); rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,65535); rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); put16(r+72,0); rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); put16(r+72,9); rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); r[512]=0; rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); r[544]=0; rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); put(r+160,UINT32_MAX); rejected(r,sizeof r,-EINVAL);
  chain_fixture(r,v,0x101,11); put(r+1160,UINT32_MAX); rejected(r,sizeof r,-EINVAL);
  /* Unknown NPDE revision leaves the advertised PCIR extent intact. */
  chain_fixture(r,v,0x102,11); r[53]=0; put16(r+48,1);
  success(r,sizeof r,v,1280);
 }
 /* A ninth chained image is rejected even when every extent is in bounds. */
 {
  u8 many[9*512]; unsigned i;
  memset(many,0,sizeof many);
  for (i=0; i<9; i++) image(many+i*512,1,i?3:0,i==8?0x80:0);
  rejected(many,sizeof many,-EINVAL);
  many[7*512+53]=0x80;
  rejected(many,8*512,-ENOENT);
 }
 /* Extension position cannot precede the recorded base-image size. */
 memset(r,0,sizeof r);
 image(r,1,3,0); image(r+512,1,0xe0,0); image(r+1024,6,0,0x80);
 rejected(r,sizeof r,-EINVAL);
 /* The NPDE last flag can terminate a PCIR chain marked non-final. */
 for (v=2; v<=3; v++) {
  memset(r,0,sizeof r); image(r,8,0,0); put16(r+42,22);
  memcpy(r+64,"NPDE",4); put16(r+68,0x101); put16(r+70,11);
  put16(r+72,8); r[74]=0x80;
  bit_table(r,128,160,256,512,0); descriptor(r+512,v);
  success(r,sizeof r,v,512);
 }
}

int main(void)
{
 u8 r[2048]; unsigned v; size_t n;
 for (v=2; v<=3; v++) {
  fixture(r,v); success(r,sizeof r,v,256);
  for (n=0; n<sizeof r; n++) rejected(r,n,-EINVAL);
  fixture(r,v); r[75]^=1; rejected(r,sizeof r,-ENOENT);
  fixture(r,v); put(r+136,UINT32_MAX); rejected(r,sizeof r,-EINVAL);
 }
 malformed_descriptors(); chains();
 printf("FWSEC synthetic V2/V3 contracts passed (%u parser calls)\n",checks);
 return 0;
}
