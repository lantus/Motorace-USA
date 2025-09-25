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

UBYTE Blitter_UnsafeCopy
(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm
);

UBYTE Blitter_SafeCopy(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, UBYTE ubMinterm,
	UWORD uwLine, const char *szFile
);

UBYTE Blitter_UnsafeCopyAligned(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight
);

UBYTE Blitter_SafeCopyAligned(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight,
	UWORD uwLine, const char *szFile
);

UBYTE Blitter_UnsafeCopyMask(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk
);

UBYTE Blitter_SafeCopyMask(
	const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,
	BitMapEx *pDst, WORD wDstX, WORD wDstY,
	WORD wWidth, WORD wHeight, const UBYTE *pMsk,
	UWORD uwLine, const char *szFile
);

UBYTE Blitter_UnsafeRect(
	BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor
);

UBYTE Blitter_SafeRect(
	BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubColor, UWORD uwLine, const char *szFile
);

void Blitter_UnsafeFillAligned(
	BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode
);

UBYTE Blitter_SafeFillAligned(
	BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,
	UBYTE ubPlane, UBYTE ubFillMode, UWORD uwLine, const char *szFile
);

void Blitter_Line(
	BitMapEx *pDst, WORD x1, WORD y1, WORD x2, WORD y2,
	UBYTE ubColor, UWORD uwPattern, UBYTE isOneDot
);
 
#define Blitter_Copy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm) \
	Blitter_UnsafeCopy(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, ubMinterm)
#define Blitter_CopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight) \
	Blitter_UnsafeCopyAligned(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight)
#define Blitter_CopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk) \
	Blitter_UnsafeCopyMask(pSrc, wSrcX, wSrcY, pDst, wDstX, wDstY, wWidth, wHeight, pMsk)
#define Blitter_tRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor) \
	Blitter_UnsafeRect(pDst, wDstX, wDstY, wWidth, wHeight, ubColor)
#define Blitter_FillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode)\
	Blitter_UnsafeFillAligned(pDst, wDstX, wDstY, wWidth, wHeight, ubPlane, ubFillMode)