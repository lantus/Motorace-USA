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
#include "sprites.h"
#include "copper.h"
 
/* Sprite sheet dimensions */
#define SHEET_WIDTH 16
#define SHEET_HEIGHT 32
#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 32
#define SPRITES_PER_ROW 10
#define SPRITES_PER_COL 4
#define SPRITE_COLORS 16
#define TOTAL_SPRITES SPRITES_PER_ROW * SPRITES_PER_COL

#define SCREEN_DEPTH 4  // 16 colors = 4 bitplanes

#define SPRITEPALSIZE (SPRITE_COLORS * 2)


UWORD null_sprite_data[] = 
{
    0x0000, 0x0000,
    0x0000, 0x0000
};


BOOL LoadSpriteSheet(char *filename,Sprite *sheet) 
{   
    // Allocate memory for sprite sheet (4 bitplanes for 16 colors)

    int elems_read;

    BPTR sprite_handle = 0;

    if (!(sprite_handle = Open(filename,MODE_OLDFILE)))
	{
        Close(sprite_handle);
		KPrintF("ERROR: Could not open Sprite Sheet File");
        Exit(-1);
	}

    if (sprite_handle)
    {
        elems_read = Read(sprite_handle, &sheet->header, 20);
        elems_read = Read(sprite_handle, &sheet->sprite_offsets,sizeof(UWORD)*sheet->header.num_sprites);
        elems_read = Read(sprite_handle, &sheet->palette,sizeof(UWORD)*sheet->header.palette_size);
        sheet->imgdata = AllocMem(sheet->header.imgdata_size, MEMF_CHIP|MEMF_CLEAR);
        elems_read = Read(sprite_handle, sheet->imgdata, sizeof(UBYTE)*sheet->header.imgdata_size);
        Close(sprite_handle);
    }
  
    return TRUE;
}

BOOL ApplySpritePalette(Sprite *sheet)
{
    for(int i=0; i < 16; i++)
	{
		Copper_SetSpritePalette(i, ((USHORT*)sheet->palette)[i]);
	}
    
    return TRUE;
}

void InitializeSprites()
{
    // point sprites 0-7 to nothing

    CopSPR0PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR0PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR1PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR1PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR2PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR2PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR3PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR3PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR4PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR4PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR5PTH[val]= (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR5PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR6PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR6PTL[val] = ((ULONG) null_sprite_data) & 0xffff;

    CopSPR7PTH[val] = (((ULONG) null_sprite_data) >> 16) & 0xffff;
    CopSPR7PTL[val] = ((ULONG) null_sprite_data) & 0xffff;
}

void BuildCompositeSprite(ULONG *sprite, int n, Sprite *sheet)
{
    for (int i = 0; i < n; i++) 
    {
        sprite[i] = ((ULONG) sheet->imgdata) + sheet->sprite_offsets[i];
    }
}

void SetSpritePointers(ULONG *sprite, UBYTE n, UBYTE sprite_index)
{

    UWORD *wp = CopSPR0PTH;
    wp += ((4*sprite_index));
    
    for (int i = 0; i < n; i++)
    {   
        wp[val + i * 4] = (((ULONG) sprite[i]) >> 16) & 0xffff;
        wp[val + i * 4 + 2] = ((ULONG) sprite[i]) & 0xffff;
    }

}

void SetSpritePosition(UWORD *sprite_data, UWORD hstart, UWORD vstart, UWORD vstop)
{
    sprite_data[0] = ((vstart & 0xff) << 8) | ((hstart >> 1) & 0xff);
    // vstop + high bit of vstart + low bit of hstart
    sprite_data[1] = ((vstop & 0xff) << 8) |  // vstop 8 low bits
        ((vstart >> 8) & 1) << 2 |  // vstart high bit
        ((vstop >> 8) & 1) << 1 |   // vstop high bit
        (hstart & 1) |              // hstart low bit
        sprite_data[1] & 0x80;      // preserve attach bit
}

// Cleanup function

void FreeSpriteSheet(void) 
{
    // TODO: Cleanup!
}