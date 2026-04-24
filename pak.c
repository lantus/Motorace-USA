#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include "pak.h"
#include "disk.h"
#include "memory.h"

#define NAME_LEN 32

typedef struct {
    char  name[NAME_LEN];
    ULONG offset;
    ULONG size;
} PakEntry;

typedef struct {
    char      filename[32];       /* "objects.dat" etc. */
    UBYTE    *data;                /* whole file in fast RAM */
    ULONG     size;
    PakEntry *directory;
    UWORD     count;
    BOOL      in_use;
} PakFile;

static PakFile paks[MAX_PAKS];
static BOOL pak_system_initialized = FALSE;

static void Pak_InitSystem(void)
{
    if (pak_system_initialized) return;
    for (int i = 0; i < MAX_PAKS; i++)
    {
        paks[i].in_use = FALSE;
        paks[i].data = NULL;
    }
    pak_system_initialized = TRUE;
}

BOOL Pak_Open(const char *filename)
{
    Pak_InitSystem();
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) { slot = i; break; }
    }
    
    if (slot < 0)
    {
        KPrintF("PAK: no free slots\n");
        return FALSE;
    }
    
    PakFile *p = &paks[slot];
    
    p->size = findSize((char *)filename);
    if (p->size == 0) return FALSE;
    
    p->data = Disk_AllocAndLoadAsset((char *)filename, MEMF_ANY);
    if (!p->data) return FALSE;
    
    if (p->data[0] != 'P' || p->data[1] != 'A' || 
        p->data[2] != 'K' || p->data[3] != '1')
    {
        FreeMem(p->data, p->size);
        p->data = NULL;
        return FALSE;
    }
    
    p->count = (p->data[4] << 8) | p->data[5];
    p->directory = (PakEntry *)(p->data + 8);
    
    /* Store filename for later lookup/close */
    int j;
    for (j = 0; j < 31 && filename[j]; j++)
        p->filename[j] = filename[j];
    p->filename[j] = 0;
    
    p->in_use = TRUE;
    
    KPrintF("PAK: opened %s (%ld files, %ld bytes)\n", 
            (LONG)filename, (LONG)p->count, (LONG)p->size);
    return TRUE;
}

void Pak_Close(const char *filename)
{
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) continue;
        
        /* Simple string compare */
        int match = 1;
        for (int j = 0; j < 32; j++)
        {
            if (paks[i].filename[j] != filename[j]) { match = 0; break; }
            if (filename[j] == 0) break;
        }
        
        if (match)
        {
            FreeMem(paks[i].data, paks[i].size);
            paks[i].data = NULL;
            paks[i].directory = NULL;
            paks[i].count = 0;
            paks[i].in_use = FALSE;
            return;
        }
    }
}

void Pak_CloseAll(void)
{
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (paks[i].in_use)
        {
            FreeMem(paks[i].data, paks[i].size);
            paks[i].data = NULL;
            paks[i].in_use = FALSE;
        }
    }
}

/* Strip common path prefixes before lookup */
static const char *StripPrefix(const char *name)
{
    /* Strip "objects/", "sprites/", "tiles/", "music/", "sfx/", "maps/" etc. */
    static const char *prefixes[] = {
        "objects/", "sprites/", "tiles/", "music/", 
        "sfx/", "maps/", "fonts/", NULL
    };
    
    for (int i = 0; prefixes[i]; i++)
    {
        const char *pfx = prefixes[i];
        int match = 1;
        int j;
        for (j = 0; pfx[j]; j++)
        {
            char c = name[j];
            if (c == '\\') c = '/';
            if (c != pfx[j]) { match = 0; break; }
        }
        if (match) return name + j;
    }
    
    return name;
}

static PakEntry *Pak_FindInFile(PakFile *p, const char *name)
{
    const char *n = StripPrefix(name);
    
    for (UWORD i = 0; i < p->count; i++)
    {
        PakEntry *e = &p->directory[i];
        int match = 1;
        for (int j = 0; j < NAME_LEN; j++)
        {
            char ec = e->name[j];
            char nc = n[j];
            if (ec == '\\') ec = '/';
            if (nc == '\\') nc = '/';
            if (ec != nc) { match = 0; break; }
            if (ec == 0) break;
        }
        if (match) return e;
    }
    return NULL;
}

static PakEntry *Pak_Find(const char *name, PakFile **out_pak)
{
    /* Search ALL open PAK files for the asset */
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) continue;
        
        PakEntry *e = Pak_FindInFile(&paks[i], name);
        if (e)
        {
            *out_pak = &paks[i];
            return e;
        }
    }
    
    *out_pak = NULL;
    return NULL;
}

UBYTE *Pak_LoadAsset(const char *name, ULONG memflags)
{
    PakFile *p;
    PakEntry *e = Pak_Find(name, &p);
    
    if (!e)
    {
        KPrintF("PAK: file not found: %s\n", name);
        return NULL;
    }
    
    UBYTE *dest = Mem_Alloc(e->size, memflags);
    if (!dest) return NULL;
    
    UBYTE *src = p->data + e->offset;
    
    ULONG longs = e->size >> 2;
    ULONG *s = (ULONG *)src;
    ULONG *d = (ULONG *)dest;
    while (longs--) *d++ = *s++;
    
    ULONG remainder = e->size & 3;
    UBYTE *sb = (UBYTE *)s;
    UBYTE *db = (UBYTE *)d;
    while (remainder--) *db++ = *sb++;
    
    return dest;
}

ULONG Pak_GetSize(const char *name)
{
    PakFile *p;
    PakEntry *e = Pak_Find(name, &p);
    return e ? e->size : 0;
}