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


OncomingCar oncoming_cars[MAX_ONCOMING_CARS];

// Sprite data for 9 scales
UBYTE *car_scale_data[NUM_CAR_SCALES];

// Dimensions for each scale (example values - adjust based on your sprites)
WORD car_scale_widths[NUM_CAR_SCALES] = {
    8, 12, 16, 20, 24, 28, 32, 36, 40  // Width in pixels
};

WORD car_scale_heights[NUM_CAR_SCALES] = {
    6, 9, 12, 15, 18, 21, 24, 27, 30   // Height in pixels
};

// Starting Y positions for each scale (where car appears on screen)
static const WORD scale_start_y[NUM_CAR_SCALES] = {
    30, 40, 50, 60, 75, 90, 110, 135, 165  // Y position for each scale
};

// Lane X positions (adjust based on your road perspective)
static const WORD lane_positions[3] = {
    100,  // Left lane
    160,  // Center lane  
    220   // Right lane
};

static WORD spawn_timer = 0;
static WORD spawn_interval = 60;  // Frames between car spawns
static BOOL city_approach_complete = FALSE;

BlitterObject nyc_horizon;
BlitterObject lv_horizon;
BlitterObject lv_horizon_anim1;
BlitterObject lv_horizon_anim2;
BlitterObject *city_horizon;

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

    car_scale_data[0] = Disk_AllocAndLoadAsset(ONCOMING_CAR_1, MEMF_CHIP);  // Smallest
    car_scale_data[1] = Disk_AllocAndLoadAsset(ONCOMING_CAR_2, MEMF_CHIP);
    car_scale_data[2] = Disk_AllocAndLoadAsset(ONCOMING_CAR_3, MEMF_CHIP);
    car_scale_data[3] = Disk_AllocAndLoadAsset(ONCOMING_CAR_4, MEMF_CHIP);
    car_scale_data[4] = Disk_AllocAndLoadAsset(ONCOMING_CAR_5, MEMF_CHIP);
    car_scale_data[5] = Disk_AllocAndLoadAsset(ONCOMING_CAR_6, MEMF_CHIP);
    car_scale_data[6] = Disk_AllocAndLoadAsset(ONCOMING_CAR_7, MEMF_CHIP);
    car_scale_data[7] = Disk_AllocAndLoadAsset(ONCOMING_CAR_8, MEMF_CHIP);
    car_scale_data[8] = Disk_AllocAndLoadAsset(ONCOMING_CAR_9, MEMF_CHIP);  // Largest
}

void City_BlitHorizon()
{
    // Position
    WORD x = city_horizon->x;
    WORD y = city_horizon->y;

    UWORD source_mod = 0; 
    UWORD dest_mod =  (SCREENWIDTH - CITYSKYLINE_WIDTH) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((CITYSKYLINE_HEIGHT<<2) << 6) | CITYSKYLINE_WIDTH/16;
    
    // Source data
    UBYTE *source = (UBYTE*)&city_horizon->data[0];
 
    // Destination
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, dest);
}

void City_FreeHorizon()
{
 
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
                DrawBlockRun(x, y+ATTRACTMODE_Y_OFFSET, block, run_length, blocksperrow, blockbytesperrow, blockplanelines, draw_buffer);
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
    
    // Enable DMA ONCE at start
    custom->dmacon = 0x8400;
 
    for (b = 0; b < 11; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * blockplanelines;
            DrawBlocks(x, y+ATTRACTMODE_Y_OFFSET, a, b, blocksperrow, blockbytesperrow, 
                      blockplanelines, FALSE, road_tile_idx, draw_buffer); 
        }
    }

    
    // Blit the Skyline
    City_BlitHorizon();     
    
    // Disable DMA ONCE at end
    custom->dmacon = 0x0400;
}