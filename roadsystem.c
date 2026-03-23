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
 
/* collision.dat is now mapwidth * mapheight bytes, one per cell */
UBYTE *collision_map = NULL;   /* Loaded from disk, MEMF_ANY is fine */
WORD   col_map_width;
WORD   col_map_height;

// NTSC road scroll speed table — index by bike_speed >> 4 (0-15)
static const UWORD road_scroll_ntsc[16] = {
//  0    16   32   48   64   80   96  112  128  144  160  176  192  208  224  240
    0,   10,  28,  48,  64,  80,  96, 112, 128, 160, 192, 224, 240, 248, 255, 255
};

// PAL — each value * 1.2
static const UWORD road_scroll_pal[16] = {
    0,   12,  34,  58,  77,  96, 115, 134, 154, 192, 230, 255, 255, 255, 255, 255
};

static const UWORD *road_scroll_table;

 
void UpdateRoadScroll(UWORD bike_speed, UWORD frame_count)
{
    UWORD scroll_speed;

    if (bike_speed > 200)
        scroll_speed = 512;
    else if (bike_speed > 150)
        scroll_speed = 384;
    else if (bike_speed > 120)
        scroll_speed = 256;
    else if (bike_speed > 100)
        scroll_speed = 192;
    else if (bike_speed > 60)
        scroll_speed = 128;
    else if (bike_speed > 40)
        scroll_speed = 80;
    else if (bike_speed > 20)
        scroll_speed = 40;
    else if (bike_speed > 10)
        scroll_speed = 16;
    else
        return;
        
    road_scroll.scroll_fraction += scroll_speed;

    while (road_scroll.scroll_fraction >= 255)
    {
        road_scroll.scroll_fraction -= 255;
        road_scroll.tile_idx++;
        if (road_scroll.tile_idx >= 15)
            road_scroll.tile_idx = 0;
    }

    road_tile_idx = road_scroll.tile_idx;
}
 
void CollisionMap_Load(void)
{
    collision_map = Disk_AllocAndLoadAsset("stages/lasvegas/collision.dat", MEMF_ANY);
    col_map_width = mapwidth;    /* Same dimensions as tilemap */
    col_map_height = mapheight;
    
    if (collision_map)
        KPrintF("Collision map loaded: %ldx%ld\n", col_map_width, col_map_height);
    else
        KPrintF("WARNING: collision.dat not found!\n");
}

/* 
 * Single lookup — no tile index indirection needed.
 * 
 * Old way (2 lookups):
 *   tile_idx = mapdata[y * width + x]     ; 1st read (word)
 *   attrib = attrib_table[tile_idx]        ; 2nd read (byte) + add
 *
 * New way (1 lookup):
 *   packed = collision_map[y * width + x]  ; 1 read (byte)
 *   lane = packed >> 4                     ; shift
 *   surface = packed & 0x0F                ; mask
 */
__attribute__((always_inline)) 
UBYTE Collision_Get(WORD world_x, WORD world_y)
{
    WORD tx = world_x >> 4;  /* / 16 */
    WORD ty = world_y >> 4;
    
    if (tx < 0 || tx >= col_map_width || ty < 0 || ty >= col_map_height)
        return 0;  /* LANE_OFFROAD | SURFACE_NORMAL = crash */
    
    return collision_map[ty * col_map_width + tx];
}

/* Convenience: just the lane */
__attribute__((always_inline))
UBYTE Collision_GetLane(WORD world_x, WORD world_y)
{
    return GET_LANE(Collision_Get(world_x, world_y));
}

/* Convenience: just the surface */
__attribute__((always_inline))
UBYTE Collision_GetSurface(WORD world_x, WORD world_y)
{
    return GET_SURFACE(Collision_Get(world_x, world_y));
}
 
WORD road_center_cache[ROAD_CACHE_SIZE];

void Road_CacheRow(WORD world_y)
{
    WORD mapy = world_y >> 4;
    WORD left = -1, right = -1;
    
    for (WORD scan = 0; scan < 192; scan += 16)
    {
        UBYTE lane = Collision_GetLane(scan, world_y);
        if (lane >= LANE_1 && lane <= LANE_4)
        {
            if (left < 0) left = scan;
            right = scan;
        }
    }
    
    road_center_cache[mapy & 0xFF] = (left >= 0) ? (left + right) / 2 : 96;
}
 

void Road_CacheFillVisible(void)
{
    for (WORD y = mapposy - 64; y < mapposy + SCREENHEIGHT + 64; y += 16)
    {
        if (y < 0) continue;
        Road_CacheRow(y);
    }
}

 