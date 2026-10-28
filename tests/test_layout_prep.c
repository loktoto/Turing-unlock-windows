/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "layout.h"
#include "prep.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
#define M (1ull << 20)

static struct turing_unlock_layout_in layout_input(void)
{
 struct turing_unlock_layout_in in = {0};
 in.usable_fb = 16 * M; in.vga_base = 16 * M;
 in.frts_size = M; in.boot_size = M; in.image_size = M;
 in.nonwpr_size = M; in.heap_request = M;
 return in;
}

static void test_layout(void)
{
 struct turing_unlock_layout_in in = layout_input();
 struct turing_unlock_layout_out out;
 CHECK(turing_unlock_layout(&in, &out) == 0);
 CHECK(out.reserved_top == 16*M && out.wpr_end == 16*M);
 CHECK(out.frts_offset == 15*M && out.boot_offset == 14*M);
 CHECK(out.gsp_offset == 13*M && out.heap_offset == 12*M);
 CHECK(out.heap_size == M && out.wpr_start == 11*M && out.nonwpr_start == 10*M);
 in.mmu_lock_valid = true; in.mmu_lock_low = 12*M + 123;
 CHECK(turing_unlock_layout(&in, &out) == 0);
 CHECK(out.reserved_top == 12*M+123 && out.wpr_end == 12*M);
 CHECK(out.frts_offset == 11*M && out.boot_offset == 10*M && out.gsp_offset == 9*M);
 CHECK(out.heap_offset == 8*M && out.wpr_start == 7*M && out.nonwpr_start == 6*M);
 in.mmu_lock_low = 17*M;
 CHECK(turing_unlock_layout(&in, &out) == 0 && out.reserved_top == 16*M);
 in.mmu_lock_low = 0; in.mmu_lock_valid = false;
 CHECK(turing_unlock_layout(&in, &out) == 0 && out.reserved_top == 16*M);
 in.mmu_lock_valid = true;
 CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 CHECK(turing_unlock_layout(NULL, &out) == -EINVAL);
 CHECK(turing_unlock_layout(&in, NULL) == -EINVAL);
 in = layout_input(); in.usable_fb = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.vga_base = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.frts_size = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.boot_size = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.image_size = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.vga_base = 128*1024-1; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.frts_size = 17*M; CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.boot_size = 16*M; CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.boot_size = 15*M; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.image_size = 15*M; CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.image_size = 14*M; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.nonwpr_size = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.heap_request = 0; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.nonwpr_size = UINT64_MAX; CHECK(turing_unlock_layout(&in, &out) == -EOVERFLOW);
 in = layout_input(); in.heap_request = UINT64_MAX; CHECK(turing_unlock_layout(&in, &out) == -EOVERFLOW);
 in = layout_input(); in.heap_request = 14*M; CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.heap_request = 13*M; CHECK(turing_unlock_layout(&in, &out) == -EINVAL);
 in = layout_input(); in.heap_request = 12*M; in.nonwpr_size = 2*M;
 CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.usable_fb = 16*M-1; CHECK(turing_unlock_layout(&in, &out) == -ERANGE);
 in = layout_input(); in.nonwpr_size = 1; in.heap_request = 1;
 CHECK(turing_unlock_layout(&in, &out) == 0 && out.heap_size == M && out.nonwpr_start == 10*M);
}

static void test_radix(void)
{
 struct turing_unlock_radix_plan p;
 CHECK(turing_unlock_radix_plan(1, &p) == 0);
 CHECK(p.pages[0] == 1 && p.pages[1] == 1 && p.pages[2] == 1 && p.pages[3] == 1);
 CHECK(p.table_pages == 3 && p.table_bytes == 3*4096 && p.data_bytes == 4096);
 CHECK(turing_unlock_radix_plan(512ull*4096, &p) == 0);
 CHECK(p.pages[2] == 1 && p.pages[3] == 512 && p.table_pages == 3);
 CHECK(turing_unlock_radix_plan(512ull*4096+1, &p) == 0);
 CHECK(p.pages[2] == 2 && p.pages[3] == 513 && p.table_pages == 4 && p.data_bytes == 513ull*4096);
 CHECK(turing_unlock_radix_plan(1ull<<39, &p) == 0);
 CHECK(p.pages[0] == 1 && p.pages[1] == 512 && p.pages[2] == 262144 && p.pages[3] == 134217728);
 CHECK(p.table_pages == 262657 && p.table_bytes == 262657ull*4096 && p.data_bytes == (1ull<<39));
 CHECK(turing_unlock_radix_plan((1ull<<39)+1, &p) == -ERANGE);
 CHECK(turing_unlock_radix_plan(UINT64_MAX, &p) == -EOVERFLOW);
 CHECK(turing_unlock_radix_plan(0, &p) == -EINVAL);
 CHECK(turing_unlock_radix_plan(1, NULL) == -EINVAL);
}

static void w16(u8 *p, u16 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); }
static void w32(u8 *p, u32 v) { unsigned i; for (i=0; i<4; i++) p[i]=(u8)(v>>(8*i)); }
static void w64(u8 *p, u64 v) { unsigned i; for (i=0; i<8; i++) p[i]=(u8)(v>>(8*i)); }

/* Three section headers at 64, 128 and 192, then the names and data. */
static void make_elf(u8 *e)
{
 memset(e, 0, 320); memcpy(e, "\177ELF", 4);
 e[4]=2; e[5]=1; e[6]=1; w32(e+20,1); w16(e+52,64);
 w64(e+40,64); w16(e+58,64); w16(e+60,3); w16(e+62,1);
 w32(e+128,1); w64(e+128+24,256); w64(e+128+32,14);
 w32(e+192,7); w64(e+192+24,300); w64(e+192+32,4);
 memcpy(e+256,"\0.names\0.text\0",14);
}

static void test_elf(void)
{
 u8 e[320]; struct turing_unlock_span s;
 make_elf(e);
 /* Name offsets are exact: .text starts at byte 8. */
 w32(e+192,8);
 CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==0 && s.offset==300 && s.size==4);
 CHECK(turing_unlock_elf_section(e,sizeof e,".tex",&s)==-ENOENT);
 CHECK(turing_unlock_elf_section(e,sizeof e,"missing",&s)==-ENOENT);
 CHECK(turing_unlock_elf_section(NULL,sizeof e,".text",&s)==-EINVAL);
 CHECK(turing_unlock_elf_section(e,sizeof e,NULL,&s)==-EINVAL);
 CHECK(turing_unlock_elf_section(e,sizeof e,".text",NULL)==-EINVAL);
 CHECK(turing_unlock_elf_section(e,63,".text",&s)==-EINVAL);
 e[0]=0; CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); e[4]=1; CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); e[5]=2; CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); e[6]=0; CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+20,0); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w16(e+52,63); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w16(e+58,63); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w16(e+60,0); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w16(e+62,3); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w64(e+40,UINT64_MAX); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w64(e+40,129); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w64(e+128+24,321); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w64(e+128+32,65); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,14); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,8); e[269]='x'; CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,8); w64(e+192+24,321); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,8); w64(e+192+24,319); w64(e+192+32,2); CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,8); w32(e+64,8);
 CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==-EINVAL);
 make_elf(e); w32(e+192,8); w64(e+192+24,320); w64(e+192+32,0);
 CHECK(turing_unlock_elf_section(e,sizeof e,".text",&s)==0 && s.offset==320 && s.size==0);
}

int main(void)
{
 test_layout(); test_radix(); test_elf();
 puts("Layout and preparation contracts passed (synthetic fixtures only)");
 return 0;
}
