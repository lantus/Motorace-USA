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
#include "sprites.h"
#include "copper.h"
#include "bitmap.h"
#include "tilemap.h"
 
extern volatile struct Custom *custom;

#define WIDTH 320
#define HEIGHT 256

UWORD wbytes[HEIGHT];

TileMap* TileMap_Create(
        BitMapEx *bitmap, 
        UBYTE *tilemap, 
        UBYTE tile_width, 
        UBYTE tile_height, 
        UWORD map_width, 
        UWORD map_height, 
        UBYTE depth)
{
    UWORD j;
    TileMap *tm = (TileMap*)AllocMem(sizeof(TileMap), MEMF_ANY);
    
    tm->map     = bitmap;
    tm->tilemap = tilemap;
    tm->twidth  = tile_width;
    tm->theight = tile_height;
    tm->tdepth  = depth;
    tm->mwidth  = map_width;
    tm->mheight = map_height;
    
    for (j = 0; j < TILES; j++) 
    {
        tm->offset[j] = tm->map->planes[0] + ((j & 0xf) << 1) + ((j >> 4) << 12);
    }
    
    for (j = 0; j < MAP_SIZE; j++) 
    {
        tm->dirty[j] = FALSE;
    }

     for (j = 0; j < HEIGHT; j++) {
        wbytes[j] = j*BITMAPLINEBYTESI;
    }

    return tm; 
}

void TileMap_DrawTile(TileMap *tm, UBYTE *dst, WORD x, WORD y, WORD tile)
{

    custom->bltcon0 = 0xFCA;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = 62;
    custom->bltbmod = 62;
    custom->bltcmod = BITMAPLINEBYTESB;
    custom->bltdmod = BITMAPLINEBYTESB;

    custom->bltapt  =  tm->offset[tile] + 32;
    custom->bltbpt  =  tm->offset[tile];
    custom->bltcpt  =  dst + ((x >> 3) & 0xFFFE) + wbytes[y];
    custom->bltdpt  =  custom->bltcpt;

    custom->bltsize = 4097;    
}