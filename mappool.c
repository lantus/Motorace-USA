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
#include "font.h"
#include "timers.h"
#include "title.h"
#include "blitter.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "hardware.h"

#include "cars.h"
#include "audio.h"
#include "tilesheet.h"

#include "nrv2s.h"


/* Map pool — single fast RAM buffer, load on demand */
static const char *map_files[5];
static ULONG map_sizes[5];
static UBYTE *map_buffer = NULL;
static ULONG map_buffer_size = 0;

/* Raw uncompressed sizes — largest determines buffer */
static const ULONG map_raw_sizes[5] = {
    29652,  /* lv1 */
    34956,  /* lv2 */
    32628,  /* lv3 */
    35820,  /* lv4 */
    34764,  /* lv5 */
};

void MapPool_Initialize(void)
{
    /* Find largest uncompressed map */
    map_buffer_size = 0;
    for (int i = 0; i < 5; i++)
        if (map_raw_sizes[i] > map_buffer_size)
            map_buffer_size = map_raw_sizes[i];
    
    map_buffer = AllocMem(map_buffer_size, MEMF_ANY);
    
    KPrintF("MapPool: decompression buffer=%ld bytes\n", map_buffer_size);
}

void MapPool_Load(UBYTE stage_id)
{
    Preloader_UnpackMap(stage_id, map_buffer);
    
    RawMap *loaded = (RawMap *)map_buffer;
    mapdata = (UWORD *)loaded->data;
    mapwidth = loaded->mapwidth;
    mapheight = loaded->mapheight;
}