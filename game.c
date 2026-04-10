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
#include "tilesheet.h"
#include "motorbike.h"
#include "planes.h"
#include "hud.h"
#include "font.h"
#include "title.h"
#include "hiscore.h"
#include "hiscore_entry.h"
#include "blitter.h"
#include "city_approach.h"
#include "roadsystem.h"
#include "ranking.h"
#include "stageprogress.h"
#include "fuel.h"

#include "cars.h"
#include "audio.h"

extern volatile struct Custom *custom;
extern BlitterObject nyc_horizon;
extern BlitterObject lv_horizon;
extern BlitterObject houston_horizon;
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

// PAL (50fps) — each NTSC value * 1.2 to match game feel
const UWORD scroll_speed_table_pal[MAX_SPEED_TABLE] = {
    0, 14, 29, 43, 58, 72, 88, 102, 116, 131, 145, 160, 175, 190, 204, 218, 233, 248, 263, 277, 292, 307,
    322, 336, 350, 365, 380, 395, 409, 424, 438, 452, 468, 482, 497, 511, 526, 541, 556, 570, 584, 599, 614,
    624, 634, 644, 654, 664, 673, 683, 692, 703, 713, 722, 732, 742, 751, 762, 772, 781, 791, 800, 810, 821,
    830, 840, 850, 859, 869, 880, 889, 899, 908, 918, 928, 938, 948, 958, 967, 977, 986, 997, 1007, 1016, 1026,
    1036, 1045, 1056, 1066, 1075, 1085, 1094, 1104, 1115, 1124, 1134, 1144, 1153, 1163, 1174, 1183, 1193, 1202, 1212, 1222, 1229,
    1234, 1240, 1246, 1250, 1256, 1262, 1268, 1273, 1279, 1285, 1291, 1296, 1302, 1308, 1314, 1319, 1325, 1331,
    1337, 1342, 1348, 1354, 1360, 1365, 1370, 1376, 1382, 1387, 1393, 1399, 1405, 1410, 1416, 1422, 1428, 1433,
    1439, 1445, 1451, 1456, 1462, 1468, 1474, 1478, 1484, 1490, 1496, 1501, 1507, 1513, 1519, 1524, 1530, 1536,
    1542, 1547, 1553, 1559, 1565, 1570, 1576, 1582, 1588, 1592, 1598, 1604, 1610, 1616, 1621, 1627, 1633, 1638,
    1644, 1650, 1656, 1661, 1667, 1673, 1679, 1684, 1690, 1696, 1702, 1706, 1712, 1718, 1724, 1729, 1735, 1741,
    1747, 1752, 1758, 1764, 1770, 1775, 1781, 1787, 1793, 1798, 1804, 1810, 1816, 1821, 1827, 1833, 1838, 1843,
    1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843,
    1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843,
    1843, 1843, 1843, 1843, 1843, 1843
};

UBYTE stage_state = STAGE_BEGIN;
UBYTE stage_complete = 0;

GameTimer countdown_timer;
GameTimer hud_update_timer;
GameTimer collision_recovery_timer;

CollisionState collision_state = COLLISION_NONE;
int collision_car_index = -1;
UBYTE countdown_value = 5;

UBYTE game_stage = STAGE_LASVEGAS;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDREDCC;
UBYTE game_map = MAP_ATTRACT_INTRO;
UWORD max_stage_speed = MAX_SPEED;
ULONG game_score;
UBYTE game_rank;
ULONG game_frame_count = 0;

UBYTE game_car_block_move_rate = 5;   // Frames between movements (lower = faster)
UBYTE game_car_block_move_speed = 1;  // Pixels per move (higher = faster)
UBYTE game_car_block_x_threshold = 32; // How close bike needs to be horizontally

ULONG speed_accumulator = 0;
ULONG speed_sample_count = 0;

UWORD frontview_bike_frames = 0;

WORD mapposy,videoposy;
LONG mapwidth,mapheight;

UBYTE *frontbuffer,*blocksbuffer;
UWORD *mapdata;
 
struct BitMapEx *BlocksBitmap,*ScreenBitmap;
 
 
// Palettes
UWORD	intro_colors[BLOCKSCOLORS];
UWORD	lv_colors[BLOCKSCOLORS];
UWORD	houston_colors[BLOCKSCOLORS];
UWORD	city_colors[BLOCKSCOLORS];
UWORD	offroad_colors[BLOCKSCOLORS];
UWORD	stlouis_colors[BLOCKSCOLORS];
UWORD   black_palette[BLOCKSCOLORS] = {0};
UWORD   palette_fv_stl[BLOCKSCOLORS];
UWORD  *current_palette; 

// Used for the Countdown
static Sprite spr_countdown_timer[4];
ULONG *spr_countdown[4];
ULONG *current_countdown_spr;
UBYTE countdown_idx = 0;

// One time startup for everything

// At top with other static variables
static GameTimer stage_complete_timer;
static WORD ranking_backdrop_y = 0;   

const UWORD *scroll_speed_table_active;

// HUD stuff
static ULONG last_score = 0xFFFFFFFF;
static UBYTE last_rank = 0xFF;
static WORD  last_speed = -1;
static UBYTE hud_phase = 0;

// hiscore/gameover stuff

static GameTimer gameover_timer;
static GameTimer gameover_flash_timer;
static BOOL gameover_text_visible = TRUE;
static UBYTE gameover_hiscore_pos = 0;  /* 0 = doesn't qualify */

// Bike repositioning stuff
BOOL bike_invulnerable = FALSE;
 
WORD bike_wy = 0;

void Game_Initialize()
{
    Timer_Init();
    Sprites_Initialize();
    HiScore_Initialize();
    
    Audio_Initialize();
    MotorBike_Initialize();
    Planes_Initialize();
    NYCVictory_Initialize();

    HUD_InitSprites();
    HUD_SetSpritePositions();
    
    /* Initialize tilesheet pool BEFORE Title and Stage */
    TilesheetPool_Initialize();
    
    Title_Initialize();
    City_Initialize();
    
    Game_SetBackGroundColor(0x125);
    
    Stage_Initialize();
 
    
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;
    
    /* Load city attract tiles for title screen */
    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);
    
    Game_SetMap(game_map);
    MotorBike_Reset();
    MotorBike_ResetApproachIndex();
    Fuel_Initialize();
    StageProgress_Initialize();
    
    
    scroll_speed_table_active = g_is_pal ? scroll_speed_table_pal : scroll_speed_table;
}

 
void Game_Reset(void)
{
    Transition_ToBlack();

    title_state = TITLE_ATTRACT_INIT;
    Title_Reset();
 
    Game_ResetBitplanePointer();

    draw_buffer = current_buffer == 0 ? screen.bitplanes : screen.offscreen_bitplanes;
    display_buffer = current_buffer == 0 ? screen.offscreen_bitplanes : screen.bitplanes;

    /* Clear all screens */
    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

    /* Turn off sprites */
    Sprites_ClearLower();
    Sprites_ClearHigher();

    title_state = TITLE_ATTRACT_START;
    Game_ApplyPalette(black_palette, BLOCKSCOLORS);
    Game_SetBackGroundColor(0x125);

    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;

    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);


    City_ResetRoadState();

    Game_SetMap(game_map);
    MotorBike_Reset();
    MotorBike_ResetApproachIndex();
    Fuel_Initialize();
    StageProgress_Initialize();

    Game_ApplyPalette(intro_colors, BLOCKSCOLORS);
    game_frame_count = 0;

    Transition_FromBlack(intro_colors, BLOCKSCOLORS);
}
 

void Game_AdvanceStage(void)
{
    Music_Stop();

    game_stage++;
    
    if (game_stage > STAGE_NEWYORK)
    {
       /* Completed all 5 stages — loop with higher difficulty */
        game_stage = STAGE_LASVEGAS;
        game_rank = 99;  /* Reset rank for new loop */

        switch (game_difficulty)
        {
            case FIVEHUNDREDCC:
                game_difficulty = SEVENFIFTYCC;
                break;
            case SEVENFIFTYCC:
                game_difficulty = TWELVEHUNDREDCC;
                break;
            case TWELVEHUNDREDCC:
                /* Already max difficulty — stay here */
                break;
        }
        
        NYCVictory_Stop();
    }
    
    /* More stages — start next overhead */
    Game_StartNextOverhead();
}

void Game_StartNextOverhead(void)
{
    Transition_ToBlack();

    game_state = STAGE_START;
    stage_state = STAGE_BEGIN;
    
    /* Keep score, fuel, rank — don't reset them */
    
    collision_state = COLLISION_NONE;
    bike_speed = 0;

     /* ---- Difficulty scaling ---- */
    WORD base_speed;
    WORD max_active_cars;
    
    switch (game_difficulty)
    {
        case FIVEHUNDREDCC:
            base_speed = 210;
            max_active_cars = 2;
            break;
        case SEVENFIFTYCC:
            base_speed = 250;
            max_active_cars = 3;
            break;
        case TWELVEHUNDREDCC:
            base_speed = 300;
            max_active_cars = 4;
            break;
        default:
            base_speed = 210;
            max_active_cars = 2;
            break;
    }
    
   // max_car_count = max_active_cars;  
    
    /* Per-stage settings: speed, tilesheet, map, music */
    UBYTE stage_music;
    switch (game_stage)
    {
        case STAGE_LASVEGAS:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL1);
            game_map = STAGE1_OVERHEAD;
            stage_music = MUSIC_START;
            current_palette = city_colors;
            road_tile_plain = 11;
            break;
        case STAGE_HOUSTON:
            max_stage_speed =  (base_speed * 4) / 5;
            TilesheetPool_Load(TILEPOOL_LEVEL2);
            game_map = STAGE2_OVERHEAD;
            stage_music = MUSIC_OFFROAD;
            current_palette = offroad_colors;
            road_tile_plain = 0;
            break;
        case STAGE_STLOUIS:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL3);
            game_map = STAGE3_OVERHEAD;
            stage_music = MUSIC_ONROAD;
            current_palette = stlouis_colors;
            road_tile_plain = 1;
            break;
        case STAGE_CHICAGO:
            max_stage_speed =  (base_speed * 4) / 5;
            TilesheetPool_Load(TILEPOOL_LEVEL4);
            game_map = STAGE4_OVERHEAD;
            stage_music = MUSIC_OFFROAD;
            current_palette = offroad_colors;
            road_tile_plain = 0;
            break;
        case STAGE_NEWYORK:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL5);
            game_map = STAGE5_OVERHEAD;
            stage_music = MUSIC_ONROAD;
            current_palette = city_colors;
            road_tile_plain = 1; 
            break;
        default:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL1);
            game_map = STAGE1_OVERHEAD;
            stage_music = MUSIC_START;
            break;
    }
    
    Game_SetMap(game_map);
 
    bike_position_x = 96;
    bike_position_y = SCREENHEIGHT - 24;
    bike_state = BIKE_STATE_STOPPED;
    bike_invulnerable = FALSE;
    
    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    mapposy = (mapposy / EFFECTIVE_HEIGHT) * EFFECTIVE_HEIGHT;
    mapposy -= 14;
    videoposy = mapposy % HALFBITMAPHEIGHT;
    
    stage_progress.mapsize = mapposy;
    speed_accumulator = 0;
    speed_sample_count = 0;
    wheelie_active = FALSE;
    wheelie_scored = FALSE;
    
    crash_anim_frames = 0;

    Game_ApplyPalette(black_palette,BLOCKSCOLORS);
    
    /* Stage 1 spawns 5 cars around bike; other stages use respawn system */
    if (game_stage == STAGE_LASVEGAS)
        Cars_ResetPositions();
    else
        Cars_ResetPositionsEmpty();
 
    Game_ResetBitplanePointer();
    
    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

     /* Reset draw buffer pointers */
    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;
    
    /* Reset bike speed for attract mode */
    bike_speed = 0;

    
    Game_FillScreen();
    Road_CacheFillVisible();
    Game_SwapBuffers();
    
    if (game_stage == STAGE_LASVEGAS)
    {
        Stage_ShowInfo();
    }
    
    StageProgress_SetStage(game_stage-1); //UGH
    StageProgress_DrawAll();
    
    MotorBike_Reset();

    HUD_SetSpritePositions();
    HUD_DrawAll();
    HUD_UpdateScore(game_score);
    Fuel_DrawAll();
    
    HUD_UpdateRank(game_rank);
    
    Music_Stop();
    Music_LoadModule(stage_music);
    
    Game_ApplyPalette(current_palette,BLOCKSCOLORS);

    Transition_FromBlack(current_palette, BLOCKSCOLORS);

    KPrintF("=== Starting stage %ld ===\n", game_stage);
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASVEGAS;
    game_state = STAGE_START;
    game_map = STAGE1_OVERHEAD;
    collision_state = COLLISION_NONE;
    game_difficulty = difficulty;
    game_score = 0;
    bike_speed = 0;
    game_rank = 99; // Start in 99th
    max_stage_speed = MAX_SPEED;  /* Stage 1: full speed 210 */
    road_tile_plain = 11;

    /* Swap to level 1 tiles */
    TilesheetPool_Load(TILEPOOL_LEVEL1);
    Game_SetMap(game_map);

    // Position bike near bottom of screen
    bike_position_x = 96;
    bike_position_y = SCREENHEIGHT - 24;  // Near bottom
    bike_state = BIKE_STATE_STOPPED; 
    bike_invulnerable = FALSE;

    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    mapposy = (mapposy / EFFECTIVE_HEIGHT) * EFFECTIVE_HEIGHT;
    mapposy -= 14;
    videoposy = mapposy % HALFBITMAPHEIGHT ;

    //videoposy = 0;
    stage_progress.mapsize = mapposy;
 
    speed_accumulator = 0;
    speed_sample_count = 0;
    wheelie_active = FALSE;
    wheelie_scored = FALSE;
    crash_anim_frames = 0;
 
    
    Cars_ResetPositions();

    HUD_Init1UPFlash();
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
            MapPool_Load(STAGE_ATTRACT);
            current_palette = intro_colors;
           
            break;
        case STAGE1_OVERHEAD:
            MapPool_Load(STAGE_LASVEGAS);
            current_palette = city_colors;
            CollisionMap_SetStage(STAGE_LASVEGAS);
            break;
        case STAGE1_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = lv_colors;
            break;
        case STAGE2_OVERHEAD:
            MapPool_Load(STAGE_HOUSTON);
            current_palette = offroad_colors;
            CollisionMap_SetStage(STAGE_HOUSTON);
            break;
        case STAGE2_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = houston_colors;
            break;     
        case STAGE3_OVERHEAD:
            MapPool_Load(STAGE_STLOUIS);
            current_palette = stlouis_colors;
            CollisionMap_SetStage(STAGE_STLOUIS);
            break;          
        case STAGE3_FRONTVIEW:
             MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;    
        case STAGE4_OVERHEAD:
            MapPool_Load(STAGE_CHICAGO);
            current_palette = offroad_colors;
            CollisionMap_SetStage(STAGE_CHICAGO); 
            break;         
        case STAGE4_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;      
        case STAGE5_OVERHEAD:
            MapPool_Load(STAGE_NEWYORK);
            current_palette = city_colors;
            CollisionMap_SetStage(STAGE_NEWYORK);  
            break;         
        case STAGE5_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;                       
    }

   
}
 
__attribute__((always_inline)) WORD GetScrollAmount(WORD speed)
{
    if (speed >= MAX_SPEED_TABLE)
        return scroll_speed_table_active[MAX_SPEED_TABLE - 1];
    
    return scroll_speed_table_active[speed];
}


static void SmoothScroll(void)
{
    WORD scroll_speed = GetScrollAmount(bike_speed);
    
    scroll_accumulator += scroll_speed;
    
    WORD pixels = scroll_accumulator >> 8;
    if (pixels == 0) return;
    scroll_accumulator &= 0xFF;
    
    if (mapposy - pixels < 1)
        pixels = mapposy - 1;
    if (pixels <= 0) return;
    
    WORD old_mapposy = mapposy;
    
    mapposy -= pixels;
   
    videoposy = mapposy;
    while (videoposy >= HALFBITMAPHEIGHT)
        videoposy -= HALFBITMAPHEIGHT;
    
    UWORD drawn_cols = 0;
    
    for (WORD p = 1; p <= pixels; p++)
    {
        WORD pos = old_mapposy - p;
        WORD col = pos & (NUMSTEPS - 1);
        
        if (col >= 12) continue;
        if (drawn_cols & (1 << col)) continue;
        drawn_cols |= (1 << col);
        
        WORD mapy = pos >> 4;
        WORD y = ROUND2BLOCKHEIGHT(pos % EFFECTIVE_HEIGHT) << 2;
        WORD x = col << 4;
        
        DrawBlock(x, y, col, mapy, screen.bitplanes);
        DrawBlock(x, y, col, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, y, col, mapy, screen.pristine);
 
        UWORD yoff = y + (HALFBITMAPHEIGHT << 2);
        DrawBlock(x, yoff, col, mapy, screen.bitplanes);
        DrawBlock(x, yoff, col, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, yoff, col, mapy, screen.pristine);
 
        /* ---- Cache road center for this tile row ---- */
        if (col == 0)
        {
            Road_CacheRow(pos);
        }
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

__attribute__((always_inline)) inline void DrawBlocksHalf(LONG x, LONG y, LONG mapx, LONG mapy, 
    UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, 
    BOOL deltas_only, UBYTE tile_idx, UBYTE *dest)
{
    UWORD halfplanelines = blockplanelines >> 1;  /* 32 = bottom 8 pixels */
    UWORD src_skip = halfplanelines * blockbytessperrow;  /* Skip top 8px in source */
    UWORD dst_skip = halfplanelines * BITMAPBYTESPERROW;  /* Skip top 8px in dest */
    
    x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3);

    UWORD block = mapdata[mapy * mapwidth + mapx];
  
    mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
    mapy = (block / blocksperrow) * (blockplanelines * blockbytessperrow);

    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytessperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
    custom->bltapt  = blocksbuffer + mapy + mapx + src_skip;   /* Skip top half */
    custom->bltdpt  = dest + y + x + dst_skip;                 /* Draw in bottom half */
    
    custom->bltsize = halfplanelines * 64 + (BLOCKWIDTH >> 4); /* Half height */
}

__attribute__((always_inline)) inline void DrawBlockRunHalf(LONG x, LONG y, UWORD block, WORD count, 
    UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
{
    UWORD halfplanelines = blockplanelines >> 1;
    UWORD src_skip = halfplanelines * blockbytesperrow;
    UWORD dst_skip = halfplanelines * BITMAPBYTESPERROW;
    
    x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3);

    UWORD mapx = (block & 15) << 1;
    UWORD mapy = (block >> 4) * (blockplanelines * blockbytesperrow);
    
    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytesperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
    custom->bltapt  = blocksbuffer + mapy + mapx + src_skip;
    custom->bltdpt  = dest + y + x + dst_skip;
    
    custom->bltsize = (halfplanelines << 6) + (BLOCKWIDTH >> 4);
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
    y = (y << 4) + (y << 3);  // y * 24

    // Assuming blocksperrow = 16
    UWORD mapx = (block & 15) << 1;                          // block % 16
    UWORD mapy = (block >> 4) * (blockplanelines * blockbytesperrow);  // block / 16
    
    
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
    custom->bltsize = (blockplanelines << 6) + (BLOCKWIDTH >> 4);
}
 
void Game_FillScreen(void)
{
    WORD a, b, x, y;
    WORD start_tile_y = mapposy / BLOCKHEIGHT;
    WORD rows = EFFECTIVE_HEIGHT / BLOCKHEIGHT;

    for (b = 0; b < rows; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * BLOCKPLANELINES;
            
            DrawBlock(x, y, a, start_tile_y + b, screen.bitplanes);
            DrawBlock(x, y, a, start_tile_y + b, screen.offscreen_bitplanes);
            DrawBlock(x, y, a, start_tile_y + b, screen.pristine);
            
#ifndef USE_YUNLIMITED2
            UWORD yoff = y + (HALFBITMAPHEIGHT * BLOCKSDEPTH);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.bitplanes);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.offscreen_bitplanes);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.pristine);
#endif
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

    Font_DrawStringCentered(draw_buffer, "       PUSH BUTTON", 60, 17);   
    Font_DrawStringCentered(draw_buffer, "       ONLY 1 PLAYER", 100, 17);  
            
 
}

void GameReady_Draw(void)
{
    // Handle blinking "PUSH BUTTON" text
    if (Timer_HasElapsed(&ready_blink_timer))
    {
        ready_text_visible = !ready_text_visible;
        
        if (ready_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "       PUSH BUTTON", 60, 17);   
            Font_DrawStringCentered(draw_buffer, "       ONLY 1 PLAYER", 100, 17);  
            WaitBlit();
        }
        else
        {
            // Clear the "PUSH BUTTON" area
            Font_ClearArea(draw_buffer, 0, 60, SCREENWIDTH, 8);
            WaitBlit();
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

        Game_ApplyPalette(current_palette,BLOCKSCOLORS);

        MotorBike_Reset();
 
        StageProgress_DrawAllEmpty();

        HUD_SetSpritePositions();
        HUD_DrawAll();

 
        Game_FillScreen();
        Road_CacheFillVisible();
        Game_SwapBuffers(); 

        Stage_ShowInfo();
        StageProgress_SetStage(0);
        Music_LoadModule(MUSIC_START);
    }    
 
}

void Stage_Initialize(void)
{

    Disk_LoadAsset((UBYTE *)city_colors,"tiles/lv1_tiles.PAL");

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
        Game_SwapBuffers();  
        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
    }
    else if (stage_state == STAGE_PLAYING)
    {
        SmoothScroll();
        Cars_Update();
        bike_world_y = mapposy + bike_position_y;

        //Stage_RedrawTunnelTiles();

        Game_SwapBuffers();
 
        if (Timer_HasElapsed(&hud_update_timer))
        {
        switch (hud_phase)
        {
            case 0:
                if (game_score != last_score)
                {
                    HUD_UpdateScore(game_score);
                    last_score = game_score;
                }
                break;
            case 1:
                if (game_rank != last_rank)
                {
                    HUD_UpdateRank(game_rank);
                    last_rank = game_rank;
                }
                break;
            case 2:
                if (bike_speed != last_speed)
                {
                    HUD_UpdateBikeSpeed(bike_speed);
                    last_speed = bike_speed;
                }
                break;
        }
        hud_phase++;
        if (hud_phase > 2) hud_phase = 0;
        Timer_Reset(&hud_update_timer);
        }

        Fuel_Draw(); 
        StageProgress_DrawOverhead();
        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
        Game_HandleCollisions();
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {       
        City_RestoreOncomingCars();
        City_DrawRoad();
        City_DrawOncomingCars();
        Planes_Restore(draw_buffer);
        Planes_Draw(draw_buffer);

        WORD vibration_x = MotorBike_GetVibrationOffset();
        MotorBike_Draw(bike_position_x + vibration_x, bike_position_y, 0);
        Planes_Update();
        
        WaitBlit();
 
        Game_ResetBitplanePointer();
 
        if (Timer_HasElapsed(&hud_update_timer))
        {
            if (bike_speed != last_speed)
            {
                HUD_UpdateBikeSpeed(bike_speed);
                last_speed = bike_speed;
            }
            Timer_Reset(&hud_update_timer);
        }

        Fuel_Draw(); 
        StageProgress_DrawFrontview();
      
    }
    else if (stage_state == STAGE_COMPLETE)
    {
        City_DrawRoad();
        Planes_Restore(draw_buffer);
        Planes_Update();
        Planes_Draw(draw_buffer);

        NYCVictory_Update();

        if (Timer_HasElapsed(&stage_complete_timer))
        {
            Planes_Stop();

            Timer_Stop(&stage_complete_timer);
            stage_state = STAGE_RANKING;

            // Turn off Bike
            Sprites_ClearLower();
            Sprites_ClearHigher();

            Game_ApplyPalette(current_palette,BLOCKSCOLORS);

            Ranking_Initialize();

            if (game_stage != STAGE_NEWYORK)
            {
                Music_Stop();
                WaitVBL();
                custom->dmacon = DMAF_SETCLR | DMAF_AUD0 | DMAF_AUD1 | DMAF_AUD2 | DMAF_AUD2 ;
                Music_LoadModule(MUSIC_RANKING);
            }
 

        }
    }
    else if (stage_state == STAGE_RANKING)
    {
        RankingState ranking_state = Ranking_GetState();

        switch (ranking_state)
        {
            case RANKING_STATE_DRAWHORIZON:
                BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
                break;
            case RANKING_STATE_SCROLLING:     
            case RANKING_STATE_FLASHING:     
            case RANKING_STATE_SOLID_RED:
                 BlitClearArea(draw_buffer, 0, 88, VIEWPORT_WIDTH, 200);
                 break;
            case RANKING_STATE_BONUS_DEPLETING: 
            case RANKING_STATE_COMPLETE:   

        }
 
        Ranking_Draw(draw_buffer);
        Game_ResetBitplanePointer();
    }
    else if (stage_state == STAGE_GAMEOVER)
    {
        if (gameover_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "GAME OVER", 120, 17); 
        }
        else
        {
            Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);
        }
        
        Game_ResetBitplanePointer();
    }
    else if (stage_state == STAGE_GAMEOVER_ENTRY)
    {
        NameEntry_Draw(draw_buffer);
        Game_ResetBitplanePointer();
    }   
}

static void CheckBonusPickups(void)
{
    UBYTE surface = Collision_GetSurface(bike_position_x + 8, bike_wy);
    UBYTE surface_l = Collision_GetSurface(bike_position_x, bike_wy);
    UBYTE surface_r = Collision_GetSurface(bike_position_x + 16, bike_wy);

    UWORD pickup_points = 0;
    UBYTE pickup_surface = SURFACE_NORMAL;
    
    if (surface_l >= SURFACE_PTS100 && surface_l <= SURFACE_PTS1000)
        pickup_surface = surface_l;
    else if (surface >= SURFACE_PTS100 && surface <= SURFACE_PTS1000)
        pickup_surface = surface;
    else if (surface_r >= SURFACE_PTS100 && surface_r <= SURFACE_PTS1000)
        pickup_surface = surface_r;
    
    if (pickup_surface != SURFACE_NORMAL)
    {
        switch (pickup_surface)
        {
            case SURFACE_PTS100:  pickup_points = 100;  break;
            case SURFACE_PTS200:  pickup_points = 200;  break;
            case SURFACE_PTS500:  pickup_points = 500;  break;
            case SURFACE_PTS700:  pickup_points = 700;  break;
            case SURFACE_PTS1000: pickup_points = 1000; break;
        }
        
        WORD hit_x = bike_position_x + 8;
        if (surface_l == pickup_surface) hit_x = bike_position_x;
        else if (surface_r == pickup_surface) hit_x = bike_position_x + 16;
        
        WORD map_x = hit_x >> 4;
        WORD map_y = bike_wy >> 4;
        
        game_score += pickup_points;
        HUD_UpdateScore(game_score);
        
        mapdata[map_y * mapwidth + map_x] = road_tile_plain;
        
        UBYTE old_col = Collision_Get(hit_x, bike_wy);
        Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
        
        WORD buf_y = ROUND2BLOCKHEIGHT((map_y << 4) % EFFECTIVE_HEIGHT) << 2;
        WORD buf_x = map_x << 4;
        
        DrawBlock(buf_x, buf_y, map_x, map_y, screen.bitplanes);
        DrawBlock(buf_x, buf_y, map_x, map_y, screen.offscreen_bitplanes);
        DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
        
        UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
        DrawBlock(buf_x, yoff, map_x, map_y, screen.bitplanes);
        DrawBlock(buf_x, yoff, map_x, map_y, screen.offscreen_bitplanes);
        DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);
    }
}

void Stage_Update()
{
    game_frame_count++;

    if (stage_state == STAGE_BEGIN)
    {
        if (game_stage == STAGE_LASVEGAS)
        {
            countdown_value = 4;
            Timer_Start(&countdown_timer, 1);
            stage_state = STAGE_COUNTDOWN;
        }
        else
        {
            /* Skip countdown — go straight to playing */
            stage_state = STAGE_PLAYING;
            bike_speed = MIN_CRUISING_SPEED;
            bike_state = BIKE_STATE_MOVING;
        }
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
                WaitVBL();
             
                custom->intena = INTF_INTEN;

                Sprites_SetPointers(current_countdown_spr, 2, SPRITEPTR_TWO_AND_THREE);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[0],96,120,32);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[1],96,120,32);

                custom->intena = INTF_SETCLR | INTF_INTEN;
            }
            else
            {
                WORD difficulty_y = videoposy + BLOCKHEIGHT + 72;
                WORD stage_y = videoposy + BLOCKHEIGHT + 128;

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

                Sprites_ClearLower();
                Sprites_ClearHigher();

                Music_LoadModule(MUSIC_ONROAD);
            }
        }
    }
    else if (stage_state == STAGE_PLAYING)
    {

        HUD_Update1UPFlash();

        speed_accumulator += bike_speed;
        speed_sample_count++;

        //#ifdef DEBUG
        /* Debug: press fire+up+down to skip to next stage */
        if (KeyPressed(KEY_F1))
        {
            stage_state = STAGE_FRONTVIEW;

            Stage_InitializeFrontView();
            HUD_UpdateBikeSpeed(bike_speed);
            HUD_UpdateScore(game_score);
            return;
        }
        //#endif

        // Update fuel gauge
        Fuel_Update();
        
        // Check if fuel is empty
        if (Fuel_IsEmpty())
        {
            // Slow bike to a stop
            bike_speed = 0;
            
            // Transition to game over
            stage_state = STAGE_GAMEOVER;
 
            // Turn off Bike
            Sprites_ClearLower();
            Sprites_ClearHigher();
            
            Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
            WaitVBL();

            BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

            Music_LoadModule(MUSIC_GAMEOVER); 

            return;
        }
        

        if (bike_invulnerable && Timer_HasElapsed(&invuln_timer))
        {
            bike_invulnerable = FALSE;
            Timer_Stop(&invuln_timer);
        }

        if (wheelie_active)
        {
            //bike_speed = wheelie_speed;
            //bike_state = BIKE_STATE_WHEELIE;

            // No skid if offroad over jump
            if (bike_state == BIKE_STATE_WHEELIE )
            {
                if ((game_frame_count & 7) == 0)
                    SFX_Play(SFX_SKID);
            }
            
            // Clear the landing path — push cars out of the way
          
            WORD bike_cx = bike_position_x + 8;
            
            for (int i = 0; i < MAX_CARS; i++)
            {
                if (!car[i].visible || car[i].off_screen || car[i].crashed) continue;
                
                WORD car_screen_y = car[i].y - mapposy;
                WORD car_cx = car[i].x + 16;
                WORD x_dist = ABS(bike_cx - car_cx);
                
                // Car is ahead of bike and in the flight path
                if (car_screen_y < bike_position_y && 
                    car_screen_y > bike_position_y - 80 && 
                    x_dist < 24)
                {
                    // Nudge car sideways out of the path
                    if (car_cx < bike_cx)
                        car[i].x -= 3;
                    else
                        car[i].x += 3;
                }
            }
            if (Timer_HasElapsed(&wheelie_timer))
            {
                wheelie_active = FALSE;
                wheelie_scored = FALSE;
                Timer_Stop(&wheelie_timer);
                bike_state = BIKE_STATE_MOVING;
                wheelie_anim_frames = 0;  // Reset animation counter
                Sprites_ClearHigher();     // Turn off sprites 2+3
                //SFX_Play(SFX_LAND);
            }

            speed_accumulator += bike_speed;
            speed_sample_count++;
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();

            if (bike_state == BIKE_STATE_JUMP )
            {
                CheckBonusPickups();
            }
            return;
        }
 
        
        if (collision_state == COLLISION_TRAFFIC || collision_state == COLLISION_OFFROAD)
        {
            if (bike_speed > 0)
            {
                bike_speed -= 4;
                if (bike_speed < 0) bike_speed = 0;
            }
            
            bike_state = BIKE_STATE_CRASHED;
                
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();
            return;
        }
 
        bike_wy = mapposy + bike_position_y - g_sprite_voffset;

        UBYTE surface = Collision_GetSurface(bike_position_x + 8, bike_wy);
        UBYTE surface_l = Collision_GetSurface(bike_position_x, bike_wy);
        UBYTE surface_r = Collision_GetSurface(bike_position_x + 16, bike_wy);
    
        if ((surface == SURFACE_WHEELIE  || 
            surface == SURFACE_JUMP ) && !wheelie_active)
        {
            wheelie_active = TRUE;
            wheelie_speed = 150;

            Timer_Start(&wheelie_timer,  surface == SURFACE_JUMP ? 1 : 2);
            
            bike_state = surface == SURFACE_JUMP  ? BIKE_STATE_JUMP : BIKE_STATE_WHEELIE;
            wheelie_anim_frames = 0;  // Reset animation counter
            
            if (!wheelie_scored)
            {
                game_score += 700;
                wheelie_scored = TRUE;
            }
            
            speed_accumulator += bike_speed;
            speed_sample_count++;
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();
            return;
        }
 

        UWORD pickup_points = 0;
        UBYTE pickup_surface = SURFACE_NORMAL;
        
        if (surface_l >= SURFACE_PTS100 && surface_l <= SURFACE_PTS1000)
            pickup_surface = surface_l;
        else if (surface >= SURFACE_PTS100 && surface <= SURFACE_PTS1000)
            pickup_surface = surface;
        else if (surface_r >= SURFACE_PTS100 && surface_r <= SURFACE_PTS1000)
            pickup_surface = surface_r;
        
        if (pickup_surface != SURFACE_NORMAL)
        {
            switch (pickup_surface)
            {
                case SURFACE_PTS100:  pickup_points = 100;  break;
                case SURFACE_PTS200:  pickup_points = 200;  break;
                case SURFACE_PTS500:  pickup_points = 500;  break;
                case SURFACE_PTS700:  pickup_points = 700;  break;
                case SURFACE_PTS1000: pickup_points = 1000; break;
            }
            
            /* Find which point hit */
            WORD hit_x = bike_position_x + 8;
            if (surface_l == pickup_surface) hit_x = bike_position_x;
            else if (surface_r == pickup_surface) hit_x = bike_position_x + 16;
            
            WORD map_x = hit_x >> 4;
            WORD map_y = bike_wy >> 4;
            /* Add score */
            game_score += pickup_points;
            HUD_UpdateScore(game_score);
            
            /* Replace tile with plain road */
            mapdata[map_y * mapwidth + map_x] = road_tile_plain;
            
            /* Preserve lane, clear surface */
            UBYTE old_col = Collision_Get(hit_x, bike_wy);
            Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
            
            /* Redraw on all buffers */
            WORD buf_y = ROUND2BLOCKHEIGHT((map_y * BLOCKHEIGHT) % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = map_x << 4;
            
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
            
            UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);
        }

        if (surface_l == SURFACE_GASCAN || surface == SURFACE_GASCAN || surface_r == SURFACE_GASCAN)
        {
            WORD hit_x = bike_position_x + 8;
            if (surface_l == SURFACE_GASCAN) hit_x = bike_position_x;
            else if (surface_r == SURFACE_GASCAN) hit_x = bike_position_x + 16;
            
            WORD map_x = hit_x >> 4;
            WORD map_y = bike_wy >> 4;
            
            Fuel_Add(1);
            Fuel_DrawAll();
            
            mapdata[map_y * mapwidth + map_x] = road_tile_plain;
            
            UBYTE old_col = Collision_Get(hit_x, bike_wy);
            Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
            
            WORD buf_y = ROUND2BLOCKHEIGHT((map_y << 4) % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = map_x << 4;
            
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
            
            UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);

            return;
        }

        if (surface == SURFACE_PUDDLE)
        {
           SFX_Play(SFX_SKID);
        }

        // === BRAKE — button 2 or pull down ===
        if (JoyButton2() || JoyDown())
        {
            bike_speed -= 8;  // Hard braking
            if (bike_speed < MIN_CRUISING_SPEED)
                bike_speed = MIN_CRUISING_SPEED;
            
            bike_state = BIKE_STATE_BRAKING;

            if ((game_frame_count & 7) == 0)
            {
                SFX_Play(SFX_BRAKE);
            }
        }

        // === ACCELERATION LOGIC ===

        if (JoyFireHeld())
        {
            // Fire button held - accelerate to max speed
            bike_speed += ACCEL_RATE;   
            if (bike_speed > max_stage_speed)
            {
                bike_speed = max_stage_speed;
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

         /* Clamp bike to road viewport */
        if (bike_position_x < 2)
            bike_position_x = 2;
        if (bike_position_x > SCREENWIDTH - 16)
            bike_position_x = SCREENWIDTH - 16;

        StageProgress_UpdateOverhead(mapposy);

        Stage_CheckCompletion();
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {
        HUD_Update1UPFlash();

         // Update fuel gauge
        Fuel_Update();
        
        // Check if fuel is empty
        if (Fuel_IsEmpty())
        {
            // Slow bike to a stop
            bike_speed = 0;
            
            // Transition to game over
            stage_state = STAGE_GAMEOVER;
          

            // Turn off Bike
            Sprites_ClearLower();
            Sprites_ClearHigher();

            Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
            WaitVBL();
  

            BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
 

            Music_LoadModule(MUSIC_GAMEOVER); 

            return;
        }

        if (City_OncomingCarsIsComplete())
        {
            stage_state = STAGE_COMPLETE;

            if (game_stage == STAGE_NEWYORK)
            {
                NYCVictory_Start();
                Timer_Start(&stage_complete_timer, 6);
            }
            else
            {
                Timer_Start(&stage_complete_timer, 2);
            }

            KPrintF("=== Stage %ld Complete! ===\n", game_stage);
            return;
        }

        CityApproachState approach_state = City_GetApproachState();
        static BikeFrame prev_bike_state = -1;
          
        if (frontview_bike_crashed == FALSE && approach_state < CITY_STATE_INTO_HORIZON )
        {
            static UBYTE frontview_bike_anim_frame = 0;   
   
            UWORD anim_speed = 15 - (bike_speed >> 4);  // Faster bike = smaller number
            if (anim_speed < 2) anim_speed = 2;  
 
            if (game_frame_count % anim_speed == 0)
            {
                frontview_bike_anim_frame ^= 1;
            }

            bike_state = frontview_bike_anim_frame ? BIKE_FRAME_APPROACH2 : BIKE_FRAME_APPROACH1;  

            if (JoyFireHeld())
            {
                // Fire button held - accelerate to max speed
                bike_speed += ACCEL_RATE;   
                if (bike_speed > max_stage_speed)
                {
                    bike_speed = max_stage_speed;
                }
                
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
                }
                else if (bike_speed > MIN_CRUISING_SPEED)
                {
                    // Decelerate back to cruising speed
                    bike_speed -= DECEL_RATE;
                    if (bike_speed < MIN_CRUISING_SPEED)
                    {
                        bike_speed = MIN_CRUISING_SPEED;
                    }
                   
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
            
        }

        if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            if (game_stage == STAGE_HOUSTON && !Planes_IsActive() && bike_position_y < 210)
            {
                Planes_Start();
            }
            else if (game_stage == STAGE_NEWYORK && !Planes_IsActive() && 
                !Planes_IsComplete() && bike_position_y < 210)
            {
                Planes_StartNYC();
            }

            City_UpdateHorizonTransition(&bike_position_y, &bike_speed, game_frame_count);

            bike_position_x = City_CalculateBikePerspectiveX(bike_position_y, 
                                                        City_GetBikeHorizonStartX());

            MotorBike_UpdateApproachFrame(bike_position_y);
        }
        else
        {
            // Normal gameplay - use the bike_state set above
            if (bike_state != prev_bike_state)
            {
                MotorBike_SetFrame(bike_state);
                    
                prev_bike_state = bike_state;
            }
        }

        // Update road scrolling
        if (approach_state == CITY_STATE_WAITING_NAME || approach_state == CITY_STATE_ACTIVE)
        {
            UpdateRoadScroll(bike_speed, 0);
        }
        else if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            UpdateRoadScroll(bike_speed, game_frame_count);
        }
    }
    else if (stage_state == STAGE_RANKING)
    {
        HUD_Show1UP();

        Ranking_Update();
    }
    else if (stage_state == STAGE_GAMEOVER)
    {
        HUD_Show1UP();

        /* First frame — set up */
        if (!Timer_IsActive(&gameover_timer))
        {
           
             /* Record best rank at moment of game over */
            Game_RecordBestRank();
            
            Timer_Start(&gameover_timer, 6);
            Timer_StartMs(&gameover_flash_timer, 300);
            gameover_text_visible = TRUE;
            
            /* Check high score qualification now */
            gameover_hiscore_pos = HiScore_CheckQualifies(game_score);

            Game_ApplyPalette(intro_colors,BLOCKSCOLORS);

        }
        
        /* Flash "GAME OVER" text */
        if (Timer_HasElapsed(&gameover_flash_timer))
        {
            gameover_text_visible = !gameover_text_visible;
            Timer_Reset(&gameover_flash_timer);
        }
        
        /* After 5 seconds */
        if (Timer_HasElapsed(&gameover_timer))
        {
            Timer_Stop(&gameover_timer);
            Timer_Stop(&gameover_flash_timer);
            
            if (gameover_hiscore_pos > 0)
            {
                /* Qualifies for high score — show name entry */
                stage_state = STAGE_GAMEOVER_ENTRY;
                
                Sprites_ClearLower();
                Sprites_ClearHigher();

                Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
                WaitVBL();

                BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
                
                NameEntry_Init(gameover_hiscore_pos);

            }
            else
            {
                /* Doesn't qualify — back to attract */
                Game_Reset();
            }
        }
    }
    else if (stage_state == STAGE_GAMEOVER_ENTRY)
    {
        Game_ApplyPalette((UWORD *)intro_colors, BLOCKSCOLORS);

        NameEntry_Update();
        
        UBYTE joy = 0;
        if (JoyUp()) joy |= 0x01;
        if (JoyDown()) joy |= 0x02;
        if (JoyLeft()) joy |= 0x04;
        if (JoyRight()) joy |= 0x08;
        NameEntry_HandleInput(joy, JoyFirePressed());
        
        if (NameEntry_IsComplete())
        {
            /* Insert into high score table */
            const char *name = NameEntry_GetName();
            HiScore_Insert(game_score, game_rank, name);
            
            /* Return to attract */
            Game_Reset();
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
    stage_text = " LOS ANGELES";
    
    WORD difficulty_y = videoposy + BLOCKHEIGHT + 72;
    WORD stage_y = videoposy + BLOCKHEIGHT + 128;

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
     if (collision_state == COLLISION_NONE)
    {
        int hit_car = -1;
        collision_state = MotorBike_CheckCollision(&hit_car);
        
       if (collision_state == COLLISION_TRAFFIC)
        {
            collision_car_index = hit_car;
            Timer_Start(&collision_recovery_timer, 3);
            Cars_HandleSpinout(hit_car);
            Music_Stop();
            crash_anim_frames = 0;
            crash_spin_frame = 0;
            SFX_Play(SFX_CRASHSKID);
           
 
        }
        else if (collision_state == COLLISION_OFFROAD)
        {
            Timer_Start(&collision_recovery_timer, 2);
            Music_Stop();
            crash_anim_frames = 0;
            crash_spin_frame = 0;
            SFX_Play(SFX_CRASHSKID);
        }
    }
    
    if (collision_state != COLLISION_NONE)
    {
        if (collision_state == COLLISION_TRAFFIC || 
            collision_state == COLLISION_OFFROAD)
        {
            bike_state = BIKE_STATE_CRASHED;
         
        }
 
        if (Timer_HasElapsed(&collision_recovery_timer))
        {
            MotorBike_CrashAndReposition();
            
            collision_state = COLLISION_NONE;
            collision_car_index = -1;
            Timer_Stop(&collision_recovery_timer);
         
            /* Restart per-stage music */
            switch (game_stage)
            {
                case STAGE_LASVEGAS:  Music_LoadModule(MUSIC_ONROAD); break;
                case STAGE_HOUSTON:   Music_LoadModule(MUSIC_OFFROAD); break;
                case STAGE_STLOUIS:   Music_LoadModule(MUSIC_ONROAD); break;
                case STAGE_CHICAGO:   Music_LoadModule(MUSIC_OFFROAD); break;
                case STAGE_NEWYORK:   Music_LoadModule(MUSIC_ONROAD); break;
                default:              Music_LoadModule(MUSIC_ONROAD); break;
            }

            Fuel_Decrease(1);
            Fuel_DrawAll();

            bike_state = BIKE_STATE_MOVING;
            bike_speed = MIN_CRUISING_SPEED;
        }
    }
 
}

void Stage_CheckCompletion(void)
{
    /* Complete when bike reaches near the top of the map */
    /* mapsize is the starting mapposy (bottom of map) */
    /* current_map_pos counts up from 0 to mapsize */
    LONG completion_threshold = stage_progress.mapsize - (BLOCKHEIGHT << 1);
    
    if (stage_progress.current_map_pos >= completion_threshold)
    {

        stage_state = STAGE_FRONTVIEW;

        Sprites_ClearLower();
        Sprites_ClearHigher();
        
        Stage_InitializeFrontView();
        HUD_UpdateBikeSpeed(bike_speed);
        HUD_UpdateScore(game_score);
    }
}

void Stage_InitializeFrontView(void)
{
    Game_ResetBitplanePointer();

    current_buffer = 0;
    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;

    Sprites_ClearLower();
    Sprites_ClearHigher();

    Game_ApplyPalette(black_palette, BLOCKSCOLORS);

    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
 
    bike_position_x = 80;
    bike_position_y = 200;
 
    /* Swap to city attract tiles for front view */
    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);

    City_ResetRoadState();

    /* Per-stage: map, skyline, city name */
    switch (game_stage)
    {
        case STAGE_LASVEGAS:
            Game_SetMap(STAGE1_FRONTVIEW);
            Skyline_Load(SKYLINE_VEGAS);
           
            break;
        case STAGE_HOUSTON:
            Game_SetMap(STAGE2_FRONTVIEW);
            Skyline_Load(SKYLINE_HOUSTON);
         
            break;
        case STAGE_STLOUIS:
            Game_SetMap(STAGE3_FRONTVIEW);
            Skyline_Load(SKYLINE_STLOUIS);
           
            break;
        case STAGE_CHICAGO:
            Game_SetMap(STAGE4_FRONTVIEW);
            Skyline_Load(SKYLINE_CHICAGO);
        
            break;
        case STAGE_NEWYORK:
            Game_SetMap(STAGE5_FRONTVIEW);
            Skyline_Load(SKYLINE_NYC);
          
            break;
        default:
            Game_SetMap(STAGE1_FRONTVIEW);
            Skyline_Load(SKYLINE_NYC);
           
            break;
    }
 
    Title_Reset();
 
    /* Reset horizon — clear stale restore state from previous stage */
    city_horizon->y = 0;
    city_horizon->old_x = 0;
    city_horizon->old_y = 0;
    city_horizon->prev_old_x = 0;
    city_horizon->prev_old_y = 0;
    city_horizon->needs_restore = FALSE;
    city_horizon->off_screen = FALSE;
    
    City_PreDrawRoad();

    City_OncomingCarsReset();
     
    Planes_ResetDone();

    switch (game_stage)
    {
        case STAGE_LASVEGAS:  City_ShowCityName("LAS VEGAS");  break;
        case STAGE_HOUSTON:   City_ShowCityName("HOUSTON");    break;
        case STAGE_STLOUIS:   City_ShowCityName("ST. LOUIS");  break;
        case STAGE_CHICAGO:   City_ShowCityName("CHICAGO");    break;
        case STAGE_NEWYORK:   City_ShowCityName("NEW YORK");   break;
        default:              City_ShowCityName("LAS VEGAS");  break;
    }

    
    Game_ApplyPalette(current_palette, BLOCKSCOLORS);
     
    MotorBike_Reset();
    MotorBike_ResetApproachIndex();

    HUD_DrawAll();
    HUD_UpdateScore(game_score);
 
    Music_Stop();
    Music_LoadModule(MUSIC_FRONTVIEW);

    Game_SetBackGroundColor(0x00);
}

void Stage_RedrawTunnelTiles(void)
{
    WORD start_row = mapposy >> 4;          /* / BLOCKHEIGHT (16) */
    WORD end_row = (mapposy + SCREENHEIGHT) >> 4;
    
    if (start_row < 0) start_row = 0;
    if (++end_row > col_map_height) end_row = col_map_height;
    
    /* Pointer to collision map row */
    UBYTE *col_row = collision_map + (start_row * col_map_width);
    WORD row_y = start_row << 4;            /* row * BLOCKHEIGHT */
    
    for (WORD row = start_row; row < end_row; row++)
    {
        /* Quick scan: any tunnel tiles in this row? */
        BOOL has_tunnel = FALSE;
        for (WORD col = 0; col < col_map_width; col++)
        {
            if ((col_row[col] & 0x0F) == SURFACE_TUNNEL)
            {
                has_tunnel = TRUE;
                break;
            }
        }
        
        if (has_tunnel)
        {
            WORD buf_y = ROUND2BLOCKHEIGHT(row_y % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = 0;
            
            for (WORD col = 0; col < col_map_width; col++)
            {
                if ((col_row[col] & 0x0F) == SURFACE_TUNNEL)
                {
                    DrawBlock(buf_x, buf_y, col, row, draw_buffer);
                }
                buf_x += 16;
            }
        }
        
        col_row += col_map_width;
        row_y += 16;
    }
}

void Game_RecordBestRank(void)
{
    if (game_rank < todays_best_rank)
        todays_best_rank = game_rank;
}

UBYTE Game_GetTodaysBestRank(void)
{
    return todays_best_rank;
}