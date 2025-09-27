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
#include "hardware.h"
#include "bitmap.h"
#include "blitter.h"

extern volatile struct Custom *custom;

void Blitter_Copy(const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,BitMapEx *pDst, WORD wDstX, WORD wDstY, WORD wWidth, WORD wHeight,UBYTE ubMinterm)
{
	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulSrcOffs, ulDstOffs;
	UBYTE ubShift, ubSrcDelta, ubDstDelta, ubWidthDelta, ubMaskFShift, ubMaskLShift;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;
	ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;
	UBYTE isBlitInterleaved = 1;
	 

	if(ubSrcDelta > ubDstDelta || ((wWidth+ubDstDelta+15) & 0xFFF0)-(wWidth+ubSrcDelta) > 16) {
		uwBlitWidth = (wWidth+(ubSrcDelta>ubDstDelta?ubSrcDelta:ubDstDelta)+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ((ubWidthDelta+15)&0xF0)-ubWidthDelta;
		ubMaskLShift = uwBlitWidth - (wWidth+ubMaskFShift);
		uwFirstMask = 0xFFFF << ubMaskFShift;
		uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) { // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift-16);
		}

		ubShift = uwBlitWidth - (ubDstDelta+wWidth+ubMaskFShift);
		uwBltCon1 = (ubShift << BSHIFTSHIFT) | BLITREVERSE;

		// Position on the end of last row of the bitmap.
		// For interleaved, position on the last row of last bitplane.
		if(isBlitInterleaved) 
        {
			// TODO: fix duplicating bitmapIsInterleaved() check inside bitmapGetByteWidth()
			ulSrcOffs = pSrc->bytes_per_row * (wSrcY + wHeight) - BitmapEx_GetByteWidth(pSrc) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->bytes_per_row * (wDstY + wHeight) - BitmapEx_GetByteWidth(pDst) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
		else {
			ulSrcOffs = pSrc->bytes_per_row * (wSrcY + wHeight - 1) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->bytes_per_row * (wDstY + wHeight - 1) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
	}
	else 
    {
		uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ubSrcDelta;
		ubMaskLShift = uwBlitWidth-(wWidth+ubSrcDelta);

		uwFirstMask = 0xFFFF >> ubMaskFShift;
		uwLastMask = 0xFFFF << ubMaskLShift;

		ubShift = ubDstDelta-ubSrcDelta;
		uwBltCon1 = ubShift << BSHIFTSHIFT;

		ulSrcOffs = pSrc->bytes_per_row * wSrcY + (wSrcX >> 3);
		ulDstOffs = pDst->bytes_per_row * wDstY + (wDstX >> 3);
	}

	uwBltCon0 = (ubShift << ASHIFTSHIFT) | USEB|USEC|USED | ubMinterm;

	if(isBlitInterleaved) 
    {
		wHeight *= pSrc->depth;
		wSrcModulo = BitmapEx_GetByteWidth(pSrc) - uwBlitWords * 2;
		wDstModulo = BitmapEx_GetByteWidth(pDst) - uwBlitWords * 2;

		HardWaitBlit(); // Don't modify registers when other blit is in progress
		custom->bltcon0 = uwBltCon0;
		custom->bltcon1 = uwBltCon1;
		custom->bltafwm = uwFirstMask;
		custom->bltalwm = uwLastMask;
		custom->bltbmod = wSrcModulo;
		custom->bltcmod = wDstModulo;
		custom->bltdmod = wDstModulo;
		custom->bltadat = 0xFFFF;
		custom->bltbpt = &pSrc->planes[0][ulSrcOffs];
		custom->bltcpt = &pDst->planes[0][ulDstOffs];
		custom->bltdpt = &pDst->planes[0][ulDstOffs];
#if defined(USE_ECS_FEATURES)
		custom->bltsizv = wHeight;
		custom->bltsizh = uwBlitWords;
#else
		custom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
	}
	else
    {
		wSrcModulo = pSrc->bytes_per_row - uwBlitWords * 2;
		wDstModulo = pDst->bytes_per_row - uwBlitWords * 2;
		UBYTE ubPlane = MIN(pSrc->depth, pDst->depth);

		HardWaitBlit(); // Don't modify registers when other blit is in progress
		custom->bltcon0 = uwBltCon0;
		custom->bltcon1 = uwBltCon1;
		custom->bltafwm = uwFirstMask;
		custom->bltalwm = uwLastMask;
		custom->bltbmod = wSrcModulo;
		custom->bltcmod = wDstModulo;
		custom->bltdmod = wDstModulo;
		custom->bltadat = 0xFFFF;
		while(ubPlane--) 
        {
			HardWaitBlit();
			// This hell of a casting must stay here or else large offsets get bugged!
			custom->bltbpt = &pSrc->planes[ubPlane][ulSrcOffs];
			custom->bltcpt = &pDst->planes[ubPlane][ulDstOffs];
			custom->bltdpt = &pDst->planes[ubPlane][ulDstOffs];

#if defined(USE_ECS_FEATURES)
			custom->bltsizv = wHeight;
			custom->bltsizh = uwBlitWords;
#else
			custom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		}
	}

	 
}

void Blitter_CopyMask(const BitMapEx *pSrc, WORD wSrcX, WORD wSrcY,BitMapEx *pDst, WORD wDstX, WORD wDstY,WORD wWidth, WORD wHeight, const UBYTE *pMsk)
{
	// Helper vars
	UWORD uwBlitWords, uwBlitWidth;
	ULONG ulSrcOffs, ulDstOffs;
	UBYTE ubShift, ubSrcDelta, ubDstDelta, ubWidthDelta, ubMaskFShift, ubMaskLShift;
	// Blitter register values
	UWORD uwBltCon0, uwBltCon1, uwFirstMask, uwLastMask;
	WORD wSrcModulo, wDstModulo;

	ubSrcDelta = wSrcX & 0xF;
	ubDstDelta = wDstX & 0xF;
	ubWidthDelta = (ubSrcDelta + wWidth) & 0xF;
	UBYTE isBlitInterleaved = 1;
	

	if(ubSrcDelta > ubDstDelta || ((wWidth+ubDstDelta+15) & 0xFFF0)-(wWidth+ubSrcDelta) > 16) {
		uwBlitWidth = (wWidth+(ubSrcDelta>ubDstDelta?ubSrcDelta:ubDstDelta)+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ((ubWidthDelta+15)&0xF0)-ubWidthDelta;
		ubMaskLShift = uwBlitWidth - (wWidth+ubMaskFShift);

		// Position on the end of last row of the bitmap.
		// For interleaved, position on the last row of last bitplane.
		if(isBlitInterleaved) 
        {
			// TODO: fix duplicating bitmapIsInterleaved() check inside bitmapGetByteWidth()
			ulSrcOffs = pSrc->bytes_per_row * (wSrcY + wHeight) - BitmapEx_GetByteWidth(pSrc) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->bytes_per_row * (wDstY + wHeight) - BitmapEx_GetByteWidth(pDst) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}
		else {
			ulSrcOffs = pSrc->bytes_per_row * (wSrcY + wHeight - 1) + ((wSrcX + wWidth + ubMaskFShift - 1) / 16) * 2;
			ulDstOffs = pDst->bytes_per_row * (wDstY + wHeight - 1) + ((wDstX + wWidth + ubMaskFShift - 1) / 16) * 2;
		}

		uwFirstMask = 0xFFFF << ubMaskFShift;
		uwLastMask = 0xFFFF >> ubMaskLShift;
		if(ubMaskLShift > 16) { // Fix for 2-word blits
			uwFirstMask &= 0xFFFF >> (ubMaskLShift-16);
		}

		ubShift = uwBlitWidth - (ubDstDelta+wWidth+ubMaskFShift);
		uwBltCon1 = (ubShift << BSHIFTSHIFT) | BLITREVERSE;
	}
	else {
		uwBlitWidth = (wWidth+ubDstDelta+15) & 0xFFF0;
		uwBlitWords = uwBlitWidth >> 4;

		ubMaskFShift = ubSrcDelta;
		ubMaskLShift = uwBlitWidth-(wWidth+ubSrcDelta);

		uwFirstMask = 0xFFFF >> ubMaskFShift;
		uwLastMask = 0xFFFF << ubMaskLShift;

		ubShift = ubDstDelta-ubSrcDelta;
		uwBltCon1 = ubShift << BSHIFTSHIFT;

		ulSrcOffs = pSrc->bytes_per_row * wSrcY + (wSrcX >> 3);
		ulDstOffs = pDst->bytes_per_row * wDstY + (wDstX >> 3);
	}

	 uwBltCon0 = uwBltCon1 |USEA|USEB|USEC|USED|MINTERM_COOKIE;

	if(isBlitInterleaved) 
	{
		wHeight *= pSrc->depth;
		wSrcModulo = BitmapEx_GetByteWidth(pSrc) - uwBlitWords * 2;
		wDstModulo = BitmapEx_GetByteWidth(pDst) - uwBlitWords * 2;

		HardWaitBlit(); // Don't modify registers when other blit is in progress
		custom->bltcon0 = uwBltCon0;
		custom->bltcon1 = uwBltCon1;
		custom->bltafwm = uwFirstMask;
		custom->bltalwm = uwLastMask;
		custom->bltamod = -4;
		custom->bltbmod = wSrcModulo;
		custom->bltcmod = wDstModulo;
		custom->bltdmod = wDstModulo;
		custom->bltapt = (APTR)&pMsk[ulSrcOffs];
		custom->bltbpt = &pSrc->planes[0][ulSrcOffs];
		custom->bltcpt = &pDst->planes[0][ulDstOffs];
		custom->bltdpt = &pDst->planes[0][ulDstOffs];
#if defined(ACE_USE_ECS_FEATURES)
		custom->bltsizv = wHeight;
		custom->bltsizh = uwBlitWords;
#else
		custom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
	}
	else 
	{
		wSrcModulo = pSrc->bytes_per_row - uwBlitWords * 2;
		wDstModulo = pDst->bytes_per_row - uwBlitWords * 2;
		UBYTE ubPlane = MIN(pSrc->depth, pDst->depth);

		HardWaitBlit(); // Don't modify registers when other blit is in progress
		custom->bltcon0 = uwBltCon0;
		custom->bltcon1 = uwBltCon1;
		custom->bltafwm = uwFirstMask;
		custom->bltalwm = uwLastMask;
		custom->bltapt = (APTR)&pMsk[ulSrcOffs];
		custom->bltamod = wSrcModulo;
		custom->bltbmod = wSrcModulo;
		custom->bltcmod = wDstModulo;
		custom->bltdmod = wDstModulo;

		while(ubPlane--) {
			HardWaitBlit();
			// This hell of a casting must stay here or else large offsets get bugged!
			custom->bltapt = (APTR)&pMsk[ulSrcOffs];
			custom->bltbpt = &pSrc->planes[ubPlane][ulSrcOffs];
			custom->bltcpt = &pDst->planes[ubPlane][ulDstOffs];
			custom->bltdpt = &pDst->planes[ubPlane][ulDstOffs];

#if defined(ACE_USE_ECS_FEATURES)
			custom->bltsizv = wHeight;
			custom->bltsizh = uwBlitWords;
#else
			custom->bltsize = (wHeight << HSIZEBITS) | uwBlitWords;
#endif
		}
	}
 
}