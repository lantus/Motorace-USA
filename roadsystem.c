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