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
#include "roadsystem.h"

#define ZIPPY_LOGO_WIDTH 80
#define ZIPPY_LOGO_HEIGHT 48

#define ATTRACTMODE_TILES_WIDTH 256
#define ATTRACTMODE_TILES_HEIGHT 512
#define NUM_ROAD_FRAMES 14
 
extern volatile struct Custom *custom;

BlitterObject zippy_logo;
//BlitterObject city_attract;
/* Restore pointers */
APTR zippy_logo_restore_ptrs[4];
APTR *zippy_logo_restore_ptr;

static UWORD title_frames = 0;
static UWORD title_flash_counter = 0;
static BYTE road_tile_idx = 0;
RawMap *city_attract_map;
BitMapEx *city_attract_tiles;
 
void Title_LoadSprites()
{
    zippy_logo.data = Disk_AllocAndLoadAsset(ZIPPY_LOGO, MEMF_CHIP);
    //city_attract.data = Disk_AllocAndLoadAsset(CITY_ATTRACT,MEMF_CHIP);
}

void Title_Initialize(void)
{
    // Load Zippy Race Logo Bob
    zippy_logo.background = Mem_AllocChip((ZIPPY_LOGO_WIDTH/8) * ZIPPY_LOGO_HEIGHT * 4);
    zippy_logo.background2 = zippy_logo.background ;
    zippy_logo.visible = TRUE;
    zippy_logo.off_screen = FALSE;
    Title_LoadSprites();

    // Attract Mode Tiles/Tilemap
    Title_OpenMap();
    Title_OpenBlocks();

    // Palette
    Game_LoadPalette("palettes/intro.pal", intro_colors, BLOCKSCOLORS);
    Game_ApplyPalette(intro_colors,BLOCKSCOLORS);

    // Reset Positions
    Title_Reset();
}

void Title_Draw()
{
    
    if (title_frames == 0)
    {
        Copper_SetPalette(0, 0x000);

        BlitClearScreen(draw_buffer, 320 << 6 | 16);
        BlitClearScreen(display_buffer, 320 << 6 | 16);

        // So we pre-draw the city skyline ones
        // then on update we only draw the tiles being replaced
      
        Title_RestoreLogo();
        
        Title_PreDrawSkyline();

        // Draw the hud
        HUD_DrawAll();

    }
    
    // Save background at new position
    //Title_SaveBackground();

    Title_DrawSkyline(TRUE);
 
    // Draw logo at new position
    //Title_BlitLogo();

    if (title_flash_counter % 5 == 0) 
    {          
        Copper_SwapColors(7, 12);
    }

    title_frames++;
    title_flash_counter++;
}

void Title_Update()
{
    if (title_frames % 4 == 0)
    {
        zippy_logo.y++;
    }

    if (zippy_logo.y >= 26)
    {
        zippy_logo.y = 26;
    }

    if (JoyLeft())
    {
        road_tile_idx--;
    }

    if (JoyRight())
    {
        road_tile_idx++;
       
    }

    if (road_tile_idx > NUM_ROAD_FRAMES)
    {
        road_tile_idx = 0;
    }
    else if (road_tile_idx < 0)
    {
        road_tile_idx = NUM_ROAD_FRAMES;
    }
}

void Title_Reset()
{
    zippy_logo.x = 64;
    zippy_logo.y = -32;
    zippy_logo.old_x = 64;   // Same as starting x
    zippy_logo.old_y = -32;  // Same as starting y
    road_tile_idx = 0;
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

void Title_OpenMap(void)
{
   
	city_attract_map = Disk_AllocAndLoadAsset("maps/city_attract.map",MEMF_PUBLIC);
	
	mapdata = (UWORD *)city_attract_map->data;
	mapwidth = city_attract_map->mapwidth;
	mapheight = city_attract_map->mapheight;   
}
 
void Title_OpenBlocks(void)
{
	city_attract_tiles = BitMapEx_Create(BLOCKSDEPTH, ATTRACTMODE_TILES_WIDTH, ATTRACTMODE_TILES_HEIGHT+64);
	Disk_LoadAsset((UBYTE *)city_attract_tiles->planes[0],"tiles/city_attract.BPL");  
}
 
void DrawBlockRun(LONG x, LONG y, UWORD block, WORD count, UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
{
    x = (x / 8) & 0xFFFE;
    y = y * BITMAPBYTESPERROW;
    
    UWORD mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
    UWORD mapy = (block / blocksperrow) * (blockplanelines * blockbytesperrow);
    
    HardWaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytesperrow - (BLOCKWIDTH / 8);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH / 8) * count;  // Skip over tiles we're not writing
    custom->bltapt  = blocksbuffer + mapy + mapx;
    custom->bltdpt  = dest + y + x;
    
    // Blit width is count * BLOCKWIDTH
    custom->bltsize = blockplanelines * 64 + (BLOCKWIDTH / 16) * count;
}

void Title_PreDrawSkyline()
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;
    WORD a, b, x, y;
    
    // Enable DMA ONCE at start
    custom->dmacon = 0x8400;
    
    for (b = 0; b < 11; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * blockplanelines;
            DrawBlocks(x, y, a, b, blocksperrow, blockbytesperrow, 
                      blockplanelines, FALSE, road_tile_idx, draw_buffer); 
        }
    }
    
    // Disable DMA ONCE at end
    custom->dmacon = 0x0400;
}

void Title_DrawSkyline(BOOL deltas_only)
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;

    WORD a, b, x, y;
    
    custom->dmacon = 0x8400;
    
    for (b = 0; b < 11; b++)
    {
        a = 0;
        while (a < 12)
        {
            UWORD block = mapdata[b * mapwidth + a];
            WORD run_length = 1;

            if (block >= 32)
            {
                block += road_tile_idx << 5;

                // Count how many consecutive tiles are the same
          
                while (a + run_length < 12)
                {
                    UWORD next_block = mapdata[b * mapwidth + a + run_length];
                    next_block += road_tile_idx << 5;
                    if (next_block != block) break;
                    run_length++;
                }
            
                x = a * BLOCKWIDTH;
                y = b * blockplanelines;
                
                // Blit 'run_length' tiles in one operation
                DrawBlockRun(x, y, block, run_length, blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
            }
            
            a += run_length;
           
        }
    }
    
    custom->dmacon = 0x0400;
 
}