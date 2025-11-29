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
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
#include "hud.h"
#include "title.h"
#include "hiscore.h"

extern volatile struct Custom *custom;

UBYTE game_stage = STAGE_LASANGELES;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDEDCC;
UBYTE game_map = MAP_ATTRACT_INTRO;
ULONG game_score = 0;
UBYTE game_rank = 99;

WORD mapposy,videoposy;
LONG mapwidth,mapheight;

UBYTE *frontbuffer,*blocksbuffer;
UWORD *mapdata;

struct BitMapEx *BlocksBitmap,*ScreenBitmap;

static void ScrollUp(void);

// Palettes
UWORD	intro_colors[BLOCKSCOLORS];
UWORD	city_colors[BLOCKSCOLORS];
UWORD	desert_colors[BLOCKSCOLORS];

// One time startup for everything

void Game_Initialize()
{
    Timer_Init();           // Detect PAL/NTSC and initialize
    Sprites_Initialize();
    HiScore_Initialize();
    
    // Motorbike
    LoadMotorbikeSprites();
    
    // HUD
    HUD_InitSprites();
    HUD_SetSpritePositions();

    // Title Screen (Logo, Palette, etc)
    Title_Initialize();
    
    Game_SetBackGroundColor(0x125);
   
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;

    Game_SetMap(game_map);
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASANGELES;
    game_state = GAME_START;
    game_map = MAP_OVERHEAD_LASANGELES;
    game_difficulty = difficulty;
    game_score = 0;
    bike_speed = 0;
    game_rank = 99; // Start in 99th

    // Position bike near bottom of screen
    bike_position_x = SCREENWIDTH / 2;  // Center horizontally (around pixel 96 for 192px display)
    bike_position_y = SCREENHEIGHT - 64;  // Near bottom
    bike_state = BIKE_STATE_MOVING; 

    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    videoposy = mapposy % HALFBITMAPHEIGHT;

}

void Game_Draw()
{
    switch(game_state)
    {
        case TITLE_SCREEN:
            Title_Draw();
            break;
        case GAME_START:
            Game_Draw();
            break;
    }
}

void Game_Update()
{
    switch(game_state)
    {
        case TITLE_SCREEN:
            Title_Update();
            break;
        case GAME_START:
            break;
    }
}

void Game_SetBackGroundColor(UWORD color)
{
    Copper_SetPalette(0, color);
}


void Game_SetMap(UBYTE maptype)
{
    switch (maptype)
    {
        case MAP_ATTRACT_INTRO:
            mapdata = (UWORD *)city_attract_map->data;
            mapwidth = city_attract_map->mapwidth;
            mapheight = city_attract_map->mapheight;  
            blocksbuffer = city_attract_tiles->planes[0];
            break;
        case MAP_OVERHEAD_LASANGELES:
            break;
        case MAP_APPROACH_LASANGELES:
            break;
        case MAP_OVERHEAD_LASVEGAS:
            break;
        case MAP_APPROACH_LASVEGAS:
            break;
        case MAP_OVERHEAD_HOUSTON:
            break;
        case MAP_APPROACH_HOUSTON:  
            break;
        case MAP_OVERHEAD_STLOUIS:
            break;
        case MAP_APPROACH_STLOUIS:
            break;
        case MAP_OVERHEAD_CHICAGO: 
            break;
        case MAP_APPROACH_CHICAGO:  
            break;
        case MAP_OVERHEAD_NEWYORK: 
            break;
        case MAP_APPROACH_NEWYORK: 
            break;
    }
}
 

// Convert speed to scroll calls
WORD GetScrollAmount(WORD speed)
{
    // Map 42-150 mph to 2-6 scroll calls
    // Linear interpolation: scrolls = 2 + ((speed - 42) / (150 - 42)) * 4
    if (speed <= MIN_SPEED) return 2;
    if (speed >= MAX_SPEED) return 6;
    
    // Calculate proportional scroll amount
    WORD range = speed - MIN_SPEED;
    WORD max_range = MAX_SPEED - MIN_SPEED;
    return 2 + ((range * 4) / max_range);
}

static void SmoothScroll(void)
{
    // Calculate scroll speed in fixed-point (8.8 format: 8 bits integer, 8 bits fraction)
    // 2 scrolls = 512 (2 << 8), 6 scrolls = 1536 (6 << 8)
    WORD scroll_speed = GetScrollAmount(bike_speed) << 8;
    
    // Add to accumulator
    scroll_accumulator += scroll_speed;
    
    // Extract whole pixels and scroll
    while (scroll_accumulator >= 256) 
	{
        ScrollUp();
        scroll_accumulator -= 256;
    }
}

void Game_CheckJoyScroll(void)
{
    if (JoyFire()) 
	{
        AccelerateMotorBike();
    } 
	else 
	{
		BrakeMotorBike();
	}
    
    SmoothScroll();  // Use smooth scrolling instead of loop
}

__attribute__((always_inline)) inline void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy, UBYTE *dest)
{
	 
	x = (x / 8) & 0xFFFE;
	y = y * BITMAPBYTESPERROW;
	
	UWORD block = mapdata[mapy * mapwidth + mapx];

	mapx = (block % BLOCKSPERROW) * (BLOCKWIDTH / 8);
	mapy = (block / BLOCKSPERROW) * (BLOCKPLANELINES * BLOCKSBYTESPERROW);
 
	HardWaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = BLOCKSBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH / 16);
}
 
__attribute__((always_inline)) inline void DrawBlocks(LONG x,LONG y,LONG mapx,LONG mapy, UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, BOOL deltas_only, UBYTE tile_idx, UBYTE *dest)
{
	 
	x = (x / 8) & 0xFFFE;
	y = y * BITMAPBYTESPERROW;
 

	UWORD block = mapdata[mapy * mapwidth + mapx];
  
	mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
	mapy = (block / blocksperrow) * (blockplanelines * blockbytessperrow);
 
    HardWaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = blockbytessperrow - (BLOCKWIDTH / 8);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = blockplanelines * 64 + (BLOCKWIDTH / 16);
 
}
 
__attribute__((always_inline)) inline void DrawBlockRun(LONG x, LONG y, UWORD block, WORD count, UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
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
 
static void ScrollUp(void)
{
	 WORD mapx, mapy, x, y;

    if (mapposy < 1) return;  // Stop at top of map

    mapposy--;
    videoposy = mapposy % HALFBITMAPHEIGHT;

	mapx = mapposy & (NUMSTEPS - 1);
	mapy = mapposy / BLOCKHEIGHT;
	
	y = ROUND2BLOCKHEIGHT(videoposy) * BLOCKSDEPTH;

   // Only draw if within the 12-tile display width
    if (mapx < 12) 
    {  
        // Limit to 192 pixels (12 tiles)
        x = mapx * BLOCKWIDTH;
        
        DrawBlock(x, y, mapx, mapy, screen.bitplanes);
        DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, mapx, mapy,screen.bitplanes);
        DrawBlock(x, y, mapx, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, mapx, mapy,screen.offscreen_bitplanes);
    }
   	
}

void Game_RenderBackgroundToDrawBuffer(void)
{
 
}
 
void Game_FillScreen(void)
{
	WORD a, b, x, y;
	WORD start_tile_y = mapposy / BLOCKHEIGHT;

	for (b = 0; b < HALFBITMAPBLOCKSPERCOL; b++)
	{
		for (a = 0; a < 12; a++)  // 12 tiles for 192 pixel width
		{
			x = a * BLOCKWIDTH;
			y = b * BLOCKPLANELINES;
			DrawBlock(x, y, a, start_tile_y + b, screen.bitplanes);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b,screen.bitplanes);
			DrawBlock(x, y, a, start_tile_y + b, screen.offscreen_bitplanes);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b,screen.offscreen_bitplanes);            
		}
	} 
 
}

void Game_SwapBuffers(void)
{
    // Toggle current buffer
    current_buffer ^= 1;
 
    // Update pointers
    draw_buffer = current_buffer == 0 ? screen.bitplanes : screen.offscreen_bitplanes;
    display_buffer = current_buffer == 0 ? screen.offscreen_bitplanes : screen.bitplanes;
    
    // Update copper bitplane pointers to show the new display buffer
    const USHORT lineSize = 320/8;
    const UBYTE* planes_temp[BLOCKSDEPTH];
    
    for(int a = 0; a < BLOCKSDEPTH; a++)
        planes_temp[a] = display_buffer + lineSize * a;
    
    LONG planeadd = ((LONG)(videoposy + BLOCKHEIGHT)) * BITMAPBYTESPERROW * BLOCKSDEPTH;

    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, planeadd);
}


void Game_LoadPalette(const char *filename, UWORD *palette, int num_colors)
{
    // Assuming Dpaint

    ULONG file_size = findSize(filename);
    UBYTE *file_data = Disk_AllocAndLoadAsset(filename, MEMF_ANY);
    if (!file_data) return;
    
    UBYTE *ptr = file_data;
    
    // Check for "FORM" header
    if (ptr[0] != 'F' || ptr[1] != 'O' || ptr[2] != 'R' || ptr[3] != 'M')
    {
        FreeMem(file_data, file_size);  // Free the loaded data
        return;
    }
    
    ptr += 4;  // Skip "FORM"
    
    // Skip file size (4 bytes, big-endian)
    ptr += 4;
    
    // Skip form type (usually "ILBM") - 4 bytes
    ptr += 4;
    
    // Now search for CMAP chunk
    while (1)
    {
        // Check chunk ID
        if (ptr[0] == 'C' && ptr[1] == 'M' && ptr[2] == 'A' && ptr[3] == 'P')
        {
            ptr += 4;  // Skip "CMAP"
            
            // Get chunk size (big-endian)
            ULONG chunk_size = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;
            
            // ptr now points to RGB data
            UBYTE *rgb_data = ptr;
            
            for (int i = 0; i < num_colors && i < chunk_size/3; i++)
            {
                UBYTE r = *rgb_data++;
                UBYTE g = *rgb_data++;
                UBYTE b = *rgb_data++;
                
                // Convert to Amiga 4-bit per channel
                UWORD r4 = (r >> 4) & 0x0F;
                UWORD g4 = (g >> 4) & 0x0F;
                UWORD b4 = (b >> 4) & 0x0F;
                
                palette[i] = (r4 << 8) | (g4 << 4) | b4;
            }
            
            break;
        }
        
        ptr += 4;  // Skip chunk ID
        
        // Get chunk size and skip over it
        ULONG chunk_size = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
        ptr += chunk_size;
        
        // Chunks are word-aligned, skip padding byte if odd size
        if (chunk_size & 1) ptr++;
    }
    
    FreeMem(file_data, file_size);  // Free the loaded data
}

void Game_ApplyPalette(UWORD *palette, int num_colors)
{
    for (int i = 0; i < num_colors; i++)
    {
        Copper_SetPalette(i, palette[i]);
    }
}