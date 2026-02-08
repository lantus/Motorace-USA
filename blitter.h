 
#ifndef BLITTER_H
#define BLITTER_H

UBYTE* Bob_CreateMask(unsigned char* bob,int bitplanes, int frames, int rows, int words);
void BlitBob(WORD x, WORD y, ULONG admod, UWORD bltsize, 
             UBYTE **restore_ptrs, UBYTE *source, UBYTE *mask, UBYTE *dest,
             UBYTE *fg_buf3, UWORD fg_offset, UWORD fg_mod);
void BlitBobSave(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
    APTR restore_buf, APTR *restore_ptrs, APTR src, APTR mask, APTR dest);
void BlitRestoreBobs(ULONG admod, UWORD bltsize, WORD count, APTR *restore_array);
void BlitClearScreen(APTR buffer, UWORD bltsize);
void BlitClearScreenWithColor(APTR buffer, UWORD bltsize, UBYTE color);
void BlitBob2(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
             UWORD width_pixels, APTR *restore_ptrs, APTR src, APTR mask, APTR dest);
void BlitClearBob(UBYTE *restore_ptr, UBYTE *screen_ptr, UWORD modulo, UWORD bltsize);

// A->D stuff
void BlitBobSimple(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                    APTR src, APTR dest);
void BlitBobSimpleSave(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
                       APTR src, APTR dest);
typedef struct {
    UBYTE *background_ptr;  // Pointer to saved background
    UBYTE *screen_ptr;      // Pointer to screen location
} RestorePtrs;

typedef struct 
{
    UBYTE id;
    UBYTE *data;
    UBYTE *data_frame2;     // Second animation frame
    WORD anim_frame;        // Current frame (0 or 1)
    WORD anim_counter;      // Counter for frame switching      
    UBYTE *background;      // Saved background data  
    UBYTE *background2;     // Saved background data   (for DblBuf)
    UBYTE *mask;            // Add this
    WORD x, y;              // Current screen position
    WORD old_x;             // Position 1 frame ago
    WORD old_y;
    WORD prev_old_x;        // Position 2 frames ago (what's in the back buffer)
    WORD prev_old_y;
    BOOL visible;           // Is BOB active
    BOOL moved;             // Position changed this frame
    BOOL needs_restore;     // Only restore if we drew last frame
    BOOL off_screen;        // Skip blits entirely if off-screen
    ULONG size;
    WORD speed;             // Random max speed ~160
    WORD accumulator;       // Fixed-point accumulator for smooth movement
    WORD target_speed;
    WORD saved_buffer_y;    // Track where background was saved
    BOOL crashed;
    UBYTE crash_timer;
    BOOL has_blocked_bike;   // TRUE if this car has already moved to block bike
    WORD block_timer;        // Timer for gradual movement   
    RestorePtrs restore;

} BlitterObject;

#endif