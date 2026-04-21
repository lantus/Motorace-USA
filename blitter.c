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

/* BLTCON0/1 lookup tables for copy operations */
static const ULONG bl_copy[16] = {
    0x09f00000, 0x19f01000, 0x29f02000, 0x39f03000,
    0x49f04000, 0x59f05000, 0x69f06000, 0x79f07000,
    0x89f08000, 0x99f09000, 0xa9f0a000, 0xb9f0b000,
    0xc9f0c000, 0xd9f0d000, 0xe9f0e000, 0xf9f0f000
};

/* BLTCON0/1 lookup tables for clear operations */
static const ULONG bl_clear[16] = {
    0x01f00000, 0x11f01000, 0x21f02000, 0x31f03000,
    0x41f04000, 0x51f05000, 0x61f06000, 0x71f07000,
    0x81f08000, 0x91f09000, 0xa1f0a000, 0xb1f0b000,
    0xc1f0c000, 0xd1f0d000, 0xe1f0e000, 0xf1f0f000
}; 

// BLTCON0/1 table for different pixel shifts (0-15)
static const ULONG bl_bob_table[16] = {
    0x0fca0000, 0x1fca1000, 0x2fca2000, 0x3fca3000,
    0x4fca4000, 0x5fca5000, 0x6fca6000, 0x7fca7000,
    0x8fca8000, 0x9fca9000, 0xafcaa000, 0xbfcab000,
    0xcfcac000, 0xdfcad000, 0xefcae000, 0xffcaf000
};

/* External references */
extern UBYTE *fg_buf32;
extern WORD fg_offset2;

/*
 * BlitBobNoRestore
 * Blits bob without updating restore pointers
 */
void BlitBobNoRestore(UWORD y_modulo, WORD x, WORD y, ULONG admod, 
                      UWORD bltsize, APTR src, APTR mask, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    WORD shift = x & 15;
    ULONG bltcon = bl_bob_table[shift];
    WORD x_offset = x >> 3;
    ULONG offset = y_offset + x_offset;
    APTR dest_ptr = (UBYTE *)dest + offset;
    
    WaitBlit();
    custom->bltcon0 = (UWORD)(bltcon >> 16);
    custom->bltcon1 = (UWORD)bltcon;
    custom->bltamod = (UWORD)(admod >> 16);
    custom->bltdmod = (UWORD)admod;
    custom->bltcmod = (UWORD)(admod >> 16);
    custom->bltbmod = (UWORD)admod;
    custom->bltapt = mask;
    custom->bltbpt = src;
    custom->bltcpt = dest_ptr;
    custom->bltdpt = dest_ptr;
    custom->bltsize = bltsize;
}

 
/*
 * BlitBobSave
 * Saves background and blits bob with cookie-cut
 */
void BlitBobSave(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                 APTR restore_buf, APTR *restore_ptrs, APTR src, APTR mask, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    WORD shift = x & 15;
        
    WORD x_offset = x >> 3;
    APTR dest_ptr = (UBYTE *)src + x_offset + y_offset;
    ULONG restore_admod = (admod & 0xFFFF0000);
 
    /* Save background */
    WaitBlit();
    custom->bltalwm = 0xFFFF;
    custom->bltcon0 = (UWORD)(bl_copy[0] >> 16);
    custom->bltcon1 = (UWORD)(bl_copy[0]);
   
    custom->bltamod = (UWORD)(admod & 0xFFFF);        // A modulo (source)
    custom->bltdmod = (UWORD)(admod >> 16);
    
 
    custom->bltapt = dest_ptr;
    custom->bltdpt = dest;
    custom->bltsize = bltsize;
 
    /* Store restore pointers (in reverse order) */
   // restore_ptrs--;
    restore_ptrs[0] = dest_ptr;
   // restore_ptrs--;
    restore_ptrs[1] = restore_buf;

}

void BlitClearScreen(APTR buffer, UWORD bltsize)
{
    WaitBlit();
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltcon0 = (UWORD)(bl_clear[0] >> 16);
    custom->bltcon1 = (UWORD)(bl_clear[0]);
    custom->bltdmod = 0;
    custom->bltadat = 0;
    custom->bltdpt = buffer;
    custom->bltsize = bltsize;
}

void BlitClearScreenWithColor(APTR buffer, UWORD bltsize, UBYTE color)
{
    // For interleaved 4 bitplanes, fill each plane based on color bits
    for (WORD plane = 0; plane < 4; plane++)
    {
        BOOL bit_set = (color >> plane) & 1;
        
        WaitBlit();
        
        if (bit_set)
        {
            // Fill plane with 1s
            custom->bltcon0 = 0xFF00;  // A->D with all 1s
            custom->bltadat = 0xFFFF;  // Source data = all 1s
        }
        else
        {
            // Fill plane with 0s
            custom->bltcon0 = 0x0100;  // D only, minterm 0 = clear
        }
        
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        
        // Offset to this plane (interleaved: skip 'plane' words)
        custom->bltdpt = buffer + (plane << 1);
        
        // Modulo: skip 3 other planes (4 planes * 2 bytes - 2)
        custom->bltdmod = 6;
        
        custom->bltsize = bltsize;
    }
}

 


void BlitRestoreBobs(UBYTE *buffer, WORD prev_x, WORD prev_y,
                       UWORD width_words, UWORD blit_lines)
{
    WORD x_byte = (prev_x >> 3) & 0xFFFE;
    WORD y4 = prev_y << 2;
    LONG offset = ((LONG)y4 << 4) + ((LONG)y4 << 3) + x_byte;

    WaitBlit();
    custom->bltcon0 = 0x09F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = BITMAPBYTESPERROW - (width_words << 1);
    custom->bltdmod = BITMAPBYTESPERROW - (width_words << 1);
    custom->bltapt  = screen.pristine + offset;
    custom->bltdpt  = buffer + offset;
    custom->bltsize = (blit_lines << 6) | width_words;
}
 

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
    WaitBlit();
    
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

void BlitClearBob(UBYTE *restore_ptr, UBYTE *screen_ptr, UWORD modulo, UWORD bltsize)
{
    if (!restore_ptr) return;
    
    WaitBlit();
    
    custom->bltcon0 = 0x09F0; // Copy A to D (simple copy)
    custom->bltcon1 = 0;
    custom->bltamod = 0;      // Source is packed (saved background)
    custom->bltdmod = modulo; // Destination has modulo (screen buffer)
    custom->bltapt = restore_ptr;  // Source: saved background
    custom->bltdpt = screen_ptr;   // Destination: screen location
    custom->bltsize = bltsize;
}
 
void BlitBob2(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
             UWORD width_pixels, APTR *restore_ptrs, APTR src, APTR mask, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    WORD shift = x & 15;
    ULONG bltcon = bl_bob_table[shift];
    
    WORD x_offset = (x >> 4) << 1;
    ULONG offset = y_offset + x_offset;
 
    APTR dest_ptr = (UBYTE *)dest + offset;
 
   
    WaitBlit();
    custom->bltcon0 = (UWORD)(bltcon >> 16);
    custom->bltcon1 = (UWORD)bltcon;
    
    custom->bltamod = (UWORD)(admod & 0xFFFF);
    custom->bltbmod = (UWORD)(admod & 0xFFFF);
    custom->bltcmod = (UWORD)(admod >> 16);
    custom->bltdmod = (UWORD)(admod >> 16);

    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;

    custom->bltapt = mask;
    custom->bltbpt = src;
    custom->bltcpt = dest_ptr;
    custom->bltdpt = dest_ptr;
    custom->bltsize = bltsize;
}

void BlitBobSimple(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                    APTR src, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    
    // Simple byte-aligned offset - no shift needed
    WORD x_offset = (x >> 4) << 1;  // Word boundary
    ULONG offset = y_offset + x_offset;
    
    APTR dest_ptr = (UBYTE *)dest + offset;
    
    WaitBlit();
    custom->bltcon0 = 0x09F0;  // Simple copy: D = A
    custom->bltcon1 = 0x0000;  // No shift
    
    custom->bltamod = (UWORD)(admod & 0xFFFF);
    custom->bltdmod = (UWORD)(admod >> 16);
    
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    
    custom->bltapt = src;
    custom->bltdpt = dest_ptr;
    custom->bltsize = bltsize;
}

void BlitBob2Clip(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                 UWORD width_pixels, APTR *restore_ptrs, APTR src, APTR mask, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    WORD shift = x & 15;
    ULONG bltcon = bl_bob_table[shift];
    
    WORD x_offset = (x >> 4) << 1;
    ULONG offset = y_offset + x_offset;
    APTR dest_ptr = (UBYTE *)dest + offset;

    /* Calculate if last blit word extends past viewport */
     WORD blit_words = bltsize & 0x3F;
    WORD height = (bltsize >> 6) & 0x3FF;
    WORD dest_first_word = x >> 4;
    WORD viewport_words = 192 >> 4;

    UWORD fwm = 0xFFFF;
    UWORD lwm = 0xFFFF;
    UWORD src_mod = (UWORD)(admod & 0xFFFF);
    UWORD dst_mod = (UWORD)(admod >> 16);

    /* Does the last blit word extend past viewport? */
    if ((dest_first_word + blit_words) > viewport_words)
    {
        WORD clipped = (dest_first_word + blit_words) - viewport_words;
        blit_words -= clipped;
        
        if (blit_words <= 0) return;
        
        /* Adjust modulos to skip clipped source words */
        src_mod += (clipped << 1);
        dst_mod += (clipped << 1);
        
        bltsize = (height << 6) | blit_words;
    }

    WaitBlit();
    custom->bltcon0 = (UWORD)(bltcon >> 16);
    custom->bltcon1 = (UWORD)bltcon;
    
    custom->bltamod = src_mod;
    custom->bltbmod = src_mod;
    custom->bltcmod = dst_mod;
    custom->bltdmod = dst_mod;

    custom->bltafwm = fwm;
    custom->bltalwm = lwm;

    custom->bltapt = mask;
    custom->bltbpt = src;
    custom->bltcpt = dest_ptr;
    custom->bltdpt = dest_ptr;
    custom->bltsize = bltsize;
}

void BlitBobSimpleSave(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                       APTR src, APTR dest)
{
    ULONG y_offset = (ULONG)y * y_modulo;
    
    // Simple byte-aligned offset - no shift needed
    WORD x_offset = (x >> 4) << 1;  // Word boundary
    ULONG offset = y_offset + x_offset;
    
    APTR src_ptr = (UBYTE *)src + offset;
    
    WaitBlit();
    custom->bltcon0 = 0x09F0;  // Simple copy: D = A
    custom->bltcon1 = 0x0000;  // No shift
    
    custom->bltamod = (UWORD)(admod & 0xFFFF);
    custom->bltdmod = (UWORD)(admod >> 16);
    
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    
    custom->bltapt = src_ptr;
    custom->bltdpt = dest;
    custom->bltsize = bltsize;
}

void BlitClearArea(APTR buffer, WORD x, WORD y, UWORD width, UWORD height)
{
    // Calculate Y offset (interleaved: y * bytes_per_row)
    ULONG y_offset = ((ULONG)y << 6) + ((ULONG)y << 5);  // y * 96 (BITMAPBYTESPERROW * 4)
    
    // Calculate X byte offset
    WORD x_byte_offset = x >> 3;
    
    // Calculate destination pointer
    UBYTE *dest = (UBYTE *)buffer + y_offset + x_byte_offset;
    
    // Calculate width in words (round up)
    UWORD width_words = (width + 15) >> 4;
    
    // Calculate modulo (bytes to skip per line)
    UWORD modulo = BITMAPBYTESPERROW - (width_words << 1);
    
    // Set up blitter for clearing
    WaitBlit();
    
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltcon0 = 0x0100;  // D only, clear
    custom->bltcon1 = 0;
    custom->bltadat = 0;       // Clear with 0s
    custom->bltdmod = modulo;
    custom->bltdpt = dest;
    
    // Blit size: (height * 4 bitplanes) << 6 | width_words
    custom->bltsize = ((height << 2) << 6) | width_words;
}