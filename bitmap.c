#include <exec/memory.h>
#include <proto/exec.h>
#include "bitmap.h"
 
 
BitMapEx* BitMapEx_Create(UBYTE depth, UWORD width, UWORD height)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    BitMapEx *bitmap;
    ULONG total_size;
    UWORD bytes_per_row;
    int i;
    
    bitmap = (BitMapEx*)AllocMem(sizeof(BitMapEx), MEMF_FAST);
    if (!bitmap) {
        return NULL;
    }
    
    bitmap->planes = (UBYTE**)AllocMem(depth * sizeof(UBYTE*), MEMF_FAST);
    if (!bitmap->planes) {
        FreeMem(bitmap, sizeof(BitMapEx));
        return NULL;
    }
    
    bitmap->depth = depth;
    bitmap->width = width;
    bitmap->height = height;
    
    bytes_per_row = width >> 3;  // Bytes per plane per line
    bitmap->bytes_per_row = bytes_per_row * depth;  // Interleaved stride
    
    // Allocate ONE block for all planes (interleaved)
    total_size = bytes_per_row * depth * height;  // 40 * 4 * 256 = 40960 bytes
    
    bitmap->planes[0] = (UBYTE*)AllocMem(total_size, MEMF_CHIP | MEMF_CLEAR);
    if (!bitmap->planes[0]) {
        FreeMem(bitmap->planes, depth * sizeof(UBYTE*));
        FreeMem(bitmap, sizeof(BitMapEx));
        return NULL;
    }
    
    // Set up plane pointers for interleaved access
    for (i = 1; i < depth; i++) {
        bitmap->planes[i] = bitmap->planes[0] + (i * bytes_per_row);
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

UWORD BitmapEx_GetByteWidth(const BitMapEx *bitmap)
{
    if (BitmapEx_IsInterleaved(bitmap)) {
        return (bitmap->width / 8) * bitmap->depth;  // Interleaved stride
    } else {
        return bitmap->width / 8;  // Non-interleaved plane width
    }
}


UBYTE BitmapEx_IsInterleaved(const BitMapEx *bitmap) 
{
	 if (bitmap->depth < 2) return FALSE;
    
    // If plane pointers are close together (one line apart), it's interleaved
    ULONG spacing = (ULONG)bitmap->planes[1] - (ULONG)bitmap->planes[0];
    return (spacing == bitmap->bytes_per_row);
}