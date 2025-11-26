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
APTR zippy_logo_restore_ptrs[4];
APTR *zippy_logo_restore_ptr;

static UWORD title_frames = 0;
static UWORD title_flash_counter = 0;

void Title_LoadSprites()
{
    zippy_logo.data = Disk_AllocAndLoadAsset(ZIPPY_LOGO_FILE, MEMF_CHIP);
}

void Title_Initialize(void)
{
    zippy_logo.bob = BitMapEx_Create(BLOCKSDEPTH, ZIPPY_LOGO_WIDTH, ZIPPY_LOGO_HEIGHT);
    zippy_logo.background = Mem_AllocChip((ZIPPY_LOGO_WIDTH/8) * ZIPPY_LOGO_HEIGHT * 4);
    zippy_logo.background2 = Mem_AllocChip((ZIPPY_LOGO_WIDTH/8) * ZIPPY_LOGO_HEIGHT * 4);
    zippy_logo.visible = TRUE;
 
    zippy_logo.off_screen = FALSE;

    Title_LoadSprites();
    Title_Reset();
}

void Title_Draw()
{
    if (title_frames == 0)
    {
        Copper_SetPalette(0, 0x000);
        BlitClearScreen(draw_buffer, 320 << 6 | 16);
        BlitClearScreen(display_buffer, 320 << 6 | 16);
        HUD_DrawAll();
    }
 
    Title_RestoreLogo();
    
    // Save background at new position
    Title_SaveBackground();
    
    // Draw logo at new position
    Title_BlitLogo();

    if (title_flash_counter % 5 == 0) 
    {
        Copper_SwapColors(9, 13);
    }

    title_frames++;
    title_flash_counter++;
}

void Title_Update()
{
    if (title_frames % 6 == 0)
    {
        zippy_logo.y++;
    }

    if (zippy_logo.y >= 26)
    {
        zippy_logo.y = 26;
    }
}

void Title_Reset()
{
    zippy_logo.x = 64;
    zippy_logo.y = -32;
    zippy_logo.old_x = 64;   // Same as starting x
    zippy_logo.old_y = -32;  // Same as starting y
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
 
    // Destination
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(160, x, y, admod, bltsize, source, dest);
}

void Title_SaveBackground()
{
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;

    APTR background = (current_buffer == 0) ? zippy_logo.background : zippy_logo.background2;
    
    UWORD source_mod = (320 - 80) / 8;   // Screen modulo: 30 bytes
    UWORD dest_mod = 0;                  // Background is tight-packed
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    UBYTE *source = draw_buffer;
    UBYTE *dest = background;
    
    BlitBobSimpleSave(160, x, y, admod, bltsize, source, dest);
}

void Title_RestoreLogo()
{
    // Restore at OLD position
    WORD old_x = zippy_logo.old_x;
    WORD old_y = zippy_logo.old_y;

    APTR background = (current_buffer == 0) ? zippy_logo.background : zippy_logo.background2;
    
    // Blit from background buffer to screen
    UWORD source_mod = 0;                  // Background: tight-packed
    UWORD dest_mod = (320 - 80) / 8;      // Screen modulo: 30 bytes
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | ZIPPY_LOGO_WIDTH/16;
    
    UBYTE *source = background;             // Source at 0,0 in buffer
    UBYTE *dest = draw_buffer;              // Dest will be offset by function
    
    BlitBobSimple(160, old_x, old_y, admod, bltsize, source, dest);
    
    // Save current position for next restore
    zippy_logo.old_x = zippy_logo.x;
    zippy_logo.old_y = zippy_logo.y;

}