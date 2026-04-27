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
#include "memory.h"
#include "tilesheet.h"
#include "disk.h"
#include "pak.h"
#include "nrv2s.h"
#include "memory.h"

typedef struct {
    UBYTE *data;
    ULONG  size;
} PreloadEntry;

/* ---- Compressed tilesheets ---- */
static const char *tilesheet_files[STAGE_COUNT] = {
 
    "stages/attract/attract.nrv",
    "stages/lasvegas/lv1.nrv",
    "stages/houston/lv2.nrv",
    "stages/stlouis/lv3.nrv",
    "stages/chicago/lv4.nrv",
    "stages/newyork/lv5.nrv",
  
};

/* ---- Compressed maps ---- */
static const char *map_files[STAGE_COUNT] = {
   
    "stages/attract/map_attr.nrv",
    "stages/lasvegas/map_lv1.nrv",
    "stages/houston/map_lv2.nrv",
    "stages/stlouis/map_lv3.nrv",
    "stages/chicago/map_lv4.nrv",
    "stages/newyork/map_lv5.nrv",
};

static BOOL map_compressed[STAGE_COUNT] = {
  
    TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
};

/* ---- Compressed collision ---- */
static const char *collision_files[STAGE_COUNT] = {
    
    "",
    "stages/lasvegas/col_lv1.nrv",
    "stages/houston/col_lv2.nrv",
    "stages/stlouis/col_lv3.nrv",
    "stages/chicago/col_lv4.nrv",
    "stages/newyork/col_lv5.nrv",
};
 

ULONG findSize(char* filename) 
{
     /* Check PAK first */
    ULONG pak_size = Pak_GetSize(filename);
    if (pak_size > 0) return pak_size;
    
    /* Fall back to disk */
    BPTR handle = Open(filename, MODE_OLDFILE);

    if (handle == NULL) 
    {
        KPrintF("File %s not found\n", filename);
        return -1;
    }

    Seek(handle, 0, OFFSET_END);
    ULONG size = Seek(handle, 0, OFFSET_BEGINNING);
    Close(handle);

    return size;
} 

static void load_asset_with_size(UBYTE* dest, ULONG size, char* filename) 
{
    BPTR handle = Open(filename, MODE_OLDFILE);

    if (handle) 
    {
        Read(handle,dest,size);
    } 
    else 
    {
        KPrintF("error: file '%s' not found\n", filename);
    }

    Close(handle);
    handle = 0;
}
 

void* Disk_AllocAndLoadAsset(char* filename, UBYTE mem_type) 
{
    UBYTE* dest = NULL;
    ULONG size = findSize(filename);

    switch (mem_type)
    {
        case MEMF_ANY:
            dest = Mem_AllocAny(size);
            break;
        case MEMF_FAST:
            dest = Mem_AllocFast(size);
            break;
        case MEMF_CHIP:
            dest = Mem_AllocChip(size);
            break;
        case MEMF_PUBLIC:
            dest = Mem_AllocPublic(size);
            break;
    }

    load_asset_with_size(dest,size,filename);

    return dest;
}

void Disk_LoadAsset(UBYTE* dest, char* filename) 
{
    ULONG size = findSize(filename);
    load_asset_with_size(dest,size,filename);
}

/* ---- Storage ---- */
static PreloadEntry tilesheets[TILEPOOL_COUNT];
static PreloadEntry maps[STAGE_COUNT];
static PreloadEntry collisions[STAGE_COUNT];
static PreloadEntry music[MUSIC_ID_COUNT];


static UBYTE *LoadFileToFast(const char *filename, ULONG *out_size)
{
    /* Try PAK first */
    ULONG pak_size = Pak_GetSize(filename);
    if (pak_size > 0)
    {
        UBYTE *buf = (UBYTE *)AllocMem(pak_size, MEMF_ANY);
        if (!buf)
        {
            KPrintF("Preloader: FAILED to alloc %ld bytes for '%s'\n", 
                    (LONG)pak_size, filename);
            if (out_size) *out_size = 0;
            return NULL;
        }
        
        if (!Pak_ReadInto(filename, buf, pak_size))
        {
            KPrintF("Preloader: PAK read failed for '%s'\n", filename);
            FreeMem(buf, pak_size);
            if (out_size) *out_size = 0;
            return NULL;
        }
        
        if (out_size) *out_size = pak_size;
        return buf;
    }
    
    /* Fall back to disk */
    BPTR fh = Open((CONST_STRPTR)filename, MODE_OLDFILE);
    if (!fh)
    {
        KPrintF("Preloader: FAILED to open '%s'\n", filename);
        if (out_size) *out_size = 0;
        return NULL;
    }

    Seek(fh, 0, OFFSET_END);
    LONG size = Seek(fh, 0, OFFSET_BEGINNING);

    UBYTE *buf = (UBYTE *)AllocMem(size, MEMF_ANY);
    if (!buf)
    {
        KPrintF("Preloader: FAILED to alloc %ld bytes for '%s'\n", size, filename);
        Close(fh);
        if (out_size) *out_size = 0;
        return NULL;
    }

    Read(fh, buf, size);
    Close(fh);

    if (out_size) *out_size = (ULONG)size;

    return buf;
}

/* ---- Public API ---- */

void Preloader_Init(void)
{
    for (int i = 0; i < TILEPOOL_COUNT; i++)
    {
        tilesheets[i].data = NULL;
        tilesheets[i].size = 0;
    }
    for (int i = 0; i < STAGE_COUNT; i++)
    {
        maps[i].data = NULL;
        maps[i].size = 0;
        collisions[i].data = NULL;
        collisions[i].size = 0;
    }
    for (int i = 0; i < MUSIC_ID_COUNT; i++)
    {
        music[i].data = NULL;
        music[i].size = 0;
    }
}

void Preloader_LoadAll(void)
{
    KPrintF("Preloader: Loading all assets to fast RAM...\n");

    /* Tilesheets */
    for (int i = 0; i < TILEPOOL_COUNT; i++)
    {
        tilesheets[i].data = LoadFileToFast(tilesheet_files[i], &tilesheets[i].size);
    }

    /* Maps */
    for (int i = 0; i < STAGE_COUNT; i++)
    {
        if (map_files[i])
            maps[i].data = LoadFileToFast(map_files[i], &maps[i].size);
    }

    /* Collision */
    for (int i = 0; i < STAGE_COUNT; i++)
    {
        if (collision_files[i])
            collisions[i].data = LoadFileToFast(collision_files[i], &collisions[i].size);
    }
 

    KPrintF("Preloader: All assets loaded.\n");

    /* Report memory */
    KPrintF("  Chip free: %ld\n", AvailMem(MEMF_CHIP));
    KPrintF("  Fast free: %ld\n", AvailMem(MEMF_FAST));
}

void Preloader_UnpackTilesheet(UBYTE sheet_id, UBYTE *chip_dest)
{
    if (sheet_id >= TILEPOOL_COUNT || !tilesheets[sheet_id].data) return;
    NRV2S_Unpack(tilesheets[sheet_id].data, chip_dest);
}

void Preloader_UnpackMap(UBYTE stage_id, UBYTE *fast_dest)
{
    if (stage_id >= STAGE_COUNT || !maps[stage_id].data) return;

    if (map_compressed[stage_id])
    {
        NRV2S_Unpack(maps[stage_id].data, fast_dest);
    }
    else
    {
        /* Uncompressed — straight copy */
        ULONG *src = (ULONG *)maps[stage_id].data;
        ULONG *dst = (ULONG *)fast_dest;
        ULONG longs = (maps[stage_id].size + 3) >> 2;
        while (longs--) *dst++ = *src++;
    }
}

void Preloader_UnpackCollision(UBYTE stage_id, UBYTE *fast_dest)
{
    if (stage_id >= STAGE_COUNT || !collisions[stage_id].data) return;
    NRV2S_Unpack(collisions[stage_id].data, fast_dest);
}
 