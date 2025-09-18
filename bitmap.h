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
 
// Alternative version using a more flexible approach with dynamic plane array
typedef struct BitMapEx 
{
    UWORD depth;            // Number of bitplanes
    UWORD width;            // Width in pixels  
    UWORD height;           // Height in pixels
    UWORD bytes_per_row;
    UBYTE **planes;         // Array of pointers to bitplanes
} BitMapEx;

void        BitMapEx_Destroy(BitMapEx *bitmap);
BitMapEx*   BitMapEx_Create(UBYTE depth, UWORD width, UWORD height);
UWORD       BitmapEx_GetByteWidth(BitMapEx *bitmap);