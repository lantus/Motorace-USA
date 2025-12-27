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

BYTE road_tile_idx = 0;

typedef struct {
    UWORD tile_idx;          // Current tile index (0-14)
    UWORD scroll_fraction;   // Accumulator for fractional movement (0-255)
} RoadScroll;

RoadScroll road_scroll = {0, 0};

UBYTE stage1_tile_attrib_map[TILEATTRIB_MAP_SIZE];

void UpdateRoadScroll(UWORD bike_speed, UWORD frame_count)
{
     UWORD scroll_speed;
    
    // Map speed to scroll rate (higher = faster scrolling)

    if (bike_speed > 200)
        scroll_speed = 255;  // Maximum speed
    else if (bike_speed > 150)
        scroll_speed = 192;
    else if (bike_speed > 120)
        scroll_speed = 128;
    else if (bike_speed > 100)
        scroll_speed = 96;
    else if (bike_speed > 60)
        scroll_speed = 64;
    else if (bike_speed > 40)
        scroll_speed = 48;
    else if (bike_speed > 20)
        scroll_speed = 28;
    else if (bike_speed > 10)
        scroll_speed = 10;                
    else
        return;  // Too slow
    
    // Accumulate fractional movement
    road_scroll.scroll_fraction += scroll_speed;
    
    // When fraction overflows 255, advance to next tile
    if (road_scroll.scroll_fraction >= 255)
    {
        road_scroll.scroll_fraction -= 255;
        road_scroll.tile_idx = (road_scroll.tile_idx + 1) % 15;
    }
    
    // Use road_scroll.tile_idx for rendering
    road_tile_idx = road_scroll.tile_idx;
}

void TileAttrib_Load(void)
{
    // Load collision.dat from disk
    UBYTE *data = Disk_AllocAndLoadAsset("stages/lasvegas/collision.dat", MEMF_ANY);
    
    if (data)
    {
        // Copy collision data
        for (int i = 0; i < TILEATTRIB_MAP_SIZE; i++)
        {
            stage1_tile_attrib_map[i] = data[i];
        }
        
        FreeMem(data, TILEATTRIB_MAP_SIZE);
        KPrintF("Collision map loaded (%ld tiles)\n", TILEATTRIB_MAP_SIZE);
    }
    else
    {
        // Default to all crash if file not found
        for (int i = 0; i < TILEATTRIB_MAP_SIZE; i++)
        {
            stage1_tile_attrib_map[i] = TILEATTRIB_CRASH;
        }
        KPrintF("Warning: collision.dat not found, using defaults\n");
    }
}

BOOL TileAttrib_IsDrivable(WORD tile_x, WORD tile_y)
{
    // Debug counter - only output every 30 frames
    static int debug_frame = 0;
    debug_frame++;
    

     // Get tile coordinates in the map
    WORD map_tile_x = tile_x / BLOCKWIDTH;
    WORD map_tile_y = tile_y / BLOCKHEIGHT;
    
    // Bounds check
    if (map_tile_x < 0 || map_tile_x >= mapwidth ||
        map_tile_y < 0 || map_tile_y >= mapheight)
    {
        return FALSE;
    }
    
    UWORD tile_number = mapdata[map_tile_y * mapwidth + map_tile_x];
 
 
    TileAttribute type = stage1_tile_attrib_map[tile_number];
    
         // Debug output for car 3 every 30 frames
   // if ( debug_frame % 30 == 0)
   // {
   //     KPrintF("tile_number=%ld map_tile_x=%ld map_tile_y=%ld type=%ld\n",
   //             tile_number,map_tile_x,map_tile_y,type);
   // }

    // Drivable types
    return (type != TILEATTRIB_CRASH);
}

TileAttribute Tile_GetAttrib(WORD world_x, WORD world_y)
{
    // Convert world coordinates to tile coordinates
    WORD tile_x = world_x / 16;
    WORD tile_y = (world_y / 16) % TILEATTRIB_MAP_HEIGHT;  // Wrap vertically
    
    if (tile_x < 0 || tile_x >= TILEATTRIB_MAP_WIDTH ||
        tile_y < 0 || tile_y >= TILEATTRIB_MAP_HEIGHT)
    {
        return TILEATTRIB_CRASH;
    }
    
    WORD tile_index = tile_y * TILEATTRIB_MAP_WIDTH + tile_x;
    return stage1_tile_attrib_map[tile_index];
}