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
    UWORD depth;        // Number of bitplanes
    UWORD width;        // Width in pixels  
    UWORD height;       // Height in pixels
    UBYTE **planes;     // Array of pointers to bitplanes
} BitMapEx;

BitMapEx* BitMapEx_Create(UBYTE depth, UWORD width, UWORD height);
void BitMapEx_Destroy(BitMapEx *bitmap);
