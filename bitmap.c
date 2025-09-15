#include <exec/memory.h>
#include <proto/exec.h>
#include "bitmap.h"
 
 
BitMapEx* BitMapEx_Create(UBYTE depth, UWORD width, UWORD height)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    BitMapEx *bitmap;
    ULONG plane_size;
    UWORD bytes_per_row;
    int i;
    
    // Allocate memory for the bitmap structure
    bitmap = (BitMapEx*)AllocMem(sizeof(BitMapEx), MEMF_FAST);
    if (!bitmap) {
        return NULL;
    }
    
    // Allocate array for plane pointers
    bitmap->planes = (UBYTE**)AllocMem(depth * sizeof(UBYTE*), MEMF_FAST);
    if (!bitmap->planes) {
        FreeMem(bitmap, sizeof(BitMapEx));
        return NULL;
    }
    
    // Initialize bitmap structure
    bitmap->depth = depth;
    bitmap->width = width;
    bitmap->height = height;
    
    // Calculate plane size
    bytes_per_row = width >> 3;
    plane_size = bytes_per_row * height;
    
    // Allocate memory for each bitplane
    for (i = 0; i < depth; i++) {
        bitmap->planes[i] = (UBYTE*)AllocMem(plane_size, MEMF_CHIP | MEMF_CLEAR);
        if (!bitmap->planes[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                FreeMem(bitmap->planes[j], plane_size);
            }
            FreeMem(bitmap->planes, depth * sizeof(UBYTE*));
            FreeMem(bitmap, sizeof(BitMapEx));
            return NULL;
        }
    }
    
    return bitmap;
}

void BitMapEx_Destroy(BitMapEx *bitmap)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    ULONG plane_size;
    UWORD bytes_per_row;
    int i;
    
    if (!bitmap) {
        return;
    }
    
    if (bitmap->planes) {
        bytes_per_row = bitmap->width >> 3;
        plane_size = bytes_per_row * bitmap->height;
        
        for (i = 0; i < bitmap->depth; i++) {
            if (bitmap->planes[i]) {
                FreeMem(bitmap->planes[i], plane_size);
            }
        }
        
        FreeMem(bitmap->planes, bitmap->depth * sizeof(UBYTE*));
    }
    
    FreeMem(bitmap, sizeof(BitMapEx));
}