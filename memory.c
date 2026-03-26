#include "support/gcc8_c_support.h"
#include <proto/exec.h>  
#include <proto/dos.h>  
#include "game.h"
#include "bitmap.h"
#include "map.h"
#include "timers.h"
#include "title.h"
#include "memory.h"

/* Fast RAM copies of raw tile data */
static APTR  tilepool_fast[TILEPOOL_COUNT];
static ULONG tilepool_sizes[TILEPOOL_COUNT];

/* Shared chip RAM buffer */
static APTR  tilepool_chip_buffer = NULL;
static ULONG tilepool_chip_size = 0;

/* Track dimensions for each sheet */
static UWORD tilepool_width[TILEPOOL_COUNT];
static UWORD tilepool_height[TILEPOOL_COUNT];

ULONG Mem_GetFreeChip(void) 
{
	return AvailMem(MEMF_CHIP);
}

ULONG Mem_GetFreeAny(void) 
{
	return AvailMem(MEMF_ANY);
}

ULONG Mem_GetFreeFast(void) 
{
	return AvailMem(MEMF_FAST);
}

UBYTE Mem_GetType(const void *pMem) 
{
	ULONG ulOsType = TypeOfMem((void *)pMem);

	if(ulOsType & MEMF_FAST) 
    {
		return MEMF_FAST;
	}

	return MEMF_CHIP;
}

void* Mem_Alloc(ULONG size, ULONG requirements)
{
    void *ptr = AllocMem(size, requirements);
    if (!ptr) 
    {
        KPrintF("ERROR: Failed to allocate %ld bytes (type: %08lx)\n", size, requirements);
    }
    return ptr;
}

void Mem_Free(void *ptr, ULONG size)
{
    if (ptr) 
    {
        FreeMem(ptr, size);
    }
}
 
void* Mem_AllocChip(ULONG size)
{
    return Mem_Alloc(size, MEMF_CHIP | MEMF_CLEAR);
}

void* Mem_AllocFast(ULONG size)
{
    return Mem_Alloc(size, MEMF_FAST | MEMF_CLEAR);
}

void* Mem_AllocAny(ULONG size)
{
    return Mem_Alloc(size, MEMF_ANY | MEMF_CLEAR);
}

void* Mem_AllocPublic(ULONG size)
{
    return Mem_Alloc(size, MEMF_PUBLIC | MEMF_CLEAR);
}

void TilesheetPool_Initialize(void)
{
    tilepool_width[TILEPOOL_CITY_ATTRACT]  = ATTRACTMODE_TILES_WIDTH;     /* 256 */
    tilepool_height[TILEPOOL_CITY_ATTRACT] = ATTRACTMODE_TILES_HEIGHT+64; /* 576 */
    tilepool_sizes[TILEPOOL_CITY_ATTRACT]  = (256/8) * BLOCKSDEPTH * (512+64);
    tilepool_fast[TILEPOOL_CITY_ATTRACT]   = AllocMem(tilepool_sizes[TILEPOOL_CITY_ATTRACT], MEMF_ANY);
    Disk_LoadAsset((UBYTE *)tilepool_fast[TILEPOOL_CITY_ATTRACT], "tiles/city_attract.BPL");
    
 
    tilepool_width[TILEPOOL_LEVEL1]  = BLOCKSWIDTH;
    tilepool_height[TILEPOOL_LEVEL1] = BLOCKSHEIGHT;
    tilepool_sizes[TILEPOOL_LEVEL1]  = (BLOCKSWIDTH/8) * BLOCKSDEPTH * BLOCKSHEIGHT;
    tilepool_fast[TILEPOOL_LEVEL1]   = AllocMem(tilepool_sizes[TILEPOOL_LEVEL1], MEMF_ANY);
    Disk_LoadAsset((UBYTE *)tilepool_fast[TILEPOOL_LEVEL1], "tiles/lv1_tiles.BPL");
    
    /* Allocate ONE chip buffer sized to the largest */
    tilepool_chip_size = 0;
    for (int i = 0; i < TILEPOOL_COUNT; i++)
    {
        if (tilepool_sizes[i] > tilepool_chip_size)
            tilepool_chip_size = tilepool_sizes[i];
    }
    
    tilepool_chip_buffer = Mem_AllocChip(tilepool_chip_size);
    
    KPrintF("TilesheetPool: chip buffer=%ld bytes (largest of %ld sheets)\n",
            tilepool_chip_size, TILEPOOL_COUNT);
}

void TilesheetPool_Load(UBYTE sheet_id)
{
    if (sheet_id >= TILEPOOL_COUNT) return;
    
    /* Copy from fast to chip */
    ULONG *src = (ULONG *)tilepool_fast[sheet_id];
    ULONG *dst = (ULONG *)tilepool_chip_buffer;
    ULONG longs = (tilepool_sizes[sheet_id] + 3) >> 2;
    
    while (longs--)
        *dst++ = *src++;
    
    /* Point blocksbuffer at the shared chip buffer */
    blocksbuffer = (UBYTE *)tilepool_chip_buffer;
    
   // KPrintF("TilesheetPool: loaded sheet %ld (%ld bytes)\n",
   //         sheet_id, tilepool_sizes[sheet_id]);
}