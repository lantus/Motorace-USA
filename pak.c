#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "pak.h"
#include "memory.h"

#define NAME_LEN 32

typedef struct {
    char  name[NAME_LEN];
    ULONG offset;
    ULONG size;
} PakEntry;

typedef struct {
    char      filename[32];
    BPTR      fh;                 /* AmigaDOS file handle */
    PakEntry *directory;          /* in fast RAM */
    UWORD     count;
    BOOL      in_use;
} PakFile;

static PakFile paks[MAX_PAKS];
static BOOL pak_system_initialized = FALSE;

/* Forward */
static const char *StripPrefix(const char *name);
static PakEntry *Pak_FindInFile(PakFile *p, const char *name);
static PakEntry *Pak_Find(const char *name, PakFile **out_pak);

static void Pak_InitSystem(void)
{
    if (pak_system_initialized) return;
    for (int i = 0; i < MAX_PAKS; i++)
    {
        paks[i].in_use = FALSE;
        paks[i].fh = 0;
        paks[i].directory = NULL;
    }
    pak_system_initialized = TRUE;
}

BOOL Pak_Open(const char *filename)
{
    Pak_InitSystem();
    
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
    
    /* Open and keep open */
    p->fh = Open((STRPTR)filename, MODE_OLDFILE);
    if (!p->fh)
    {
        KPrintF("PAK: can't open %s\n", (LONG)filename);
        return FALSE;
    }
    
    /* 8-byte header */
    UBYTE header[8];
    if (Read(p->fh, header, 8) != 8)
    {
        Close(p->fh);
        p->fh = 0;
        return FALSE;
    }
    
    if (header[0] != 'P' || header[1] != 'A' || header[2] != 'K' || header[3] != '1')
    {
        KPrintF("PAK: bad magic in %s\n", (LONG)filename);
        Close(p->fh);
        p->fh = 0;
        return FALSE;
    }
    
    p->count = (header[4] << 8) | header[5];
    
    /* Read directory into fast RAM */
    ULONG dir_size = (ULONG)p->count * sizeof(PakEntry);
    p->directory = (PakEntry *)AllocMem(dir_size, MEMF_FAST);
    if (!p->directory)
        p->directory = (PakEntry *)AllocMem(dir_size, MEMF_ANY);
    if (!p->directory)
    {
        KPrintF("PAK: can't alloc directory for %s (%ld bytes)\n",
                (LONG)filename, (LONG)dir_size);
        Close(p->fh);
        p->fh = 0;
        return FALSE;
    }
    
    if (Read(p->fh, p->directory, dir_size) != (LONG)dir_size)
    {
        FreeMem(p->directory, dir_size);
        Close(p->fh);
        p->fh = 0;
        return FALSE;
    }
    
    /* Store filename for close */
    int j;
    for (j = 0; j < 31 && filename[j]; j++)
        p->filename[j] = filename[j];
    p->filename[j] = 0;
    
    p->in_use = TRUE;
    
    KPrintF("PAK: %s opened (%ld files, %ld byte index)\n",
            (LONG)filename, (LONG)p->count, (LONG)dir_size);
    return TRUE;
}

void Pak_Close(const char *filename)
{
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) continue;
        
        int match = 1;
        for (int j = 0; j < 32; j++)
        {
            if (paks[i].filename[j] != filename[j]) { match = 0; break; }
            if (filename[j] == 0) break;
        }
        
        if (match)
        {
            if (paks[i].fh) Close(paks[i].fh);
            if (paks[i].directory)
                FreeMem(paks[i].directory, (ULONG)paks[i].count * sizeof(PakEntry));
            paks[i].fh = 0;
            paks[i].directory = NULL;
            paks[i].in_use = FALSE;
            return;
        }
    }
}

void Pak_CloseAll(void)
{
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) continue;
        if (paks[i].fh) Close(paks[i].fh);
        if (paks[i].directory)
            FreeMem(paks[i].directory, (ULONG)paks[i].count * sizeof(PakEntry));
        paks[i].fh = 0;
        paks[i].directory = NULL;
        paks[i].in_use = FALSE;
    }
}

static const char *StripPrefix(const char *name)
{
    static const char *prefixes[] = {
        "objects/", "sprites/", "tiles/", "music/", 
        "sfx/", "maps/", "fonts/", "stages/", "mus/", NULL
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
    for (int i = 0; i < MAX_PAKS; i++)
    {
        if (!paks[i].in_use) continue;
        PakEntry *e = Pak_FindInFile(&paks[i], name);
        if (e) { *out_pak = &paks[i]; return e; }
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
        KPrintF("PAK: not found: %s\n", name);
        return NULL;
    }
    
    /* Allocate destination */
    UBYTE *dest = Mem_Alloc(e->size, memflags);
    if (!dest)
    {
        KPrintF("PAK: alloc %ld failed for %s\n", (LONG)e->size, name);
        return NULL;
    }
    
    /* Seek + read directly into destination — no intermediate buffer */
    Seek(p->fh, e->offset, OFFSET_BEGINNING);
    LONG bytes_read = Read(p->fh, dest, e->size);
    
    if (bytes_read != (LONG)e->size)
    {
        KPrintF("PAK: short read %s (%ld/%ld)\n", name, bytes_read, (LONG)e->size);
        Mem_Free(dest, e->size);
        return NULL;
    }
    
    return dest;
}

ULONG Pak_GetSize(const char *name)
{
    PakFile *p;
    PakEntry *e = Pak_Find(name, &p);
    return e ? e->size : 0;
}

BOOL Pak_ReadInto(const char *name, UBYTE *dest, ULONG max_size)
{
    PakFile *p;
    PakEntry *e = Pak_Find(name, &p);
    
    if (!e || e->size > max_size) return FALSE;
    
    Seek(p->fh, e->offset, OFFSET_BEGINNING);
    return (Read(p->fh, dest, e->size) == (LONG)e->size);
}

/* For sequential reads — seek to asset, then call Pak_ReadBytes repeatedly */
static PakFile *current_seek_pak = NULL;

BOOL Pak_SeekToAsset(const char *name)
{
    PakFile *p;
    PakEntry *e = Pak_Find(name, &p);
    if (!e) return FALSE;
    
    Seek(p->fh, e->offset, OFFSET_BEGINNING);
    current_seek_pak = p;
    return TRUE;
}

LONG Pak_ReadBytes(const char *name, void *buffer, ULONG size)
{
    /* name is unused in current implementation but kept for clarity / future use */
    (void)name;
    if (!current_seek_pak) return -1;
    return Read(current_seek_pak->fh, buffer, size);
}