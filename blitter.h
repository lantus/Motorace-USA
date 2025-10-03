// BltCon0 channel enable bits
#define USEA 0x800
#define USEB 0x400
#define USEC 0x200
#define USED 0x100

// Minterm presets - OR unfriendly!
#define MINTERM_A 0xF0
#define MINTERM_B 0xCC
#define MINTERM_C 0xAA
#define MINTERM_A_OR_C 0xFA
#define MINTERM_NA_AND_C 0x0A
#define MINTERM_COOKIE 0xCA
#define MINTERM_REVERSE_COOKIE 0xAC
#define MINTERM_COPY 0xC0

void Blitter_Copy(const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight, UBYTE ubMinterm);
void Blitter_CopyMask(const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,BitMapEx *pDst, WORD wDstX, WORD wDstY,WORD wWidth, WORD wHeight, const UBYTE *pMsk);
UBYTE* Bob_CreateMask(unsigned char* bob,int bitplanes, int frames, int rows, int words);
void BlitBob(WORD x, WORD y, ULONG admod, UWORD bltsize, 
             UBYTE **restore_ptrs, UBYTE *source, UBYTE *mask, UBYTE *dest,
             UBYTE *fg_buf3, UWORD fg_offset, UWORD fg_mod);