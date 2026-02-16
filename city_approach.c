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
#include "motorbike.h"
#include "timers.h"
#include "disk.h"
#include "font.h"
#include "audio.h"
#include "city_approach.h"

extern volatile struct Custom *custom;

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

BlitterObject nyc_horizon;
BlitterObject lv_horizon;
BlitterObject oncoming_car[8];
BlitterObject flipped_car[4];
BlitterObject *city_horizon;
BlitterObject *current_car;

APTR oncoming_car_restore_ptrs[4];
APTR *oncoming_car_restore_ptr;

BOOL frontview_bike_crashed = FALSE;
BOOL use_alt_frame = FALSE;

void City_Initialize()
{
    // The City Skyline
    nyc_horizon.visible = TRUE;
    nyc_horizon.off_screen = FALSE;
    nyc_horizon.x = 0;
    nyc_horizon.y = 0;
 
    nyc_horizon.size = findSize(NYC_SKYLINE);
    nyc_horizon.data = Disk_AllocAndLoadAsset(NYC_SKYLINE, MEMF_CHIP);
    nyc_horizon.data_frame2 = NULL;
    
    lv_horizon.size = findSize(VEGAS_SKYLINE);
    lv_horizon.data = Disk_AllocAndLoadAsset(VEGAS_SKYLINE, MEMF_CHIP);
    lv_horizon.data_frame2 = Disk_AllocAndLoadAsset(VEGAS_SKYLINE2, MEMF_CHIP);
 

    city_horizon = &nyc_horizon;

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
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, screen.pristine);
}

void City_DrawRoad()
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;

    WORD a, b, x, y;
    WORD b_offset = 1;

    //if (stage_state == STAGE_FRONTVIEW)
    //    b_offset = 0;
 
    for (b = b_offset; b < 10; b++)
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
            
                x = a << 4;
                y = b << 6;
                
                // Blit 'run_length' tiles in one operation
                DrawBlockRun(x, y + ATTRACTMODE_Y_OFFSET, block, run_length, 
                            blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
                DrawBlockRun(x, y + ATTRACTMODE_Y_OFFSET, block, run_length, 
                            blocksperrow, blockbytesperrow, blockplanelines, screen.pristine);
            }

            a += run_length;
        }
    }

    // City Horizon Animation. Some stages dont use it
    if (game_map == MAP_FRONTVIEW_LASVEGAS)
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
 
            DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                    blockplanelines, FALSE, road_tile_idx, screen.bitplanes); 

            DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                    blockplanelines, FALSE, road_tile_idx, screen.offscreen_bitplanes); 

            DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                    blockplanelines, FALSE, road_tile_idx, screen.pristine);                     

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

    
    /* 32x32
    Barrel Shifting is a pain...
    bitplanes = 4
    words per scanline - 3 words (48/16) 
                       - 4 words with -AW padding (64/16)
              
    Bytes per scanline = (64/8) = 8 bytes + 8 bytes mask 
    Total scanline = 48

    BlitSize (48x4) << 6 | 3
 
    src mod = 4 words x 2 Bytes = 8
    dst mod =  for 192 = 24 - 8 = 16
    */

    if (!car->visible) return;
    
    // Draw car
    WORD x = car->x;
    WORD y = car->y;

    UWORD source_mod = 8;
    UWORD dest_mod = (SCREENWIDTH / 8) - (4 * 2);
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((ONCOMING_CAR_HEIGHT << 2) << 6) | 4;
    
    UBYTE *source;
    if (car->id >= 4 && car->moved)
    {
        source = (UBYTE*)&flipped_car[car->id - 4].data[0];
    }
    else
    {
        source = (UBYTE*)&oncoming_car[car->id].data[0];
    }
 
    UBYTE *mask = source + source_mod;
    UBYTE *dest = draw_buffer;
 
    BlitBob2(SCREENWIDTH/2, x, y, admod, bltsize, ONCOMING_CAR_WIDTH, 
             oncoming_car_restore_ptrs, source, mask, dest);

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    
    car->old_x = x;
    car->old_y = y;
    
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
                Game_SetBackGroundColor(0xF00);  // Flash red
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
                
                KPrintF("Car passed! Total: %d/%d\n", cars_passed, TOTAL_CARS_TO_PASS);
                
                if (cars_passed >= TOTAL_CARS_TO_PASS)
                {
                 
                    city_state = CITY_STATE_INTO_HORIZON;
                    KPrintF("=== All cars passed - moving into horizon ===\n");
                }
                else
                {
                    Timer_Start(&spawn_timer, 2);
                }
            }
            
            City_DrawOncomingCar(current_car);
        }
        else
        {
            if (Timer_HasElapsed(&spawn_timer))
            {
                Timer_Stop(&spawn_timer);
                City_SpawnOncomingCar();
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
        Music_LoadModule(MUSIC_FRONTVIEW);
        MotorBike_SetFrame(BIKE_FRAME_APPROACH1);

        if (!current_car->visible && cars_passed < TOTAL_CARS_TO_PASS)
        {
            Timer_Start(&spawn_timer, 2);
        }
    }
}

void City_UpdateHorizonTransition(WORD *bike_y, WORD *bikespeed, UWORD frame_count)
{
    if (city_state != CITY_STATE_INTO_HORIZON) return;
    
    if (*bikespeed > 80 && frame_count % 3 == 0)
    {
        *bikespeed -= 1;
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
    
    if (*bike_y <= 100)
    {
        city_state = CITY_STATE_COMPLETE;
        KPrintF("=== Horizon transition complete ===\n");
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
        
        current_car->old_x = temp_x;
        current_car->old_y = temp_y;
    }
}

void City_CopyPristineBackground(BlitterObject *car)
{
  if (!car->needs_restore) return;
 
    WORD old_screen_y = car->prev_old_y;
 
    ULONG y_offset = ((ULONG)old_screen_y << 6) + ((ULONG)old_screen_y << 5);
   
 
    WORD x_word_aligned = (car->old_x >> 3) & 0xFFFE;
    
    UBYTE *pristine_ptr = screen.pristine + y_offset + x_word_aligned;
    UBYTE *screen_ptr = draw_buffer + y_offset + x_word_aligned;
    
    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
 
 
    custom->bltamod = 16;
    custom->bltdmod = 16;
    
    custom->bltapt = pristine_ptr;
    custom->bltdpt = screen_ptr;
 
    custom->bltsize = ((ONCOMING_CAR_HEIGHT << 2) << 6) | 4;
}
 
void City_ShowCityName(const char *city_name)
{
    // Draw city name on all buffers
    Font_DrawStringCentered(screen.bitplanes, (char*)city_name, 220, 7);  
    Font_DrawStringCentered(screen.offscreen_bitplanes, (char*)city_name, 220, 7);
    
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
        
        Font_RestoreFromPristine(screen.bitplanes, x, 220, text_width, CHAR_HEIGHT);
        Font_RestoreFromPristine(screen.offscreen_bitplanes, x, 220, text_width, CHAR_HEIGHT);
        
        city_state = CITY_STATE_ACTIVE;   
        
        KPrintF("City name complete - starting cars\n");
        return TRUE;
    }

    return FALSE;
}

WORD City_CalculatePerspectiveX(WORD car_y, WORD target_x)
{
  
    WORD center_y = car_y + 16;   
    
    WORD car_progress_y = center_y - HORIZON_Y;
    
    if (car_progress_y <= 0) return HORIZON_VANISHING_X;
    
    // Total Y distance from horizon (67) to bottom (240)
    WORD total_y = 240 - HORIZON_Y;  // 173 pixels
    
    WORD delta_x = target_x - HORIZON_VANISHING_X;
    
    // Linear interpolation: x = 96 + delta_x * (car_y / total_y)
    LONG x = HORIZON_VANISHING_X + ((LONG)delta_x * car_progress_y) / total_y;
    
    return (WORD)x;
}

BOOL City_CheckCarCollision(BlitterObject *car)
{
    // Only check collision for closest scales (large cars)
    if (car->id < 6) return FALSE;
    
    // Car bounding box (x is center, y is top)
    WORD car_width = 48;  // ONCOMING_CAR_WIDTH
    WORD car_left = car->x - (car_width >> 1);
    WORD car_right = car->x + (car_width >> 1);
    WORD car_bottom = car->y + ONCOMING_CAR_HEIGHT;  // Bottom edge
    
    // Bike bounding box (estimate - adjust if needed)
    WORD bike_width = 32;
    WORD bike_height = 32;
    WORD bike_left = bike_position_x - (bike_width >> 1);
    WORD bike_right = bike_position_x + (bike_width >> 1);
    WORD bike_top = bike_position_y;
    WORD bike_bottom = bike_position_y + bike_height;
    
    // Check X overlap
    BOOL x_overlap = (car_right > bike_left) && (car_left < bike_right);
    
    // Check if car bottom edge overlaps bike
    BOOL y_overlap = (car_bottom >= bike_top) && (car->y < bike_bottom);
    
    if (x_overlap && y_overlap)
    {
        return TRUE;
    }
    
    return FALSE;
}

void City_UpdateBikeCrashAnimation(void)
{
    if (!frontview_bike_crashed) return;
    
    crash_anim_counter++;
    
    // Change frame every 4 game frames
    if (crash_anim_counter % 4 == 0)
    {
        crash_anim_frame++;
 
        if (crash_anim_frame > 4)
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
