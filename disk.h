#ifndef _DISK_
#define _DISK_

#include <clib/exec_protos.h>
 
void Disk_LoadAsset(UBYTE* dest, char* filename);
void* Disk_AllocAndLoadAsset(char* filename, UBYTE mem_type);
ULONG findSize(char* fileName);

#endif

