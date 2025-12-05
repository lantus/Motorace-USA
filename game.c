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
#include "font.h"
#include "title.h"
#include "hiscore.h"
#include "city_approach.h"

extern volatile struct Custom *custom;


UBYTE stage_state = STAGE_COUNTDOWN;
GameTimer countdown_timer;
GameTimer hud_update_timer;   
UBYTE countdown_value = 5;

UBYTE game_stage = STAGE_LASANGELES;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDREDCC;
UBYTE game_map = MAP_ATTRACT_INTRO;
ULONG game_score = 0;
UBYTE game_rank = 90;
ULONG game_frame_count = 0;

WORD mapposy,videoposy;
LONG mapwidth,mapheight;

UBYTE *frontbuffer,*blocksbuffer;
UWORD *mapdata;

struct BitMapEx *BlocksBitmap,*ScreenBitmap;

static void ScrollUp(void);

// Tiles and TileMaps

RawMap *la_map;
BitMapEx *la_tiles;

// Palettes
UWORD	intro_colors[BLOCKSCOLORS];
UWORD	city_colors[BLOCKSCOLORS];
UWORD	desert_colors[BLOCKSCOLORS];

// Used for the Countdown
static Sprite spr_countdown_timer[4];
ULONG *spr_countdown[4];
ULONG *current_countdown_spr;
UBYTE countdown_idx = 0;

// One time startup for everything

void Game_Initialize()
{
    Timer_Init();           // Detect PAL/NTSC and initialize
    Sprites_Initialize();
    HiScore_Initialize();
    
    // Motorbike
    MotorBike_Initialize();
    
    // HUD
    HUD_InitSprites();
    HUD_SetSpritePositions();

    // Title Screen (Logo, Palette, etc)
    Title_Initialize();

    // City Skyline
    City_Initialize();

    Game_SetBackGroundColor(0x125);

    // Stage Tiles and TileMaps
    Stage_Initialize();
 
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;

    Game_SetMap(game_map);

    MotorBike_Reset();
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASANGELES;
    game_state = STAGE_START;
    game_map = MAP_OVERHEAD_LASANGELES;
    game_difficulty = difficulty;
    game_score = 0;
    bike_speed = 0;
    game_rank = 99; // Start in 99th

    Game_SetMap(game_map);

    // Position bike near bottom of screen
    bike_position_x = SCREENWIDTH / 2;  // Center horizontally (around pixel 96 for 192px display)
    bike_position_y = SCREENHEIGHT - 64;  // Near bottom
    bike_state = BIKE_STATE_STOPPED; 
    
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
        case GAME_READY:
            GameReady_Draw();
            break;    
        case STAGE_START:
            Stage_Draw();
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
        case GAME_READY:
            GameReady_Update();
            break;   
        case STAGE_START:
            Stage_Update();
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
            mapdata = (UWORD *)la_map->data;
            mapwidth = la_map->mapwidth;
            mapheight = la_map->mapheight;  
            blocksbuffer = la_tiles->planes[0];
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
    if (speed == 0)
    {
        return 0;
    }
    else if (speed <= 10)
    {
        // Very slow start (0 to 0.5 pixels/frame)
        return speed / 20;  // Returns 0-0.5
    }
    else if (speed <= 21)
    {
        // Picking up speed (0.5 to 1 pixel/frame)
        return ((speed - 10) / 11) + 0;  // Returns 0.5-1
    }
    else if (speed <= 42)
    {
        // Reaching cruising speed (1 to 2 pixels/frame)
        return ((speed - 21) / 21) + 1;  // Returns 1-2
    }
    else if (speed <= 105)
    {
        // Accelerating beyond cruise (2 to 4 pixels/frame)
        return 2 + ((speed - 42) * 2) / 63;  // Returns 2-4
    }
    else if (speed <= 210)
    {
        // High speed (4 to 6 pixels/frame)
        return 4 + ((speed - 105) * 2) / 105;  // Returns 4-6
    }
    else
    {
        return 6;  // Max
    }
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

    ULONG file_size = findSize((char *)filename);
    UBYTE *file_data = Disk_AllocAndLoadAsset((char *)filename, MEMF_ANY);
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

// Game Ready Stuff

// Add these static variables with the other timers
static GameTimer ready_blink_timer;
static BOOL ready_text_visible = TRUE;

void GameReady_Initialize(void)
{
      // Turn off sprites
    Sprites_Initialize();

    // Clear entire screen including HUD area
    BlitClearScreen(draw_buffer, 320 << 6 | 256);
    BlitClearScreen(display_buffer, 320 << 6 | 256);
 
    // Start blink timer
    ready_text_visible = TRUE;
    Timer_StartMs(&ready_blink_timer, 800);  // Blink every 800ms

    Font_DrawStringCentered(draw_buffer, "            PUSH BUTTON", 60, 17);   
    Font_DrawStringCentered(draw_buffer, "            ONLY 1 PLAYER", 100, 17);  
}

void GameReady_Draw(void)
{
    // Handle blinking "PUSH BUTTON" text
    if (Timer_HasElapsed(&ready_blink_timer))
    {
        ready_text_visible = !ready_text_visible;
        
        if (ready_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "            PUSH BUTTON", 60, 17);   
            Font_DrawStringCentered(draw_buffer, "            ONLY 1 PLAYER", 100, 17);  
        }
        else
        {
            // Clear the "PUSH BUTTON" area
            Font_ClearArea(draw_buffer, 0, 60, SCREENWIDTH, 8);
        }
        
        Timer_Reset(&ready_blink_timer);
    }
}

void GameReady_Update(void)
{
    if (JoyFirePressed())
    {
        // Clear screens for game start
        BlitClearScreen(draw_buffer, 320 << 6 | 256);
        BlitClearScreen(display_buffer, 320 << 6 | 256);
        
        // Transition to actual game
        game_state = STAGE_START;

        stage_state = STAGE_COUNTDOWN;
        countdown_value = 4;
        Timer_Start(&countdown_timer, 1);  // 1 second timer

        // Start HUD update timer (update every 96 frames)
        Timer_StartMs(&hud_update_timer, 96);
    
        Game_NewGame(0);

        Game_ApplyPalette(city_colors,BLOCKSCOLORS);

        MotorBike_Reset();
        HUD_SetSpritePositions();
        HUD_DrawAll();
        
        Game_FillScreen();

    }    
 
}

void Stage_Initialize(void)
{
	la_tiles = BitMapEx_Create(BLOCKSDEPTH, BLOCKSWIDTH, BLOCKSHEIGHT);
	Disk_LoadAsset((UBYTE *)la_tiles->planes[0],"tiles/lv1_tiles.BPL");
	Disk_LoadAsset((UBYTE *)city_colors,"tiles/lv1_tiles.PAL");
	la_map = Disk_AllocAndLoadAsset("maps/level1.map",MEMF_PUBLIC);

     // Used for the Countdown
    Sprites_LoadFromFile(COUNTDOWN_ZERO,&spr_countdown_timer[0]);
    Sprites_LoadFromFile(COUNTDOWN_ONE,&spr_countdown_timer[1]);
    Sprites_LoadFromFile(COUNTDOWN_TWO,&spr_countdown_timer[2]);
    Sprites_LoadFromFile(COUNTDOWN_THREE,&spr_countdown_timer[3]);

    spr_countdown[0] = Mem_AllocChip(32);
    spr_countdown[1] = Mem_AllocChip(32);
    spr_countdown[2] = Mem_AllocChip(32);
    spr_countdown[3] = Mem_AllocChip(32);

    Sprites_BuildComposite(spr_countdown[0],2,&spr_countdown_timer[0]);
    Sprites_BuildComposite(spr_countdown[1],2,&spr_countdown_timer[1]);
    Sprites_BuildComposite(spr_countdown[2],2,&spr_countdown_timer[2]);
    Sprites_BuildComposite(spr_countdown[3],2,&spr_countdown_timer[3]); 

    current_countdown_spr = spr_countdown[3];
 
    mapdata = (UWORD *)la_map->data;
	mapwidth = la_map->mapwidth;
	mapheight = la_map->mapheight;   

}

void Stage_Draw()
{
 
    if (stage_state == STAGE_COUNTDOWN)
    {
        Stage_ShowInfo();
        game_frame_count = 0;
    }
    else if (stage_state == STAGE_PLAYING)
    {
        Game_SwapBuffers();
    }
    
     // === PERIODIC HUD UPDATE ===
    if (Timer_HasElapsed(&hud_update_timer))
    {
        //HUD_UpdateScore(player_score);     // Use actual score variable
        //HUD_UpdateRank(player_rank);       // Use actual rank variable
        HUD_UpdateBikeSpeed(bike_speed);
        
        Timer_Reset(&hud_update_timer);
    }

    MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
   // Cars_Update(); 
}

void Stage_Update()
{
    game_frame_count++;

    if (stage_state == STAGE_COUNTDOWN)
    {
        // Handle countdown timer
        if (Timer_HasElapsed(&countdown_timer))
        {
            if (countdown_value > 0)
            {
                countdown_value--;
                Timer_Reset(&countdown_timer);  // Reset for next second
                current_countdown_spr = spr_countdown[countdown_value];
                Sprites_SetPointers(current_countdown_spr, 2, SPRITEPTR_TWO_AND_THREE);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[0],96,100,32);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[1],96,100,32);
            }
            else
            {
                // Countdown complete - start gameplay
                stage_state = STAGE_PLAYING;
                Timer_Stop(&countdown_timer);

                Sprites_ClearHigher();
            }
        }
    }
    else if (stage_state == STAGE_PLAYING)
    {
      
        // === ACCELERATION LOGIC ===

        if (JoyFireHeld())
        {
            // Fire button held - accelerate to max speed
            bike_speed += ACCEL_RATE;   
            if (bike_speed > MAX_SPEED)
            {
                bike_speed = MAX_SPEED;
            }
            bike_state = BIKE_STATE_ACCELERATING;
        }
        else
        {
            // Fire button not held - auto-adjust to cruising speed
            if (bike_speed < MIN_CRUISING_SPEED)
            {
                // Auto-accelerate to cruising speed SLOWLY  
                bike_speed += 1;   
                if (bike_speed > MIN_CRUISING_SPEED)
                {
                    bike_speed = MIN_CRUISING_SPEED;
                }
                bike_state = BIKE_STATE_ACCELERATING;
            }
            else if (bike_speed > MIN_CRUISING_SPEED)
            {
                // Decelerate back to cruising speed
                bike_speed -= DECEL_RATE;
                if (bike_speed < MIN_CRUISING_SPEED)
                {
                    bike_speed = MIN_CRUISING_SPEED;
                }
                bike_state = BIKE_STATE_BRAKING;
            }
            else
            {
                // Maintain cruising speed
                bike_state = BIKE_STATE_MOVING;
            }
        }        
        // === SCROLLING LOGIC  
        SmoothScroll();   
        
        // === LEFT/RIGHT MOVEMENT ===
        if (JoyLeft())
        {
            bike_position_x -= 2;
            // Preserve acceleration state but show turning
            if (bike_state == BIKE_STATE_MOVING || bike_state == BIKE_STATE_ACCELERATING)
            {
                bike_state = BIKE_STATE_LEFT;
            }
        }

        if (JoyRight())
        {
            bike_position_x += 2;
            if (bike_state == BIKE_STATE_MOVING || bike_state == BIKE_STATE_ACCELERATING)
            {
                bike_state = BIKE_STATE_RIGHT;
            }
        }
    }
}
 
void Stage_ShowInfo(void)
{
    char* difficulty_text;
    char* stage_text;
    
    // Determine difficulty text
    switch(game_difficulty)
    {
        case FIVEHUNDREDCC:
            difficulty_text = "500CC    READY";
            break;
        case SEVENFIFTYCC:
            difficulty_text = "750CC    READY";
            break;
        case TWELVEHUNDREDCC:
            difficulty_text = "1200CC   READY";
            break;
        default:
            difficulty_text = "500CC    READY";
            break;
    }
    
    // Determine stage text
    switch(game_stage)
    {
        case STAGE_LASANGELES:
            stage_text = " LOS ANGELES";
            break;
        case STAGE_LASVEGAS:
            stage_text = "   LAS VEGAS";
            break;
        case STAGE_HOUSTON:
            stage_text = "    HOUSTON";
            break;
        case STAGE_STLOUIS:
            stage_text = "   ST.LOUIS";
            break;
        case STAGE_CHICAGO:
            stage_text = "   CHICAGO";
            break;
        case STAGE_NEWYORK:
            stage_text = "   NEW YORK";
            break;
        default:
            stage_text = " LOS ANGELES";
            break;
    }
    
    // Draw difficulty at top  
    Font_DrawString(draw_buffer, difficulty_text, 48, 50, 12);  
 
    // Draw stage name at bottom  
    Font_DrawStringCentered(draw_buffer, stage_text, 120, 12);   
}