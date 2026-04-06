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

void MapPool_Initialize(void)
{
    map_files[0] = "stages/lasvegas/lv1.map";
    map_files[1] = "stages/houston/lv2.map";
    map_files[2] = "stages/stlouis/lv3.map";
    map_files[3] = "stages/chicago/lv4.map";
    map_files[4] = "stages/newyork/lv5.map";
    
    map_sizes[0] = findSize(map_files[0]);
    map_sizes[1] = findSize(map_files[1]);
    map_sizes[2] = findSize(map_files[2]);
    map_sizes[3] = findSize(map_files[3]);
    map_sizes[4] = findSize(map_files[4]);
    
    /* Find largest */
    map_buffer_size = 0;
    for (int i = 0; i < 5; i++)
        if (map_sizes[i] > map_buffer_size)
            map_buffer_size = map_sizes[i];
    
    map_buffer = AllocMem(map_buffer_size, MEMF_ANY);
    
    KPrintF("MapPool: buffer=%ld bytes (largest of 5 maps)\n", map_buffer_size);
}

void MapPool_Load(UBYTE stage)
{
    if (stage > 4) return;
    
    System_EnableOS();
    Disk_LoadAsset(map_buffer, map_files[stage]);
    System_DisableOS();
    
    /* Point the shared RawMap at the buffer */
    RawMap *loaded = (RawMap *)map_buffer;
    mapdata = (UWORD *)loaded->data;
    mapwidth = loaded->mapwidth;
    mapheight = loaded->mapheight;
}