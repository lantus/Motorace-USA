ULONG Mem_GetFreeChipSize(void);
ULONG Mem_GetFreeSize(void);
UBYTE Mem_GetType(const void *pMem);
void* Mem_Alloc(ULONG size, ULONG requirements);
void Mem_Free(void *ptr, ULONG size);
void* Mem_AllocChip(ULONG size);
void* Mem_AllocFast(ULONG size);
void* Mem_AllocAny(ULONG size);