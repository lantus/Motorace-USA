#ifndef PAK_H
#define PAK_H

#include <exec/types.h>

#define MAX_PAKS 8

BOOL   Pak_Open(const char *filename);
void   Pak_Close(const char *filename);
void   Pak_CloseAll(void);
UBYTE *Pak_LoadAsset(const char *name, ULONG memflags);
ULONG  Pak_GetSize(const char *name);

#endif