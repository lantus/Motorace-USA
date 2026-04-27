#ifndef PAK_H
#define PAK_H

#include <exec/types.h>

#define MAX_PAKS 8

/* Open a PAK file — keeps file handle open, loads only directory into fast RAM */
BOOL   Pak_Open(const char *filename);

/* Close one PAK by filename */
void   Pak_Close(const char *filename);

/* Close all open PAKs — call after init is complete */
void   Pak_CloseAll(void);

/* Load an asset by name — searches all open PAKs, allocates with memflags,
   reads directly from disk into the allocation */
UBYTE *Pak_LoadAsset(const char *name, ULONG memflags);

/* Get size of an asset without loading it */
ULONG  Pak_GetSize(const char *name);

/* Read an asset directly into a pre-allocated buffer (no allocation) */
BOOL   Pak_ReadInto(const char *name, UBYTE *dest, ULONG max_size);

/* Seek a PAK file to an asset's offset — used for sequential reads
   (e.g. sprite header + offsets + palette + imgdata in separate Read calls).
   Returns TRUE if found and seeked. */
BOOL   Pak_SeekToAsset(const char *name);

/* Read bytes from the currently-seeked PAK file (after Pak_SeekToAsset).
   Returns bytes actually read. */
LONG   Pak_ReadBytes(const char *name, void *buffer, ULONG size);

#endif