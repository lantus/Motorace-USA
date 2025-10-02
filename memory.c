#include <proto/exec.h>  
#include <proto/dos.h>  
#include "memory.h"

ULONG Mem_GetFreeChipSize(void) 
{
	return AvailMem(MEMF_CHIP);
}

ULONG Mem_GetFreeSize(void) 
{
	return AvailMem(MEMF_ANY);
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