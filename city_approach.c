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
#include "city_approach.h"

extern volatile struct Custom *custom;

// Starting Y positions for each scale (horizon to foreground)
static const WORD scale_start_y[NUM_CAR_SCALES] = {
    67,   // Scale 0 starts
    70,   // Scale 1 at Y=60
    90,   // Scale 2 at Y=70
    100,   // Scale 3 at Y=80
    110,   // Scale 4 at Y=95
    125,  // Scale 5 at Y=115
    130,  // Scale 6 at Y=140
    160   // Scale 7 at Y=170
};

// Lane X positions - CRITICAL: Use WORD not BYTE (180 as BYTE = -76!)
static const WORD lane_positions[3] = {
    100,   // Left lane
    100,  // Center lane  
    100   // Right lane
};

static WORD cars_passed = 0;
static GameTimer spawn_timer;
static BOOL city_approach_complete = FALSE;

BlitterObject nyc_horizon;
BlitterObject lv_horizon;
BlitterObject lv_horizon_anim1;
BlitterObject lv_horizon_anim2;
BlitterObject oncoming_car[8];
BlitterObject *city_horizon;
BlitterObject *current_car;

APTR oncoming_car_restore_ptrs[4];
APTR *oncoming_car_restore_ptr;

void City_Initialize()
{
    // The City Skyline
    nyc_horizon.visible = TRUE;
    nyc_horizon.off_screen = FALSE;
    nyc_horizon.x = 0;
    nyc_horizon.y = 0;
 
    nyc_horizon.size = findSize(NYC_SKYLINE);
    nyc_horizon.data = Disk_AllocAndLoadAsset(NYC_SKYLINE, MEMF_CHIP);
    
    lv_horizon.size = findSize(VEGAS_SKYLINE);
    lv_horizon.data = Disk_AllocAndLoadAsset(VEGAS_SKYLINE, MEMF_CHIP);

    lv_horizon_anim1.size = findSize(VEGAS_SKYLINE_ANIM1);
    lv_horizon_anim1.data = Disk_AllocAndLoadAsset(VEGAS_SKYLINE_ANIM1, MEMF_CHIP);

    lv_horizon_anim2.size = findSize(VEGAS_SKYLINE_ANIM2);
    lv_horizon_anim2.data = Disk_AllocAndLoadAsset(VEGAS_SKYLINE_ANIM2, MEMF_CHIP);    

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
}

void City_OncomingCarsReset()
{
 
    cars_passed = 0;
    city_approach_complete = FALSE;
 
    Timer_Stop(&spawn_timer);
    
    // Spawn first car immediately
    City_SpawnOncomingCar();
    
    KPrintF("City approach initialized - 8 cars to pass\n");
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
    UBYTE *source = (UBYTE*)&city_horizon->data[0];
  
    // Destination
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, dest);
}

void City_DrawRoad()
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;

    WORD a, b, x, y;
    
    custom->dmacon = 0x8400;
    
    for (b = 1; b < 10; b++)
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
                DrawBlockRun(x, y + ATTRACTMODE_Y_OFFSET, block, run_length, blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
            }

            a += run_length;
        }
    }
 
    custom->dmacon = 0x0400;
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
 
    for (b = 0; b < 11; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * blockplanelines;
            
            tmpa = a;
            tmpb = b;
            
            DrawBlocks(x, y + ATTRACTMODE_Y_OFFSET, tmpa, tmpb, blocksperrow, blockbytesperrow, 
                      blockplanelines, FALSE, road_tile_idx, draw_buffer); 
        }
    }

    // Blit the Skyline
    City_BlitHorizon();     
    
    // Disable DMA ONCE at end
    custom->dmacon = 0x0400;
}

void City_SpawnOncomingCar(void)
{
    current_car = &oncoming_car[7];
    current_car->visible = TRUE;
    current_car->y = scale_start_y[0];

    current_car->id = 0;  // Start at smallest scale
    current_car->y = scale_start_y[0];  // Start at first Y position
    current_car->anim_counter = cars_passed % 3;  // lane (reusing anim_counter)

    current_car->x = lane_positions[current_car->anim_counter];

    current_car->speed = 2;
    current_car->needs_restore = FALSE;
 
}

void City_RestoreBackground(BlitterObject *car)
{
     if (!car->needs_restore) return;  // ✅ Add this!

    WORD old_x = car->old_x;
    WORD old_y = car->old_y;

    APTR background = car->background;
 
    UWORD source_mod = 0;   
    UWORD dest_mod = (SCREENWIDTH / 8) - (4 * 2);
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ONCOMING_CAR_HEIGHT << 2) << 6) | 4; 
    
    UBYTE *source = background;
    UBYTE *dest = draw_buffer;
    
    BlitBobSimple(SCREENWIDTH/2, old_x, old_y, admod, bltsize, source, dest);
 
}

void City_SaveBackground(BlitterObject *car)
{
    WORD x = car->x;
    WORD y = car->y;

    APTR background = car->background;

    UWORD source_mod = (SCREENWIDTH / 8) - (4 * 2);
    UWORD dest_mod = 0;  // Tight-packed
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;

    UWORD bltsize = ((ONCOMING_CAR_HEIGHT << 2) << 6) | 4;   

    UBYTE *source = draw_buffer;
    UBYTE *dest = background;

    BlitBobSimpleSave(SCREENWIDTH/2, x, y, admod, bltsize, source, dest);
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
 
    // Restore old position first
    City_RestoreBackground(car);
    
    // Save new background
    City_SaveBackground(car);
    
    WORD x = car->x;
    WORD y = car->y;

    UWORD source_mod = 8;
    UWORD dest_mod = (SCREENWIDTH / 8) - (4 * 2);
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((ONCOMING_CAR_HEIGHT<<2) << 6) | 4;
    
    UBYTE *source = (UBYTE*)&car->data[0];
    UBYTE *mask = source + source_mod;
    UBYTE *dest = draw_buffer;
 
    BlitBob2(SCREENWIDTH/2, x, y, admod, bltsize, ONCOMING_CAR_WIDTH, 
             oncoming_car_restore_ptrs, source, mask, dest);
    
    car->needs_restore = TRUE;
    
    car->old_x = car->x;
    car->old_y = car->y;
}

void City_DrawOncomingCars(void)
{
    if (current_car->visible)
    {
        // Move car down screen
       current_car->y += current_car->speed;
        
        // Advance to next scale when passing threshold
        if (current_car->id < NUM_CAR_SCALES - 1)
        {
            // Check if we've reached the NEXT scale's Y threshold
            if (current_car->y >= scale_start_y[current_car->id + 1])
            {
                current_car->id++;
                KPrintF("Car advancing to scale %d at Y=%d\n",current_car->id, current_car->y);
            }
        }
        
        // Car has passed bike (reached largest scale and bottom of screen)
        if (current_car->id >= NUM_CAR_SCALES - 1 && current_car->y > 210)
        {
            City_RestoreBackground(current_car);  // Clean up last position
            current_car->visible = FALSE;
            cars_passed++;
            
            KPrintF("Car passed! Total: %d/%d\n", cars_passed, TOTAL_CARS_TO_PASS);
            
            if (cars_passed >= TOTAL_CARS_TO_PASS)
            {
                city_approach_complete = TRUE;
                KPrintF("=== City approach complete! ===\n");
            }
            else
            {
                Timer_Start(&spawn_timer, 2);  // 2 second delay
            }
        }
        
        // Draw the car
        City_DrawOncomingCar(current_car);
    }
    else if (!city_approach_complete)
    {
        // Wait for spawn delay
        if (Timer_HasElapsed(&spawn_timer))
        {
            Timer_Stop(&spawn_timer);
            City_SpawnOncomingCar();
        }
    }
}

BOOL City_OncomingCarsIsComplete(void)
{
    return city_approach_complete;
}