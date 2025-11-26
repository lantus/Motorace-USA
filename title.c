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
#include "map.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "memory.h"
#include "blitter.h"
#include "hud.h"
#include "title.h"

#define ZIPPY_LOGO_WIDTH 80
#define ZIPPY_LOGO_HEIGHT 48
 
BlitterObject zippy_logo;
/* Restore pointers */
APTR zippy_logo_restore_ptrs;
APTR *zippy_logo_restore_ptr;

static UWORD title_frames = 0;

void Title_LoadSprites()
{
    zippy_logo.data = Disk_AllocAndLoadAsset(ZIPPY_LOGO_FILE, MEMF_CHIP);
}

void Title_Initialize(void)
{
    zippy_logo.bob = BitMapEx_Create(BLOCKSDEPTH, ZIPPY_LOGO_WIDTH, ZIPPY_LOGO_HEIGHT);
    zippy_logo.background = Mem_AllocChip(ZIPPY_LOGO_WIDTH*ZIPPY_LOGO_HEIGHT);
    zippy_logo.visible = TRUE;
    zippy_logo.x = 32;
    zippy_logo.y = 160;
    zippy_logo.off_screen = FALSE;

    Title_LoadSprites();
    Title_Reset();
}

void Title_Draw()
{
    if (title_frames == 0)
    {
        // Set everything up on the first frame

        Copper_SetPalette(0, 0x000);

        BlitClearScreen(draw_buffer, 320 << 6 | 16 );
        BlitClearScreen(display_buffer, 320 << 6 | 16 );

        HUD_DrawAll();
    }

    Title_BlitLogo();

    title_frames++;
}

void Title_Update()
{
     zippy_logo.y++;

     if (zippy_logo.y >= 32)
     {
        zippy_logo.y = 32;
     }
}

void Title_Reset()
{
    zippy_logo.x = 64;
    zippy_logo.y = -32;

    title_frames = 0;
 
}

void Title_BlitLogo()
{
    // Position
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;
 
    UWORD source_mod = 0; 
    UWORD dest_mod =  (320 - 80) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    // Source data
    UBYTE *source = (UBYTE*)&zippy_logo.data[0];
    
    // Mask data
    //UBYTE *mask = source + 32 / 8  * 1;
    
    // Destination
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(160, x, y, admod, bltsize, source, dest);
}