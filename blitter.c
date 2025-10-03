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
#include "game.h"
#include "sprites.h"
#include "copper.h"
#include "memory.h"
#include "hardware.h"
#include "bitmap.h"
#include "blitter.h"

extern volatile struct Custom *custom;

typedef struct {
    UBYTE *background_save;     // Saved background data
    UBYTE *screen_location;     // Where BOB was drawn on screen
    WORD x, y;                  // BOB position
    WORD width, height;         // BOB dimensions
    BOOL needs_restore;         // Flag if background needs restoring
} BOBRestore;
 
 

// BLTCON0/1 table for different pixel shifts (0-15)
static const ULONG bl_bob_table[16] = {
    0x0fca0000, 0x1fca1000, 0x2fca2000, 0x3fca3000,
    0x4fca4000, 0x5fca5000, 0x6fca6000, 0x7fca7000,
    0x8fca8000, 0x9fca9000, 0xafcaa000, 0xbfcab000,
    0xcfcac000, 0xdfcad000, 0xefcae000, 0xffcaf000
};

void BlitBob(WORD x, WORD y, ULONG admod, UWORD bltsize, 
             UBYTE **restore_ptrs, UBYTE *source, UBYTE *mask, UBYTE *dest,
             UBYTE *fg_buf3, UWORD fg_offset, UWORD fg_mod)
{
     
    WORD original_x = x;
    ULONG y_offset = y * fg_mod;
    
    // Calculate X shift (0-15 pixels)
    WORD x_shift = x & 15;
    ULONG bltcon = bl_bob_table[x_shift];
    
    // Calculate byte offset
    WORD x_byte_offset = x >> 3;  // Divide by 8 for byte offset
    ULONG total_offset = y_offset + x_byte_offset;
    
    // Calculate restore pointer 1 (background buffer)
    UBYTE *restore_ptr1 = fg_buf3 + fg_offset + total_offset;
    restore_ptrs[0] = restore_ptr1;
    
    // Calculate restore pointer 2 (destination)
    UBYTE *restore_ptr2 = dest + total_offset;
    restore_ptrs[1] = restore_ptr2;
    
    // Wait for blitter and set up blit operation
    HardWaitBlit();
    
    custom->bltcon0 = (UWORD)(bltcon >> 16);
    custom->bltcon1 = (UWORD)(bltcon & 0xFFFF);
    
    // Set modulos
    custom->bltamod = (UWORD)(admod & 0xFFFF);        // A and B modulo
    custom->bltbmod = (UWORD)(admod & 0xFFFF);
    custom->bltcmod = (UWORD)(admod >> 16);           // C and D modulo  
    custom->bltdmod = (UWORD)(admod >> 16);
    
    // Set pointers
    custom->bltapt = mask;           // A = mask
    custom->bltbpt = source;         // B = source
    custom->bltcpt = restore_ptr2;   // C = destination (for cookie-cut)
    custom->bltdpt = restore_ptr2;   // D = destination
    
    // Start the blit
    custom->bltsize = bltsize;
}
