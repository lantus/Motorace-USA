
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