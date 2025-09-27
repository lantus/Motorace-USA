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
#include "motorbike.h"
#include "hud.h"

extern volatile struct Custom *custom;
 

UBYTE game_stage = STAGE_LASANGELES;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDEDCC;
ULONG game_score = 0;
UBYTE game_rank = 99;

WORD mapposy,videoposy;
LONG mapwidth,mapheight;

UBYTE *frontbuffer,*blocksbuffer;
UWORD *mapdata;

struct BitMapEx *BlocksBitmap,*ScreenBitmap;

static void ScrollUp(void);

// One time startup for everything

void Game_Initialize()
{
    Sprites_Initialize();
    
    // Motorbike
    LoadMotorbikeSprites();
    
    // HUD
    HUD_InitSprites();
    HUD_SetSpritePositions();
    
    Game_SetBackGroundColor(0x125);
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASANGELES;
    game_state = GAME_START;
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

void Game_Loop()
{

}

void Game_SetBackGroundColor(UWORD color)
{
    Copper_SetPalette(0, color);
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

__attribute__((always_inline)) inline static void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy)
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
	custom->bltdpt	= frontbuffer + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH / 16);
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
        
        DrawBlock(x, y, mapx, mapy);
        DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, mapx, mapy);
    }
   	
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
			DrawBlock(x, y, a, start_tile_y + b);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b);
		}
	} 
 
}



