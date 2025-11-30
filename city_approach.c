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

BlitterObject city_horizon;

void City_LoadHorizon(const char* city)
{
    city_horizon.size = findSize(city);
    city_horizon.data = Disk_AllocAndLoadAsset(city, MEMF_CHIP);
}

void City_Initialize()
{
     // The City Skyline

    city_horizon.visible = TRUE;
    city_horizon.off_screen = FALSE;

    city_horizon.x = 0;
    city_horizon.y = 0;
 
    // default to demo/nyc
    City_LoadHorizon(DEMO_NYC_SKYLINE);

}

void City_BlitHorizon()
{
    // Position
    WORD x = city_horizon.x;
    WORD y = city_horizon.y;

    UWORD source_mod = 0; 
    UWORD dest_mod =  (320 - CITYSKYLINE_WIDTH) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((CITYSKYLINE_HEIGHT<<2) << 6) | CITYSKYLINE_WIDTH/16;
    
    // Source data
    UBYTE *source = (UBYTE*)&city_horizon.data[0];
 
    // Destination
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(160, x, y, admod, bltsize, source, dest);
}

void City_FreeHorizon()
{
    if (city_horizon.data != NULL)
    {
        Mem_Free(city_horizon.data, city_horizon.size);
        city_horizon.data = NULL;
    }
}

void City_DrawRoad()
{
    const UBYTE blocksperrow = 16;
    const UBYTE blockbytesperrow = 32;
    const UBYTE blockplanelines = 64;

    WORD a, b, x, y;
    
    custom->dmacon = 0x8400;
    
    for (b = 0; b < 10; b++)
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
 
    for (b = 0; b < 10; b++)
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