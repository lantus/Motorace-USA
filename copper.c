#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include "bitmap.h"
#include "copper.h"
 
// Function to get custom chipset base address
extern volatile struct Custom *custom;

 
// Helper functions for setting bitplane pointers

__attribute__((always_inline)) inline void Copper_SetBitplanePointer(int plane, const UBYTE **address, LONG offset) 
{
    UWORD *wp = CopPLANE1H;

    for(int i = 0; i < plane; i++)
	{
		ULONG pl = (ULONG)address[i] + offset;
		
		wp[1] = pl >> 16;
		wp[3] = pl & 0xFFFF;

		wp += 4;
	}
 
}

__attribute__((always_inline)) inline void Copper_SetPalette(int index,USHORT color)
{
    UWORD *wp = CopCOLOR00;
    wp += ((2*index));
    wp[val] = color;
}

// Assuming 5 bitplanes, set the sprite palette to the upper 16 colors
__attribute__((always_inline)) inline void Copper_SetSpritePalette(int index,USHORT color)
{
    UWORD *wp = CopCOLOR16;
    wp += ((2*index));
    wp[val] = color;
}
 
// Helper function to set display window
void Copper_SetDisplayWindow(UWORD startX, UWORD startY, UWORD stopX, UWORD stopY) 
{
    CopDIWSTART[val] = (startY << 8) | startX;
    CopDIWSTOP[val] = (stopY << 8) | stopX;
}

// Helper function to set data fetch window
void Copper_SetDataFetch(UWORD start, UWORD stop) 
{
    CopDDFSTART[val] = start;
    CopDDFSTOP[val] = stop;
}
 