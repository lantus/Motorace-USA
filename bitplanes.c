#include "bitplanes.h"
#include "screen.h"
#include "copper.h"
#include "memory.h"
#include <clib/exec_protos.h>
 
ULONG bpsize = 0;

static void point_bitplanes_dp(UBYTE* bitplanes, UWORD* BPL1PTH_addr, int bpl_number) 
{
    ULONG addr;
    int bplptr_offset = 1;

    // Playfield 1, BPL 1,3,5,7 
    for (int i=0 ; i < 4;i++) 
    {
        addr = (ULONG) &(bitplanes[i * screen.row_size]);
        BPL1PTH_addr[bplptr_offset] = (addr >> 16) & 0xffff;
        BPL1PTH_addr[bplptr_offset + 2] = addr & 0xffff;
        bplptr_offset += 8; 
    }
    
    UWORD* BPL2PTH_addr = BPL1PTH_addr+4; 
    
    bitplanes += screen.framebuffer_size / 2;     
 
    bplptr_offset = 0;
    // Playfield 2, BPL 2,4,6,8  
    for (int i=0; i < 4; i++) {
        addr = (ULONG) &(bitplanes[i * screen.row_size]);
        BPL2PTH_addr[bplptr_offset] = (addr >> 16) & 0xffff;
        BPL2PTH_addr[bplptr_offset + 2] = addr & 0xffff;
        bplptr_offset += 8; // next bitplane
    }


}
 
void Bitplanes_Point(UBYTE* bitplanes, UWORD* BPL1PTH_addr, UBYTE bpl_number) 
{
    
    if (screen.dual_playfield) 
    {
        point_bitplanes_dp(bitplanes,BPL1PTH_addr,bpl_number);
        return;
    }

    ULONG addr;
    int bplptr_offset = 1;
    
    for (int i = 0; i < bpl_number; i++) 
    {
        addr = (ULONG) &(bitplanes[i * screen.row_size]) +  screen.offset;
        BPL1PTH_addr[bplptr_offset] = (addr >> 16) & 0xffff;
        BPL1PTH_addr[bplptr_offset + 2] = addr & 0xffff;
        bplptr_offset += 4; // next bitplane
    }
}

UBYTE* Bitplanes_Initialize(ULONG sz) 
{
    bpsize = sz<<1;                                    
    UBYTE* bpls = Mem_AllocChip(bpsize);
    return bpls;
}

void Bitplanes_Free(UBYTE* bitplanes) 
{
    Mem_Free(bitplanes, bpsize);
}

void Bitplanes_SwapBuffers() 
{
    int bitplaneOffset = 0;
    
    if (screenbuffer_idx == 0) 
    {
        screenbuffer_idx = 1;
    } 
    else 
    {
     //   bitplaneOffset = screen.framebuffer_size;
        screenbuffer_idx = 0;
    }

   // Bitplanes_Point(screen.bitplanes+bitplaneOffset, CopPLANE1H ,screen.depth);
}