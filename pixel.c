#include <exec/types.h>
#include <graphics/gfx.h>
#include "bitmap.h"
#include "pixel.h"

// BitMap structure offsets (assuming standard Amiga BitMap)
#define BitMap_Width 8
#define BitMap_Depth 5
#define BitMap_Plane1 16

/*
 * Set a pixel in a bitmap
 * IN:
 *   x = x coordinate
 *   y = y coordinate  
 *   color = color value
 *   bitmap = bitmap pointer
 */
void Pix_Set(UWORD x, UWORD y, UWORD color, struct BitMapEx *bitmap) 
{
    ULONG offset;
    UBYTE bitOffset;
    UWORD depth, colorBit;
    UBYTE *bitplane;
    ULONG *planePtr;
    
    // Calculate byte offset in bitplane
    UWORD width = bitmap->width;
    offset = (ULONG)y * width + x;
    offset >>= 3;  // Convert to byte offset
    
    // Calculate bit offset within byte
    bitOffset = 7 - (x & 7);
    
    // Get bitmap depth
    depth = bitmap->depth;
    
    // Get pointer to first bitplane pointer
    planePtr = (ULONG *)((UBYTE *)bitmap->planes[0]);
    
    // Set/clear bits in each bitplane based on color
    for (colorBit = 0; colorBit < depth; colorBit++) 
    {
        bitplane = (UBYTE *)(*planePtr);
        
        if (color & (1 << colorBit)) {
            // Set bit
            bitplane[offset] |= (1 << bitOffset);
        } else {
            // Clear bit
            bitplane[offset] &= ~(1 << bitOffset);
        }
        
        planePtr++;
    }
}

/*
 * Set pixels in a rectangular area around a center point
 * IN:
 *   x = center x coordinate
 *   y = center y coordinate
 *   color = color value
 *   radius = pixel radius (0 means just one pixel, 1 means 3x3, etc.)
 *   bitmap = bitmap pointer
 */
void Pix_SetWithRadius(UWORD x, UWORD y, UWORD color, UWORD radius, struct BitMapEx *bitmap) 
{
    UWORD minX, minY, maxX, maxY;
    UWORD currentX, currentY;
    
    // Calculate bounding rectangle
    minX = x - radius;
    minY = y - radius;
    maxX = x + radius;
    maxY = y + radius;
    
    // Draw rectangle of pixels
    for (currentX = minX; currentX <= maxX; currentX++) {
        for (currentY = minY; currentY <= maxY; currentY++) {
            Pix_Set(currentX, currentY, color, bitmap);
        }
    }
}

/*
 * Get the color of a pixel from a bitmap
 * IN:
 *   x = x coordinate
 *   y = y coordinate
 *   bitmap = bitmap pointer
 * OUT:
 *   Returns color value
 */
UWORD Pix_Get(UWORD x, UWORD y, struct BitMapEx *bitmap) 
{
    ULONG offset;
    UBYTE bitOffset;
    UWORD depth, colorBit;
    UWORD result = 0;
    UBYTE *bitplane;
    ULONG *planePtr;
    
    // Calculate byte offset in bitplane
    UWORD width = bitmap->width;
    offset = (ULONG)y * width + x;
    offset >>= 3;  // Convert to byte offset
    
    // Calculate bit offset within byte
    bitOffset = 7 - (x & 7);
    
    // Get bitmap depth
    depth = *(UWORD *)((UBYTE *)bitmap + BitMap_Depth);
    
    // Get pointer to first bitplane pointer
    planePtr = (ULONG *)((UBYTE *)bitmap + BitMap_Plane1);
    
    // Read bits from each bitplane to construct color
    for (colorBit = 0; colorBit < depth; colorBit++) {
        bitplane = (UBYTE *)(*planePtr);
        
        if (bitplane[offset] & (1 << bitOffset)) {
            result |= (1 << colorBit);
        }
        
        planePtr++;
    }
    
    return result;
}

/*
 * Alternative implementation using standard Amiga BitMap structure
 * (if you have access to the actual BitMap structure definition)
 */
#ifdef USE_STANDARD_BITMAP

void Pix_Set_Standard(UWORD x, UWORD y, UWORD color, struct BitMap *bitmap) {
    ULONG offset;
    UBYTE bitOffset;
    UWORD plane;
    
    // Calculate byte and bit offsets
    offset = (ULONG)y * (bitmap->BytesPerRow) + (x >> 3);
    bitOffset = 7 - (x & 7);
    
    // Set/clear bits in each bitplane
    for (plane = 0; plane < bitmap->Depth; plane++) {
        UBYTE *bitplane = (UBYTE *)bitmap->Planes[plane];
        
        if (color & (1 << plane)) {
            bitplane[offset] |= (1 << bitOffset);
        } else {
            bitplane[offset] &= ~(1 << bitOffset);
        }
    }
}

UWORD Pix_Get_Standard(UWORD x, UWORD y, struct BitMap *bitmap) {
    ULONG offset;
    UBYTE bitOffset;
    UWORD plane;
    UWORD result = 0;
    
    // Calculate byte and bit offsets
    offset = (ULONG)y * (bitmap->BytesPerRow) + (x >> 3);
    bitOffset = 7 - (x & 7);
    
    // Read bits from each bitplane
    for (plane = 0; plane < bitmap->Depth; plane++) {
        UBYTE *bitplane = (UBYTE *)bitmap->Planes[plane];
        
        if (bitplane[offset] & (1 << bitOffset)) {
            result |= (1 << plane);
        }
    }
    
    return result;
}

#endif /* USE_STANDARD_BITMAP */