#ifndef _MEMORY_
#define _MEMORY_

ULONG Mem_GetFreeChip(void);
ULONG Mem_GetFreeAny(void);
ULONG Mem_GetFreeFast(void) ;
UBYTE Mem_GetType(const void *pMem);
void* Mem_Alloc(ULONG size, ULONG requirements);
void Mem_Free(void *ptr, ULONG size);
void* Mem_AllocChip(ULONG size);
void* Mem_AllocFast(ULONG size);
void* Mem_AllocAny(ULONG size);
void* Mem_AllocPublic(ULONG size);

#define TILEPOOL_CITY_ATTRACT   0
#define TILEPOOL_LEVEL1         1
#define TILEPOOL_LEVEL2         2
#define TILEPOOL_COUNT          3

void TilesheetPool_Initialize(void);
void TilesheetPool_Load(UBYTE sheet_id);

#endif
 