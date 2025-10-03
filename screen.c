#include "screen.h"
#include "bitplanes.h"
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

extern struct Custom custom;
 
struct AmigaScreen screen;
 
UBYTE screenbuffer_idx = 0;

void Screen_Initialize(UWORD width,
    UWORD   height,
    UBYTE   depth,
    BOOL    dual_playfield) 
{
    
    screen.framebuffer_size = (width/8)*height*depth;

    screen.bitplanes = Bitplanes_Initialize(screen.framebuffer_size);
    screen.depth = depth;
    screen.left_margin_bytes = 0;
    screen.right_margin_bytes = 0;
    screen.display_start_offset_bytes = 0;
    screen.bytes_width = width/8;
    screen.row_size = (width/8);
    screen.dual_playfield = dual_playfield;
    screen.height = height;
}

 
static UWORD getFadedValueOCS(UWORD colorvalue, USHORT frame) 
{
    short output = 0;

    short red = (colorvalue & 0x0F00);
    red *= frame;
    red >>= 4;
    red &= 0x0F00;
    output |= red;

    short green = (colorvalue & 0x00F0);
    green *= frame;
    green >>= 4;
    green &= 0x00F0;
    output |= green;

    short blue = (colorvalue & 0x000F);
    blue *= frame;
    blue >>= 4;
    blue &= 0x000F;
    output |= blue;

    return output;
}
 
void Screen_FadePalette(UWORD* rawPalette,UWORD* copPalette, USHORT frame,USHORT colors) 
{
    for (int i = 0; i < colors; i++) 
    {
        *copPalette = getFadedValueOCS(*rawPalette,frame);
        rawPalette++;
        copPalette+=2;
    }
}