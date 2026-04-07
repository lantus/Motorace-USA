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

/* Temp buffer for compressed data — allocate in fast RAM at init */
static UBYTE *compressed_buffer = NULL;
#define COMPRESSED_BUFFER_SIZE  32768   /* Largest compressed file */


/* Fast RAM copies of raw tile data */
static APTR  tilepool_fast[TILEPOOL_COUNT];
static ULONG tilepool_sizes[TILEPOOL_COUNT];

/* Shared chip RAM buffer */
static APTR  tilepool_chip_buffer = NULL;
static ULONG tilepool_chip_size = 0;

/* Track dimensions for each sheet */
static UWORD tilepool_width[TILEPOOL_COUNT];
static UWORD tilepool_height[TILEPOOL_COUNT];

static const char *tilepool_files[TILEPOOL_COUNT];

void TilesheetPool_Initialize(void)
{
    /* ---- City Attract ---- */
    tilepool_files[TILEPOOL_CITY_ATTRACT]  = "stages/attract/attract.nrv";
    tilepool_width[TILEPOOL_CITY_ATTRACT]  = ATTRACTMODE_TILES_WIDTH;
    tilepool_height[TILEPOOL_CITY_ATTRACT] = ATTRACTMODE_TILES_HEIGHT + 64;
    tilepool_sizes[TILEPOOL_CITY_ATTRACT]  = (ATTRACTMODE_TILES_WIDTH / 8) * BLOCKSDEPTH * (ATTRACTMODE_TILES_HEIGHT + 64);

    /* ---- Level 1 (Las Vegas overhead) ---- */
    tilepool_files[TILEPOOL_LEVEL1]  = "stages/lasvegas/lv1.nrv";
    tilepool_width[TILEPOOL_LEVEL1]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL1] = LV1_BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL1]  = (BLOCKSWIDTH / 8) * BLOCKSDEPTH * LV1_BLOCKSHEIGHT;

    /* ---- Level 2 (Houston overhead) ---- */
    tilepool_files[TILEPOOL_LEVEL2]  = "stages/houston/lv2.nrv";
    tilepool_width[TILEPOOL_LEVEL2]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL2] = LV2_BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL2]  = (BLOCKSWIDTH / 8) * BLOCKSDEPTH * LV2_BLOCKSHEIGHT;

    /* ---- Level 3 (St. Louis overhead) ---- */
    tilepool_files[TILEPOOL_LEVEL3]  = "stages/stlouis/lv3.nrv";
    tilepool_width[TILEPOOL_LEVEL3]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL3] = LV3_BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL3]  = (BLOCKSWIDTH / 8) * BLOCKSDEPTH * LV3_BLOCKSHEIGHT;

    /* ---- Level 4 (Chicago overhead) ---- */
    tilepool_files[TILEPOOL_LEVEL4]  = "stages/chicago/lv4.nrv";
    tilepool_width[TILEPOOL_LEVEL4]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL4] = LV4_BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL4]  = (BLOCKSWIDTH / 8) * BLOCKSDEPTH * LV4_BLOCKSHEIGHT; 

     /* ---- Level 5 (NYC overhead) ---- */
    tilepool_files[TILEPOOL_LEVEL5]  = "stages/newyork/lv5.nrv";
    tilepool_width[TILEPOOL_LEVEL5]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL5] = LV5_BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL5]  = (BLOCKSWIDTH / 8) * BLOCKSDEPTH * LV5_BLOCKSHEIGHT;    

    /* ---- Allocate ONE chip buffer sized to the largest DECOMPRESSED sheet ---- */
    tilepool_chip_size = 0;
    for (int i = 0; i < TILEPOOL_COUNT; i++)
    {
        if (tilepool_sizes[i] > tilepool_chip_size)
            tilepool_chip_size = tilepool_sizes[i];
    }
    tilepool_chip_buffer = Mem_AllocChip(tilepool_chip_size);

    /* ---- Temp buffer for compressed data (fast RAM) ---- */
    compressed_buffer = AllocMem(COMPRESSED_BUFFER_SIZE, MEMF_ANY);

    KPrintF("TilesheetPool: chip buffer=%ld bytes, compressed buffer=%ld bytes, NO preload\n",
            tilepool_chip_size, COMPRESSED_BUFFER_SIZE);
}

void TilesheetPool_Load(UBYTE sheet_id)
{
    if (sheet_id >= TILEPOOL_COUNT) return;
    
    /* Show loading message */
    if (draw_buffer)
    {
        //Font_DrawStringCentered(draw_buffer, "LOADING PLEASE WAIT...", 120, 15);
        Game_ResetBitplanePointer();
        Copper_SetPalette(0, 0x000);
        Copper_SetPalette(15, 0xFFF);
        WaitVBL();
    }
    
    /* Load compressed data into temp buffer */
    System_EnableOS();
    Disk_LoadAsset(compressed_buffer, tilepool_files[sheet_id]);
    System_DisableOS();
    
    /* Decompress into chip buffer */
    NRV2S_Unpack(compressed_buffer, tilepool_chip_buffer);
    
    /* Clear loading message */
    Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);
    Copper_SetPalette(15, 0x000);
    
    blocksbuffer = (UBYTE *)tilepool_chip_buffer;
   
    KPrintF("Chip Ram Free: %ld\n", Mem_GetFreeChip());
    KPrintF("Fast Ram Free: %ld\n", Mem_GetFreeFast());
}