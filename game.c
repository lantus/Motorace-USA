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
#include "blitter.h"
#include "city_approach.h"
#include "roadsystem.h"

#include "cars.h"
#include "audio.h"

extern volatile struct Custom *custom;
extern BlitterObject nyc_horizon;
extern BlitterObject lv_horizon;
extern BlitterObject *city_horizon;


// Precomputed scroll amounts for speeds 0-255 (8.8 fixed point)
#define MAX_SPEED_TABLE 256
const UWORD scroll_speed_table[MAX_SPEED_TABLE] = {
    0, 12, 24, 36, 48, 60, 73, 85, 97, 109, 121, 133, 146, 158, 170, 182, 194, 207, 219, 231, 243, 256,
    268, 280, 292, 304, 317, 329, 341, 353, 365, 377, 390, 402, 414, 426, 438, 451, 463, 475, 487, 499, 512,
    520, 528, 537, 545, 553, 561, 569, 577, 586, 594, 602, 610, 618, 626, 635, 643, 651, 659, 667, 675, 684,
    692, 700, 708, 716, 724, 733, 741, 749, 757, 765, 773, 782, 790, 798, 806, 814, 822, 831, 839, 847, 855,
    863, 871, 880, 888, 896, 904, 912, 920, 929, 937, 945, 953, 961, 969, 978, 986, 994, 1002, 1010, 1018, 1024,
    1028, 1033, 1038, 1042, 1047, 1052, 1057, 1061, 1066, 1071, 1076, 1080, 1085, 1090, 1095, 1099, 1104, 1109,
    1114, 1118, 1123, 1128, 1133, 1137, 1142, 1147, 1152, 1156, 1161, 1166, 1171, 1175, 1180, 1185, 1190, 1194,
    1199, 1204, 1209, 1213, 1218, 1223, 1228, 1232, 1237, 1242, 1247, 1251, 1256, 1261, 1266, 1270, 1275, 1280,
    1285, 1289, 1294, 1299, 1304, 1308, 1313, 1318, 1323, 1327, 1332, 1337, 1342, 1346, 1351, 1356, 1361, 1365,
    1370, 1375, 1380, 1384, 1389, 1394, 1399, 1403, 1408, 1413, 1418, 1422, 1427, 1432, 1437, 1441, 1446, 1451,
    1456, 1460, 1465, 1470, 1475, 1479, 1484, 1489, 1494, 1498, 1503, 1508, 1513, 1517, 1522, 1527, 1532, 1536,
    1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
    1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
    1536, 1536, 1536, 1536, 1536, 1536
};

UBYTE stage_state = STAGE_BEGIN;
UBYTE stage_complete = 0;

GameTimer countdown_timer;
GameTimer hud_update_timer;
GameTimer collision_recovery_timer;

CollisionState collision_state = COLLISION_NONE;
int collision_car_index = -1;
UBYTE countdown_value = 5;

UBYTE game_stage = STAGE_LASANGELES;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDREDCC;
UBYTE game_map = MAP_ATTRACT_INTRO;
ULONG game_score = 0;
UBYTE game_rank = 90;
ULONG game_frame_count = 0;

UBYTE game_car_block_move_rate = 5;   // Frames between movements (lower = faster)
UBYTE game_car_block_move_speed = 1;  // Pixels per move (higher = faster)
UBYTE game_car_block_x_threshold = 32; // How close bike needs to be horizontally

UWORD frontview_bike_frames = 0;

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
UWORD	lv_colors[BLOCKSCOLORS];
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
    
    Audio_Initialize();     // SFX
    
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
    TileAttrib_Load();
 
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;

    Game_SetMap(game_map);

    MotorBike_Reset();

    KPrintF("Avail Chip  = %ld\n", Mem_GetFreeChip());
    KPrintF("Avail Fast  = %ld\n", Mem_GetFreeFast());
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASANGELES;
    game_state = STAGE_START;
    game_map = MAP_OVERHEAD_LASANGELES;
    collision_state = COLLISION_NONE;
    game_difficulty = difficulty;
    game_score = 0;
    bike_speed = 0;
    game_rank = 99; // Start in 99th

    Game_SetMap(game_map);

    // Position bike near bottom of screen
    bike_position_x = 96;
    bike_position_y = SCREENHEIGHT - 24;  // Near bottom
    bike_state = BIKE_STATE_STOPPED; 
    
    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT - 1;
    videoposy = mapposy % HALFBITMAPHEIGHT ;

    Cars_ResetPositions();
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
    game_map = maptype;
    switch (maptype)
    {
        case MAP_ATTRACT_INTRO:
            mapdata = (UWORD *)city_attract_map->data;
            mapwidth = city_attract_map->mapwidth;
            mapheight = city_attract_map->mapheight;  
            blocksbuffer = city_attract_tiles->planes[0];
            city_horizon = &nyc_horizon;
            break;
        case MAP_OVERHEAD_LASANGELES:
            mapdata = (UWORD *)la_map->data;
            mapwidth = la_map->mapwidth;
            mapheight = la_map->mapheight;  
            blocksbuffer = la_tiles->planes[0];
            city_horizon = &lv_horizon;
            break;
        case MAP_FRONTVIEW_LASVEGAS:
            mapdata = (UWORD *)city_attract_map->data;
            mapwidth = city_attract_map->mapwidth;
            mapheight = city_attract_map->mapheight;  
            blocksbuffer = city_attract_tiles->planes[0];
            city_horizon = &lv_horizon;
            break;
        case MAP_OVERHEAD_LASVEGAS:
            break;
    }
}
 

 
__attribute__((always_inline)) WORD GetScrollAmount(WORD speed)
{
    if (speed >= MAX_SPEED_TABLE)
        return scroll_speed_table[MAX_SPEED_TABLE - 1];
    
    return scroll_speed_table[speed];
}

static void SmoothScroll(void)
{
    
    WORD scroll_speed = GetScrollAmount(bike_speed);
    
    scroll_accumulator += scroll_speed;
    
    while (scroll_accumulator >= 256) 
    {
        ScrollUp();
        scroll_accumulator -= 256;
    }
}
 

__attribute__((always_inline)) inline void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy, UBYTE *dest)
{
	x = (x >> 3) & 0xFFFE;
	y = (y << 4) + (y << 3);
	
	UWORD block = mapdata[mapy * mapwidth + mapx];

	mapx = (block % BLOCKSPERROW) * (BLOCKWIDTH >> 3);
	mapy = (block / BLOCKSPERROW) * (BLOCKPLANELINES * BLOCKSBYTESPERROW);
 
	WaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = BLOCKSBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH >> 4);
}
 
__attribute__((always_inline)) inline void DrawBlocks(LONG x,LONG y,LONG mapx,LONG mapy, UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, BOOL deltas_only, UBYTE tile_idx, UBYTE *dest)
{
	 
	x = (x >> 3) & 0xFFFE;
	y = (y << 4) + (y << 3);
 

	UWORD block = mapdata[mapy * mapwidth + mapx];
  
	mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
	mapy = (block / blocksperrow) * (blockplanelines * blockbytessperrow);
 
    WaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = blockbytessperrow - (BLOCKWIDTH >> 3);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = blockplanelines * 64 + (BLOCKWIDTH >> 4);
 
}
 
__attribute__((always_inline)) inline void DrawBlockRun(LONG x, LONG y, UWORD block, WORD count, UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
{
    x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3); // y * 16 + y * 8 = y * 24

    UWORD mapx = (block % blocksperrow) << 1;
    UWORD mapy = (block / blocksperrow) * (blockplanelines * blockbytesperrow);
    
    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytesperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);  // Skip over tiles we're not writing
    custom->bltapt  = blocksbuffer + mapy + mapx;
    custom->bltdpt  = dest + y + x;
    
    // Blit width is count * BLOCKWIDTH
    custom->bltsize = blockplanelines * 64 + (BLOCKWIDTH >> 4);
}
 
static void ScrollUp(void)
{
    WORD mapx, mapy, x, y;

    if (mapposy < 1) return;  // Stop at top of map

    mapposy--;
    videoposy = mapposy % HALFBITMAPHEIGHT;

	mapx = mapposy & (NUMSTEPS - 1);
	mapy = mapposy >> 4;
	
	y = ROUND2BLOCKHEIGHT(videoposy) << 2;

   // Only draw if within the 12-tile display width
    if (mapx < 12) 
    {  
        UWORD yoff = y + (HALFBITMAPHEIGHT << 2);
        // Limit to 192 pixels (12 tiles)
        x = mapx << 4;
        
        DrawBlock(x, y, mapx, mapy, screen.bitplanes);
        DrawBlock(x, yoff, mapx, mapy,screen.bitplanes);

        DrawBlock(x, y, mapx, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, yoff, mapx, mapy,screen.offscreen_bitplanes);    
 
        DrawBlock(x, y, mapx, mapy, screen.pristine);
        DrawBlock(x, yoff, mapx, mapy,screen.pristine);    
 
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
			DrawBlock(x, y, a, start_tile_y + b, screen.bitplanes);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b,screen.bitplanes);

           	DrawBlock(x, y, a, start_tile_y + b, screen.offscreen_bitplanes);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b,screen.offscreen_bitplanes); 

            DrawBlock(x, y, a, start_tile_y + b, screen.pristine);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b,screen.pristine);       

   
		}
	} 
 
}
 

void Game_SwapBuffers(void)
{
    // Toggle current buffer
 
    // Update copper bitplane pointers to show the new display buffer
 
    const UBYTE* planes_temp[BLOCKSDEPTH];
    
    planes_temp[0] = draw_buffer;
    planes_temp[1] = draw_buffer + 24;
    planes_temp[2] = draw_buffer + 48;
    planes_temp[3] = draw_buffer + 72;

    LONG planeadd = ((LONG)(videoposy + BLOCKHEIGHT)) * SCREENWIDTH_WORDS;
 
    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, planeadd);
}

void Game_ResetBitplanePointer(void)
{
    // Reset copper bitplane pointers  
 
    const UBYTE* planes_temp[BLOCKSDEPTH];
    
    planes_temp[0] = draw_buffer;
    planes_temp[1] = draw_buffer + 24;
    planes_temp[2] = draw_buffer + 48;
    planes_temp[3] = draw_buffer + 72;

    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, 0);
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
    BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
    BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 256);
 
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
        BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
        BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 256);
        
        // Transition to actual game
        game_state = STAGE_START;

        stage_state = STAGE_BEGIN;

        // Start HUD update timer (update every 96 frames)
        Timer_StartMs(&hud_update_timer, 96);
    
        Game_NewGame(0);

        Game_ApplyPalette(city_colors,BLOCKSCOLORS);

        MotorBike_Reset();

        HUD_SetSpritePositions();
        HUD_DrawAll();
        
        Game_FillScreen();

        Stage_ShowInfo();

        Music_LoadModule(MUSIC_START);
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
        if (game_frame_count == 0)
        {
            Stage_ShowInfo();   
        }
        Cars_PreDraw();
        Game_SwapBuffers();  // Add this
        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
    }
    else if (stage_state == STAGE_PLAYING)
    {
        SmoothScroll();
        Cars_Update();
        bike_world_y = mapposy + bike_position_y;
        Game_SwapBuffers();

        // === PERIODIC HUD UPDATE ===
        if (Timer_HasElapsed(&hud_update_timer))
        {
            //HUD_UpdateScore(player_score);     // Use actual score variable
            //HUD_UpdateRank(player_rank);       // Use actual rank variable
            HUD_UpdateBikeSpeed(bike_speed);
            Timer_Reset(&hud_update_timer);
        }

        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
        Game_HandleCollisions();
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {       
        City_RestoreOncomingCars();
        City_DrawRoad();
        City_DrawOncomingCars();

        Game_ResetBitplanePointer();
 
        // Apply vibration offset for distant sprites (ADD THIS)
        WORD vibration_x = MotorBike_GetVibrationOffset();

        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
        MotorBike_Draw(bike_position_x + vibration_x, bike_position_y, 0);
    }
    else
    {
        City_DrawRoad();
    }
    

}

void Stage_Update()
{
    game_frame_count++;

    if (stage_state == STAGE_BEGIN)
    {
        countdown_value = 4;
        Timer_Start(&countdown_timer, 1);  // 1 second timer

        stage_state = STAGE_COUNTDOWN;
    }
    else if (stage_state == STAGE_COUNTDOWN)
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
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[0],96,120,32);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[1],96,120,32);
            }
            else
            {
                WORD difficulty_y = videoposy + BLOCKHEIGHT + 70;
                WORD stage_y = videoposy + BLOCKHEIGHT + 120;

                if (difficulty_y >= BITMAPHEIGHT)
                    difficulty_y -= BITMAPHEIGHT;
                if (stage_y >= BITMAPHEIGHT)
                    stage_y -= BITMAPHEIGHT;
    

                Font_RestoreFromPristine(screen.bitplanes, 48, difficulty_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.offscreen_bitplanes, 48, difficulty_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.bitplanes, 48, stage_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.offscreen_bitplanes, 48, stage_y, 192, CHAR_HEIGHT);
                // Countdown complete - start gameplay
                stage_state = STAGE_PLAYING;
                Timer_Stop(&countdown_timer);

                Sprites_ClearHigher();

                Music_LoadModule(MUSIC_ONROAD);
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

        Stage_CheckCompletion();
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {
        if (City_OncomingCarsIsComplete())
        {
            stage_state = STAGE_COMPLETE;
            KPrintF("=== Stage 1 Complete! ===\n");
            return;
        }

        CityApproachState approach_state = City_GetApproachState();
 
        if (frontview_bike_crashed == FALSE && approach_state < CITY_STATE_INTO_HORIZON )
        {
            if (JoyFireHeld())
            {
                // Fire button held - accelerate to max speed
                bike_speed += ACCEL_RATE;   
                if (bike_speed > MAX_SPEED)
                {
                    bike_speed = MAX_SPEED;
                }
                bike_state = BIKE_FRAME_APPROACH1;
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
                    bike_state = BIKE_FRAME_APPROACH1;
                }
                else if (bike_speed > MIN_CRUISING_SPEED)
                {
                    // Decelerate back to cruising speed
                    bike_speed -= DECEL_RATE;
                    if (bike_speed < MIN_CRUISING_SPEED)
                    {
                        bike_speed = MIN_CRUISING_SPEED;
                    }
                    bike_state = BIKE_FRAME_APPROACH1;
                }
                else
                {
                    // Maintain cruising speed
                    bike_state = BIKE_FRAME_APPROACH1;
                }
    
            }        
    
            // === LEFT/RIGHT MOVEMENT ===
            if (JoyLeft())
            {
                bike_position_x -= 2;
                bike_state = BIKE_STATE_FRONTVIEW_LEFT;
            }
            else if (JoyRight())
            {
                bike_position_x += 2;
                bike_state = BIKE_STATE_FRONTVIEW_RIGHT;
            }
            else
            {
                static UBYTE frontview_bike_anim_frame = 0;   
                UWORD anim_speed = 8 - (bike_speed / 30);
                
                if (anim_speed < 2) anim_speed = 2;
                
                if (game_frame_count % anim_speed == 0)
                {
                    frontview_bike_anim_frame ^= 1;
                }
                
                bike_state = frontview_bike_anim_frame ? BIKE_FRAME_APPROACH2 : BIKE_FRAME_APPROACH1;
            }
        }

        if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            City_UpdateHorizonTransition(&bike_position_y, &bike_speed, game_frame_count);

            bike_position_x = City_CalculateBikePerspectiveX(bike_position_y, 
                                                        City_GetBikeHorizonStartX());

            MotorBike_UpdateApproachFrame(bike_position_y);
        }
        else
        {
            // Normal gameplay - use the bike_state set above
            MotorBike_SetFrame(bike_state);
        }

        // Update road scrolling
        if (approach_state == CITY_STATE_WAITING_NAME || approach_state == CITY_STATE_ACTIVE)
        {
            UpdateRoadScroll(bike_speed << 1, 0);
        }
        else if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            UpdateRoadScroll(bike_speed, game_frame_count);
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
            game_car_block_move_rate = 4;   // Slow (every 10 frames)
            game_car_block_move_speed = 1;   // 1 pixel at a time
            game_car_block_x_threshold = 48; // Wide threshold 
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
    
    WORD difficulty_y = videoposy + BLOCKHEIGHT + 70;
    WORD stage_y = videoposy + BLOCKHEIGHT + 120;

    if (difficulty_y >= BITMAPHEIGHT)
        difficulty_y -= BITMAPHEIGHT;
    if (stage_y >= BITMAPHEIGHT)
        stage_y -= BITMAPHEIGHT;
 
    Font_DrawString(screen.bitplanes, difficulty_text, 48, difficulty_y, 13);
    Font_DrawStringCentered(screen.bitplanes, stage_text, stage_y, 13);
    Font_DrawString(screen.offscreen_bitplanes, difficulty_text, 48, difficulty_y, 13);
    Font_DrawStringCentered(screen.offscreen_bitplanes, stage_text, stage_y, 13);
}

void Game_HandleCollisions(void)
{
    // Handle NEW collisions only if not already in collision
    if (collision_state == COLLISION_NONE)
    {
        int hit_car = -1;
        collision_state = MotorBike_CheckCollision(&hit_car);
        
        if (collision_state == COLLISION_TRAFFIC)
        {
            collision_car_index = hit_car;
            Timer_Start(&collision_recovery_timer, 2);  // 2 seconds recovery
            Cars_HandleSpinout(hit_car);

            Music_Stop();
        }
        else if (collision_state == COLLISION_OFFROAD)
        {
            Timer_Start(&collision_recovery_timer, 2);  // 2 seconds recovery
        }
    }
    
    // Handle ONGOING collision state
    if (collision_state != COLLISION_NONE)
    {
        // Decelerate bike
        if (collision_state == COLLISION_TRAFFIC)
        {
            bike_state = BIKE_STATE_CRASHED;

            if (bike_speed > 0)
            {
                bike_speed -= 10;  // Rapid deceleration
                if (bike_speed < 0) bike_speed = 0;
                
            }
        }
        else if (collision_state == COLLISION_OFFROAD)
        {
            if (bike_speed > 20)
            {
                bike_speed -= 3;  // Slower deceleration
            }
        }
        
        // Check if 2 seconds have elapsed
        if (Timer_HasElapsed(&collision_recovery_timer))
        {
            // Resume play
            collision_state = COLLISION_NONE;
            collision_car_index = -1;
            Timer_Stop(&collision_recovery_timer);
          
            Music_LoadModule(MUSIC_ONROAD);

            bike_state = BIKE_STATE_MOVING;
        }
    }

    if (collision_state == COLLISION_TRAFFIC)
        Copper_SetPalette(0,0xF00);
    else if (collision_state == COLLISION_OFFROAD)
        Copper_SetPalette(0,0xFF0);
    else
        Copper_SetPalette(0,0x00);
 
}

void Stage_CheckCompletion(void)
{
    // Check if bike reached the top of the map (end of stage)
    // Map starts at high Y values and scrolls toward 0
    
    if (mapposy <= 12000)  // Near the top/end of map
    {
        stage_state = STAGE_FRONTVIEW;

        // clear screens and initialize frontview
        Stage_InitializeFrontView();
        
    }
}

void Stage_InitializeFrontView(void)
{
    Game_ResetBitplanePointer();

    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

    bike_position_x = 80;
    bike_position_y = 200;
 
    Sprites_ClearLower();
    Sprites_ClearHigher();

    Game_SetMap(MAP_FRONTVIEW_LASVEGAS);

    MotorBike_Reset();

    Game_ApplyPalette(lv_colors,BLOCKSCOLORS);
    Game_SetBackGroundColor(0x00);
    
    Title_Reset();
   
    City_PreDrawRoad();
    City_OncomingCarsReset();

    City_ShowCityName("LAS VEGAS");

    HUD_DrawAll();

    Music_Stop();
    Music_LoadModule(MUSIC_FRONTVIEW);
}