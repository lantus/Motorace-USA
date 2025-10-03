#ifndef _BITPLANES_
#define _BITPLANES_

#include <clib/exec_protos.h>
 
UBYTE*  Bitplanes_Initialize(ULONG size);
void    Bitplanes_Free(UBYTE* bitplanes);
void    Bitplanes_SwapBuffers();
void    Bitplanes_Point (UBYTE* bitplanes, UWORD* BPL1PTH_addr, UBYTE bpl_number);

#endif