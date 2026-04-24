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
#include "timers.h"
#include "title.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "planes.h"
#include "disk.h"
#include "font.h"
#include "audio.h"
#include "fuel.h"
#include "stageprogress.h"
#include "city_approach.h"

extern volatile struct Custom *custom;

static BYTE last_road_tile_idx = -1;
static UBYTE road_draw_count = 0;


static UBYTE *vivany_data = NULL;
static UBYTE *liberty_data_f1 = NULL;
static UBYTE *liberty_data_f2 = NULL;

static BOOL  victory_active = FALSE;
static BOOL  vivany_blitted = FALSE;
static UBYTE liberty_frame = 0;
static GameTimer liberty_anim_timer;


// Starting Y positions for each scale (horizon to foreground)
static const WORD scale_start_y[NUM_CAR_SCALES] = {
    42,   // Scale 0 starts
    56,   // Scale 1 at Y=60
    72,   // Scale 2 at Y=70
    92,   // Scale 3 at Y=80
    114,   // Scale 4 at Y=95
    124,  // Scale 5 at Y=115
    135,  // Scale 6 at Y=140
    150   // Scale 7 at Y=170
};

#
static CityApproachState city_state = CITY_STATE_WAITING_NAME;

static WORD cars_passed = 0;
static BOOL city_name_complete = FALSE;
static UBYTE crash_anim_frame = 0;
static UWORD crash_anim_counter = 0;
static UWORD skyline_anim_frame = 0;

static GameTimer spawn_timer;
static GameTimer city_name_timer;
static GameTimer city_skyline_anim_timer;
static GameTimer crash_recovery_timer;

static WORD bike_horizon_start_x = 0;  

BlitterObject attract_horizon;
BlitterObject nyc_horizon;
BlitterObject lv_horizon;
BlitterObject houston_horizon;
BlitterObject stl_horizon;
BlitterObject chi_horizon;
BlitterObject oncoming_car[8];
BlitterObject flipped_car[4];
BlitterObject *city_horizon;
BlitterObject *current_car;

 
static UBYTE *attract_skyline_fast = NULL;
static UBYTE *lv_skyline_fast = NULL;
static UBYTE *lv_skyline2_fast = NULL;
static UBYTE *houston_skyline_fast = NULL;
static UBYTE *stl_skyline_fast = NULL;
static UBYTE *chi_skyline_fast = NULL;
static UBYTE *nyc_skyline_fast = NULL;

static ULONG attract_skyline_size = 0;
static ULONG lv_skyline_size = 0;
static ULONG lv_skyline2_size = 0;
static ULONG houston_skyline_size = 0;
static ULONG stl_skyline_size = 0;
static ULONG chi_skyline_size = 0;
static ULONG nyc_skyline_size = 0;

// Shared chip buffers
static UBYTE *skyline_chip_buffer = NULL;
static UBYTE *skyline_chip_buffer2 = NULL;
static ULONG  skyline_chip_size = 0;
 
APTR oncoming_car_restore_ptrs[4];
APTR *oncoming_car_restore_ptr;

BOOL frontview_bike_crashed = FALSE;
BOOL use_alt_frame = FALSE;

void City_Initialize()
{
 
    // Scaled Oncoming Car Bobs
    oncoming_car[0].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_1, MEMF_CHIP);  // Smallest
    oncoming_car[1].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_2, MEMF_CHIP);
    oncoming_car[2].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_3, MEMF_CHIP);
    oncoming_car[3].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_4, MEMF_CHIP);
    oncoming_car[4].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_5, MEMF_CHIP);
    oncoming_car[5].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_6, MEMF_CHIP);
    oncoming_car[6].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_7, MEMF_CHIP);
    oncoming_car[7].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_8, MEMF_CHIP);
 
    oncoming_car[0].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[1].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[2].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[3].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[4].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[5].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[6].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    oncoming_car[7].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);

    // flipped cars

    flipped_car[0].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_5_RIGHT, MEMF_CHIP);
    flipped_car[1].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_6_RIGHT, MEMF_CHIP);
    flipped_car[2].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_7_RIGHT, MEMF_CHIP);
    flipped_car[3].data = Disk_AllocAndLoadAsset(ONCOMING_CAR_8_RIGHT, MEMF_CHIP);

    flipped_car[0].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    flipped_car[1].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    flipped_car[2].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);
    flipped_car[3].background = Mem_AllocChip((ONCOMING_CAR_WIDTH_WORDS * 2) * ONCOMING_CAR_HEIGHT * 4);

    City_InitializeSkylines();
}

void City_InitializeSkylines(void)
{
    // Get sizes
    attract_skyline_size = findSize(ATTRACT_SKYLINE);
    lv_skyline_size = findSize(VEGAS_SKYLINE);
    lv_skyline2_size = findSize(VEGAS_SKYLINE2);
    houston_skyline_size = findSize(HOUSTON_SKYLINE);
    stl_skyline_size = findSize(STL_SKYLINE);
    chi_skyline_size = findSize(CHI_SKYLINE);
    nyc_skyline_size = findSize(NYC_SKYLINE);
    
    // Load all to fast RAM
    attract_skyline_fast = Disk_AllocAndLoadAsset(ATTRACT_SKYLINE, MEMF_ANY);
    lv_skyline_fast = Disk_AllocAndLoadAsset(VEGAS_SKYLINE, MEMF_ANY);
    lv_skyline2_fast = Disk_AllocAndLoadAsset(VEGAS_SKYLINE2, MEMF_ANY);
    houston_skyline_fast = Disk_AllocAndLoadAsset(HOUSTON_SKYLINE, MEMF_ANY);
    stl_skyline_fast = Disk_AllocAndLoadAsset(STL_SKYLINE, MEMF_ANY);
    chi_skyline_fast = Disk_AllocAndLoadAsset(CHI_SKYLINE, MEMF_ANY);
    nyc_skyline_fast = Disk_AllocAndLoadAsset(NYC_SKYLINE, MEMF_ANY);
    
    // Find largest for each buffer
    skyline_chip_size = attract_skyline_size;
    if (lv_skyline_size > skyline_chip_size) skyline_chip_size = lv_skyline_size;
    if (houston_skyline_size > skyline_chip_size) skyline_chip_size = houston_skyline_size;
    if (stl_skyline_size > skyline_chip_size) skyline_chip_size = stl_skyline_size;
    if (chi_skyline_size > skyline_chip_size) skyline_chip_size = chi_skyline_size;
    if (nyc_skyline_size > skyline_chip_size) skyline_chip_size = nyc_skyline_size;
    
    ULONG chip_size2 = lv_skyline2_size;  // Only Vegas uses frame2
    
    skyline_chip_buffer = Mem_AllocChip(skyline_chip_size);
    skyline_chip_buffer2 = Mem_AllocChip(chip_size2);
    
    // Set up horizon structs
    attract_horizon.visible = TRUE;
    attract_horizon.off_screen = FALSE;
    attract_horizon.x = 0;
    attract_horizon.y = 0;
    attract_horizon.size = attract_skyline_size;
    attract_horizon.data = skyline_chip_buffer;
    attract_horizon.data_frame2 = NULL;
    
    lv_horizon.visible = TRUE;
    lv_horizon.off_screen = FALSE;
    lv_horizon.x = 0;
    lv_horizon.y = 0;
    lv_horizon.size = lv_skyline_size;
    lv_horizon.data = skyline_chip_buffer;
    lv_horizon.data_frame2 = skyline_chip_buffer2;
    
    houston_horizon.visible = TRUE;
    houston_horizon.off_screen = FALSE;
    houston_horizon.x = 0;
    houston_horizon.y = 0;
    houston_horizon.size = houston_skyline_size;
    houston_horizon.data = skyline_chip_buffer;
    houston_horizon.data_frame2 = NULL;

    stl_horizon.visible = TRUE;
    stl_horizon.off_screen = FALSE;
    stl_horizon.x = 0;
    stl_horizon.y = 0;
    stl_horizon.size = houston_skyline_size;
    stl_horizon.data = skyline_chip_buffer;
    stl_horizon.data_frame2 = NULL;   

    chi_horizon.visible = TRUE;
    chi_horizon.off_screen = FALSE;
    chi_horizon.x = 0;
    chi_horizon.y = 0;
    chi_horizon.size = chi_skyline_size;
    chi_horizon.data = skyline_chip_buffer;
    chi_horizon.data_frame2 = NULL;      

    nyc_horizon.visible = TRUE;
    nyc_horizon.off_screen = FALSE;
    nyc_horizon.x = 0;
    nyc_horizon.y = 0;
    nyc_horizon.size = nyc_skyline_size;
    nyc_horizon.data = skyline_chip_buffer;
    nyc_horizon.data_frame2 = NULL;
    
    // Load default
    Skyline_Load(SKYLINE_ATTRACT);
    city_horizon = &nyc_horizon;
    
    KPrintF("SkylinePool: chip buffer=%ld + %ld bytes\n", skyline_chip_size, chip_size2);
}

void Skyline_Load(UBYTE skyline_id)
{
    ULONG *src;
    ULONG *dst;
    ULONG longs;
    
    switch (skyline_id)
    {
        case SKYLINE_ATTRACT:
            src = (ULONG *)attract_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (attract_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &attract_horizon;
            break;
            
        case SKYLINE_VEGAS:
            src = (ULONG *)lv_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (lv_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            
            src = (ULONG *)lv_skyline2_fast;
            dst = (ULONG *)skyline_chip_buffer2;
            longs = (lv_skyline2_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &lv_horizon;
            break;
            
        case SKYLINE_HOUSTON:
            src = (ULONG *)houston_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (houston_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &houston_horizon;
            break;

        case SKYLINE_STLOUIS:
            src = (ULONG *)stl_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (stl_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &stl_horizon;
            break;       
        case SKYLINE_CHICAGO:
            src = (ULONG *)chi_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (chi_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &chi_horizon;
            break;     
        case SKYLINE_NYC:
            src = (ULONG *)nyc_skyline_fast;
            dst = (ULONG *)skyline_chip_buffer;
            longs = (nyc_skyline_size + 3) >> 2;
            while (longs--) *dst++ = *src++;
            city_horizon = &nyc_horizon;
            break;                       
    }
}

void City_OncomingCarsReset()
{
    cars_passed = 0;
    city_name_complete = FALSE;
    city_state = CITY_STATE_WAITING_NAME;
    frontview_bike_crashed = FALSE;
    
    Timer_Stop(&spawn_timer);
    Timer_Stop(&crash_recovery_timer);

}

void City_ResetRoadState(void)
{
    last_road_tile_idx = -1;
    road_draw_count = 0;
    use_alt_frame = FALSE;
}

void City_BlitHorizon()
{
    // Position
    WORD x = city_horizon->x;
    WORD y = city_horizon->y;

    UWORD source_mod = 0; 
    UWORD dest_mod = (SCREENWIDTH - CITYSKYLINE_WIDTH) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((CITYSKYLINE_HEIGHT << 2) << 6) | CITYSKYLINE_WIDTH / 16;
    
    // Source data

    UBYTE *source;

    if (use_alt_frame == TRUE)
    {
        source = (UBYTE*)&city_horizon->data_frame2[0];
    }
    else
    {
        source = (UBYTE*)&city_horizon->data[0];
    }
   
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, screen.bitplanes);
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, screen.offscreen_bitplanes);

    if (stage_state < STAGE_RANKING)
    {
        BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, screen.pristine);
    }
}


void City_DrawRoad()
{
    if (road_tile_idx != last_road_tile_idx)
    {
        last_road_tile_idx = road_tile_idx;
        road_draw_count = 2;
    }
    
    if (road_draw_count == 0)
        return;
    
    road_draw_count--;

    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;

    UWORD tile_offset = road_tile_idx << 5;
    UWORD *row_ptr = mapdata;       /* Include row 0 */
    WORD y = 0;                      /* Start at top */

    for (WORD b = 0; b < 10; b++)
    {
        BOOL has_animated = FALSE;
        for (WORD c = 0; c < 12; c++)
        {
            if (row_ptr[c] >= 32) { has_animated = TRUE; break; }
        }
        
        if (!has_animated && b > 0)  /* Never skip row 0 */
        {
            row_ptr += mapwidth;
            y += 64;
            continue;
        }

        WORD a = 0;
        while (a < 12)
        {
            UWORD block = row_ptr[a];
            WORD run_length = 1;

            if (block >= 32 || (b == 0 && block > 0))  /* Row 0: draw any non-empty tile */
            {
                if (block >= 32)
                    block += tile_offset;

                WORD check = a + 1;
                while (check < 12)
                {
                    UWORD next_block = row_ptr[check];
                    if (next_block >= 32) next_block += tile_offset;
                    if (next_block != block) break;
                    check++;
                }
                run_length = check - a;
            
                WORD x = a << 4;
                
                if (b == 0)
                {
                    DrawBlockRunHalf(x, y + ATTRACTMODE_Y_OFFSET, block, run_length,
                                    blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
                }
                else
                {
                    DrawBlockRun(x, y + ATTRACTMODE_Y_OFFSET, block, run_length, 
                                blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
                }
            }

            a += run_length;
        }
        
        row_ptr += mapwidth;
        y += 64;
    }

    if (game_map == STAGE1_FRONTVIEW)
    {
        if (Timer_HasElapsed(&city_skyline_anim_timer))
        {
            use_alt_frame = !use_alt_frame;
            Timer_Reset(&city_skyline_anim_timer);
            City_BlitHorizon();
        }
    }   
}
void City_PreDrawRoad()
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;
    WORD a, b, x, y;
    WORD tmpa, tmpb;
    
    // Enable DMA ONCE at start
    custom->dmacon = 0x8400;
 
    for (b = 0; b < 13; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * blockplanelines;
            
            tmpa = a;
            tmpb = b;

            if (tmpb > 10)
            {
                tmpa = 2;
                tmpb = 10;
            }
 
            if (b == 0)
            {
                DrawBlocksHalf(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.bitplanes); 
                DrawBlocksHalf(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.offscreen_bitplanes); 
                DrawBlocksHalf(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.pristine);                     
            }
            else
            {
                DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.bitplanes); 
                DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.offscreen_bitplanes); 
                DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                        blockplanelines, FALSE, road_tile_idx, screen.pristine);                     
            }
        }
    }

    // Blit the Skyline
    City_BlitHorizon();
    
    // Disable DMA ONCE at end
    custom->dmacon = 0x0400;

    Timer_StartMs(&city_skyline_anim_timer,400);
}

void City_SpawnOncomingCar(void)
{
    current_car = &oncoming_car[0];
    current_car->visible = TRUE;
    current_car->y = scale_start_y[0];

    current_car->id = 0;  // Start at smallest scale
    current_car->y = scale_start_y[0];  // Start at first Y position
    current_car->anim_counter = bike_position_x;

    current_car->x = HORIZON_VANISHING_X;
    current_car->speed = 1;
    current_car->needs_restore = FALSE;

    current_car->old_x = current_car->x;
    current_car->old_y = current_car->y;
    current_car->prev_old_x = current_car->x;
    current_car->prev_old_y = current_car->y;

    current_car->moved = (bike_position_x > 96);
    
    if (cars_passed & 1)
    {
        Copper_SetPalette(12, 0x0F0);
    }
    else 
    {
        Copper_SetPalette(12, 0xD00);
    }
}
 

void City_DrawOncomingCar(BlitterObject *car)
{
    if (!car->visible) return;

    UBYTE *source = (car->id >= 4 && car->moved) 
        ? (UBYTE*)&flipped_car[car->id - 4].data[0]
        : (UBYTE*)&oncoming_car[car->id].data[0];
 
    UBYTE *mask = source + CAR_SOURCE_MOD;
 
    BlitBob2(SCREEN_WIDTH_WORDS, car->x, car->y, CAR_ADMOD, CAR_BLTSIZE, 
             ONCOMING_CAR_WIDTH, oncoming_car_restore_ptrs, source, mask, draw_buffer);

    WaitBlit();

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    car->old_x = car->x;
    car->old_y = car->y;
    car->needs_restore = TRUE;
}

void City_DrawOncomingCars(void)
{
    // Wait for city name to finish
    if (city_state == CITY_STATE_WAITING_NAME)
    {
        if (City_IsCityNameComplete())
        {
            City_SpawnOncomingCar();
        }
        return;
    }
    
    // Active car spawning/passing
    if (city_state == CITY_STATE_ACTIVE)
    {
        if (current_car->visible)
        {
            current_car->y += current_car->speed;
            current_car->x = City_CalculatePerspectiveX(current_car->y, current_car->anim_counter);
      
            if (!frontview_bike_crashed && City_CheckCarCollision(current_car))
            {
              
                frontview_bike_crashed = TRUE;
                bike_speed = 20;  // Drop to near zero
                crash_anim_frame = 0;
                crash_anim_counter = 0;
                MotorBike_SetFrame(BIKE_FRAME_CRASH1);  
                Music_Stop();
                SFX_Play(SFX_CRASHSKID);
             
                Timer_Start(&crash_recovery_timer, 2);  // 2 second recovery
            }

            if (current_car->id < NUM_CAR_SCALES - 1)
            {
                if (current_car->y >= scale_start_y[current_car->id + 1])
                {
                    current_car->id++;
                    current_car->speed++;
                }
            }
            
            if (current_car->id >= NUM_CAR_SCALES - 1 && current_car->y > 256+32)
            {
                current_car->visible = FALSE;
                cars_passed++;
 
                StageProgress_UpdateFrontview(cars_passed, TOTAL_CARS_TO_PASS);

                KPrintF("Car passed! Total: %ld/%ld\n", cars_passed, TOTAL_CARS_TO_PASS);
                
                if (cars_passed >= TOTAL_CARS_TO_PASS)
                {
                    Timer_Start(&spawn_timer, CAR_SPAWN_TIMER+2);
                }
                else
                {
                    if (frontview_bike_crashed == FALSE)
                    {
                        if (fuel_alarm_active == FALSE)
                        {
                            SFX_Play(SFX_FRONTVIEWOVERTAKE);
                        }
                        Timer_Start(&spawn_timer, CAR_SPAWN_TIMER);
                    }
                }
                
            }
            
            City_DrawOncomingCar(current_car);
        }
        else
        {
            if (cars_passed >= TOTAL_CARS_TO_PASS)
            {
                // Last car already passed, waiting for timer
                if (Timer_HasElapsed(&spawn_timer))
                {
                    Timer_Stop(&spawn_timer);
                    
                    city_state = CITY_STATE_INTO_HORIZON;
                    bike_horizon_start_x = bike_position_x;
                    bike_state = BIKE_FRAME_APPROACH1;
                }
            }
            else
            {
                // Normal spawn - not the last car yet
                if (!frontview_bike_crashed && Timer_HasElapsed(&spawn_timer))
                {
                    Timer_Stop(&spawn_timer);
                    City_SpawnOncomingCar();
                }
            }
        }
    }
    // Horizon transition - bike only, no cars
    else if (city_state == CITY_STATE_INTO_HORIZON)
    {
        // Bike moves into horizon - handled in City_UpdateHorizonTransition
    }

    City_UpdateBikeCrashAnimation();

    if (frontview_bike_crashed && Timer_HasElapsed(&crash_recovery_timer))
    {
        frontview_bike_crashed = FALSE;
        Timer_Stop(&crash_recovery_timer);
        Game_SetBackGroundColor(0x000);
        Fuel_Decrease(1);
        Fuel_DrawAll();
        Music_LoadModule(MUSIC_FRONTVIEW);

        MotorBike_SetFrame(BIKE_FRAME_APPROACH1);

        if (!current_car->visible && cars_passed < TOTAL_CARS_TO_PASS)
        {
            Timer_Start(&spawn_timer, CAR_SPAWN_TIMER);
        }
    }
}

void City_UpdateHorizonTransition(WORD *bike_y, WORD *bikespeed, UWORD frame_count)
{
    if (city_state != CITY_STATE_INTO_HORIZON) return;
   

    if (*bikespeed > 80)
    {
        *bikespeed -= 1;
    }
 
    if (*bikespeed < 80)
    {
        *bikespeed = 80;
    }
 
    UWORD y_speed;
    
    if (*bikespeed > 150)
        y_speed = 2;
    else if (*bikespeed > 100)
        y_speed = 2;
    else
        y_speed = 2;
    
    if (frame_count % y_speed == 0)
    {
        (*bike_y)--;
    }
 
    if (*bike_y <= 84)
    {
        if (game_stage == STAGE_NEWYORK && Planes_IsActive())
        {
            *bike_y = 84;  
            return;
        }

        Music_Stop();
 
        city_state = CITY_STATE_COMPLETE;

        Sprites_ClearLower();
        Sprites_ClearHigher();

        LONG vpos = VBeamPos();
        while (VBeamPos() - vpos < 4) ;   // brief settle
        
        if (game_stage == STAGE_NEWYORK)
        {
            Music_LoadModule(MUSIC_VIVA_NY);
        }
        else
        {
            Music_LoadModule(MUSIC_CHECKPOINT);
        }
    }
}

BOOL City_OncomingCarsIsComplete(void)
{
     return (city_state == CITY_STATE_COMPLETE);
}

CityApproachState City_GetApproachState(void)
{
    return city_state;
}

void City_RestoreOncomingCars(void)
{
    if (current_car && current_car->visible && current_car->needs_restore)
    {
         
        WORD temp_x = current_car->old_x;
        WORD temp_y = current_car->old_y;
        
        current_car->old_x = current_car->prev_old_x;
        current_car->old_y = current_car->prev_old_y;

        City_CopyPristineBackground(current_car);
        
        WaitBlit();
        
        current_car->old_x = temp_x;
        current_car->old_y = temp_y;
    }
}

void City_CopyPristineBackground(BlitterObject *car)
{
    if (!car->needs_restore) return;

    ULONG y_offset = ((ULONG)car->prev_old_y << 6) + ((ULONG)car->prev_old_y << 5);
    WORD x_word_aligned = (car->old_x >> 3) & 0xFFFE;
    
    UBYTE *pristine_ptr = screen.pristine + y_offset + x_word_aligned;
    UBYTE *screen_ptr = draw_buffer + y_offset + x_word_aligned;
    
    #define CAR_BLTSIZE_RESTORE 8196   

    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
 
 
    custom->bltamod = 16;
    custom->bltdmod = 16;
    
    custom->bltapt = pristine_ptr;
    custom->bltdpt = screen_ptr;
 
    custom->bltsize = CAR_BLTSIZE_RESTORE;
}
 
void City_ShowCityName(const char *city_name)
{
    WORD name_y = g_is_pal ? 220 : 200;

    // Draw city name on all buffers
    Font_DrawStringCentered(screen.bitplanes, (char*)city_name, name_y, 7);  
    Font_DrawStringCentered(screen.offscreen_bitplanes, (char*)city_name, name_y, 7);
    
    // Start 3-second timer
    Timer_Start(&city_name_timer,3);
    city_name_complete = FALSE;

}

BOOL City_IsCityNameComplete(void)
{
    if (city_name_complete) return TRUE;

    if (Timer_HasElapsed(&city_name_timer))
    {
        Timer_Stop(&city_name_timer);
        city_name_complete = TRUE;
        
        // Clear text by restoring road from pristine
        UWORD text_width = Font_MeasureText("LAS VEGAS");
        UWORD x = (VIEWPORT_WIDTH - text_width) >> 1;
        x &= 0xFFF8;

        WORD name_y = g_is_pal ? 220 : 200;
        
        Font_RestoreFromPristine(screen.bitplanes, x, name_y, text_width, CHAR_HEIGHT);
        Font_RestoreFromPristine(screen.offscreen_bitplanes, x, name_y, text_width, CHAR_HEIGHT);
        
        city_state = CITY_STATE_ACTIVE;   
        
        KPrintF("City name complete - starting cars\n");
        return TRUE;
    }

    return FALSE;
}

WORD City_CalculatePerspectiveX(WORD car_y, WORD target_x)
{
    WORD car_progress_y = (car_y + 16) - HORIZON_Y ;   
    
    if (car_progress_y <= 0) return HORIZON_VANISHING_X;
 
    WORD delta_x = target_x - HORIZON_VANISHING_X;
 
    LONG x = HORIZON_VANISHING_X + (((LONG)delta_x * car_progress_y * 378) >> 16);
    
    return (WORD)x;
}

BOOL City_CheckCarCollision(BlitterObject *car)
{
    if (car->id < 6) return FALSE;
    
    // Pre-calculate constants 
    #define CAR_WIDTH 48
    #define CAR_HALF_WIDTH 24  // 48 >> 1
    #define BIKE_WIDTH 32
    #define BIKE_HALF_WIDTH 16  // 32 >> 1
    #define BIKE_HEIGHT 32
    
    // Use shifts instead of division
    WORD car_left = car->x - CAR_HALF_WIDTH;
    WORD car_right = car->x + CAR_HALF_WIDTH;
    WORD car_bottom = car->y + ONCOMING_CAR_HEIGHT;
    
    WORD bike_left = bike_position_x - BIKE_HALF_WIDTH;
    WORD bike_right = bike_position_x + BIKE_HALF_WIDTH;
    WORD bike_bottom = bike_position_y + BIKE_HEIGHT;
    
    // Combined early exit - most collisions fail on X
    if (car_right <= bike_left || car_left >= bike_right)
        return FALSE;
    
    // Then check Y
    return (car_bottom >= bike_position_y) && (car->y < bike_bottom);
}

void City_UpdateBikeCrashAnimation(void)
{
    if (!frontview_bike_crashed) return;
    
    crash_anim_counter++;
    
  
    if (crash_anim_counter % 8 == 0)
    {
        crash_anim_frame++;
 
        if (crash_anim_frame > 8)
            crash_anim_frame = 0;   
 
        switch(crash_anim_frame)
        {
            case 0: bike_state = BIKE_FRAME_CRASH1; break;
            case 1: bike_state = BIKE_FRAME_CRASH2; break;
            case 2: bike_state = BIKE_FRAME_CRASH3; break;
            case 3: bike_state = BIKE_FRAME_CRASH4; break;
        }
    }
}

// Calculate bike X position as it moves into horizon
 
WORD City_CalculateBikePerspectiveX(WORD bike_y, WORD starting_x)
{
    WORD HORIZON_CENTER_X = 81;
    const WORD BIKE_START_Y = 200;
    const WORD HORIZON_END_Y = 92;

    if (bike_y <= HORIZON_END_Y) return HORIZON_CENTER_X;
    
    if (bike_y >= BIKE_START_Y) return starting_x;
 
    WORD distance_traveled = BIKE_START_Y - bike_y;
    WORD total_distance = BIKE_START_Y - HORIZON_END_Y;  
 
    WORD delta_x = HORIZON_CENTER_X - starting_x;
 
    LONG x = starting_x + ((LONG)delta_x * distance_traveled) / total_distance;
    
    return (WORD)x;
}

WORD City_GetBikeHorizonStartX(void)
{
    return bike_horizon_start_x;
}


void NYCVictory_Initialize(void)
{
    vivany_data    = Disk_AllocAndLoadAsset(VIVA_NY, MEMF_CHIP);
    liberty_data_f1 = Disk_AllocAndLoadAsset(LIBERTY_ANIM1, MEMF_CHIP);
    liberty_data_f2 = Disk_AllocAndLoadAsset(LIBERTY_ANIM2, MEMF_CHIP);
}

void NYCVictory_Start(void)
{
    victory_active = TRUE;
    vivany_blitted = FALSE;
    liberty_frame = 0;
    Timer_StartMs(&liberty_anim_timer, 400);
}

void NYCVictory_Stop(void)
{
    victory_active = FALSE;
    Timer_Stop(&liberty_anim_timer);
}

BOOL NYCVictory_IsActive(void)
{
    return victory_active;
}

void NYCVictory_Update(void)
{
    if (!victory_active) return;
    
    /* ---- Blit "VIVA! NY" once onto the skyline BOB data ---- */
    if (!vivany_blitted && vivany_data)
    {
        /* Straight copy onto the skyline chip buffer — no cookie cutter.
         * We blit directly into the skyline BOB data so it persists
         * without needing per-frame redraws.
         *
         * Skyline is 192px wide = 24 bytes/plane line
         * VIVA!NY is 96px wide = 12 bytes/plane line
         */
        UBYTE *dst = city_horizon->data;
        UBYTE *src = vivany_data;
        
        UWORD sky_bytes_per_row = (192 / 8);          /* 24 */
        UWORD viva_bytes_per_row = (VIVANY_WIDTH / 8); /* 12 */
        UWORD dst_x_byte = VIVANY_X / 8;
        
        /* For each plane line (height * depth = 16 * 4 = 64 lines) */
        UWORD src_mod = 0;
        UWORD dst_mod = sky_bytes_per_row - viva_bytes_per_row;
        
        ULONG dst_offset = (ULONG)VIVANY_Y * sky_bytes_per_row * VIVANY_DEPTH + dst_x_byte;
        
        WaitBlit();
        custom->bltcon0 = 0x9F0;      /* D = A (straight copy) */
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = src_mod;
        custom->bltdmod = dst_mod;
        custom->bltapt  = src;
        custom->bltdpt  = dst + dst_offset;
        custom->bltsize = ((VIVANY_HEIGHT * VIVANY_DEPTH) << 6) | (viva_bytes_per_row >> 1);
        
        vivany_blitted = TRUE;

        City_BlitHorizon();
    }
    
    /* ---- Animate liberty flame ---- */
    if (Timer_HasElapsed(&liberty_anim_timer))
    {
        liberty_frame ^= 1;
        Timer_Reset(&liberty_anim_timer);
        
        /* Blit flame frame directly into skyline BOB data */
        UBYTE *src = (liberty_frame == 0) ? liberty_data_f1 : liberty_data_f2;
        UBYTE *dst = city_horizon->data;
        
        UWORD sky_bytes_per_row = (192 / 8);             /* 24 */
        UWORD lib_bytes_per_row = (LIBERTY_WIDTH / 8);    /* 2 */
        UWORD dst_x_byte = LIBERTY_X / 8;
        
        UWORD src_mod = 0;
        UWORD dst_mod = sky_bytes_per_row - lib_bytes_per_row;
        
        ULONG dst_offset = (ULONG)LIBERTY_Y * sky_bytes_per_row * LIBERTY_DEPTH + dst_x_byte;
        
        WaitBlit();
        custom->bltcon0 = 0x9F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = src_mod;
        custom->bltdmod = dst_mod;
        custom->bltapt  = src;
        custom->bltdpt  = dst + dst_offset;
        custom->bltsize = ((LIBERTY_HEIGHT * LIBERTY_DEPTH) << 6) | (lib_bytes_per_row >> 1);

        City_BlitHorizon();
    }
}