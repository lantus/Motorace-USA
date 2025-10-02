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
#include "memory.h"
#include "disk.h"


ULONG findSize(char* filename) 
{
    BPTR handle = Open(filename, MODE_OLDFILE);

    if (handle == NULL) 
    {
        KPrintF("File %s not found\n",filename);
        return -1;
    }

    Seek(handle,0,OFFSET_END);
    ULONG size = Seek(handle,0,OFFSET_BEGINNING);
  
    Close(handle);

    handle = 0;

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