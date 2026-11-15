/* SPDX-License-Identifier: MIT
 * CPU-only Gen12 packet probe using Mesa 26.3 generated pack definitions.
 * Mesa gen120_pack.h carries Intel's MIT license and copyright notice.
 */
#include <stdint.h>
#include <stdio.h>
#define __gen_address_type uint64_t
#define __gen_user_data void
#define __gen_combine_address(data,loc,addr,delta) ((addr)+(delta))
#include "gen120_pack.h"

int main(void)
{
 uint32_t words[GFX12_RENDER_SURFACE_STATE_length]={0};
 struct GFX12_RENDER_SURFACE_STATE s={0};
 s.SurfaceType=SURFTYPE_BUFFER;
 s.SurfaceFormat=511; /* ISL_FORMAT_RAW, Mesa isl.h */
 s.SurfaceHorizontalAlignment=1; /* ISL halign(4) */
 s.SurfaceVerticalAlignment=1; /* ISL valign(4) */
 s.Width=15; /* 16-byte UBO, one-byte stride: elements - 1 */
 s.SurfacePitch=0; /* stride - 1 */
 s.MOCS=6; /* current native uncached policy */
 s.SurfaceBaseAddress=0x410000;
 GFX12_RENDER_SURFACE_STATE_pack(NULL,words,&s);
 for(unsigned i=0;i<GFX12_RENDER_SURFACE_STATE_length;i++)
  printf("%02u %08x\n",i,words[i]);
 return 0;
}
