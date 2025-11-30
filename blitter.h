 
UBYTE* Bob_CreateMask(unsigned char* bob,int bitplanes, int frames, int rows, int words);
void BlitBob(WORD x, WORD y, ULONG admod, UWORD bltsize, 
             UBYTE **restore_ptrs, UBYTE *source, UBYTE *mask, UBYTE *dest,
             UBYTE *fg_buf3, UWORD fg_offset, UWORD fg_mod);
void BlitBobSave(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
    APTR restore_buf, APTR *restore_ptrs, APTR src, APTR mask, APTR dest);
void BlitRestoreBobs(ULONG admod, UWORD bltsize, WORD count, APTR *restore_array);
void BlitClearScreen(APTR buffer, UWORD bltsize);
void BlitBob2(UWORD y_modulo, WORD x, WORD y, ULONG admod, UWORD bltsize,
             APTR *restore_ptrs, APTR src, APTR mask, APTR dest);
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
    BitMapEx *bob;
    UBYTE *data;      
    UBYTE *background;      // Saved background data  
    UBYTE *background2;     // Saved background data   (for DblBuf)
    UBYTE *mask;            // Add this
    WORD x, y;              // Current screen position
    WORD old_x, old_y;      // Previous position for restore
    BOOL visible;           // Is BOB active
    BOOL moved;             // Position changed this frame
    BOOL needs_restore;     // Only restore if we drew last frame
    BOOL off_screen;        // Skip blits entirely if off-screen
    ULONG size;
    RestorePtrs restore;
} BlitterObject;