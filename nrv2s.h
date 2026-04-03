#ifndef NRV2S_H
#define NRV2S_H

#include <exec/types.h>

/*
 * NRV2S decompressor — 68000 assembly by ross
 * C inline asm wrapper matching ptplayer calling convention
 *
 * Compress assets on PC with matching nrv2s packer.
 * Decompress on Amiga with: NRV2S_Unpack(compressed, destination)
 *
 * src and dst must NOT overlap.
 */

static inline void NRV2S_Unpack(const void *src, void *dst)
{
    register volatile void *_a0 ASM("a0") = (void *)src;
    register volatile void *_a1 ASM("a1") = (void *)dst;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _nrv2s_unpack\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a0), "+rf"(_a1)
    :
    : "cc", "memory");
}

#endif