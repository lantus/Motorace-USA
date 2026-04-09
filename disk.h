#ifndef _DISK_
#define _DISK_

#include <clib/exec_protos.h>
 
/* Music IDs */
#define MUSIC_ID_START        0
#define MUSIC_ID_OFFROAD      1
#define MUSIC_ID_ONROAD       2
#define MUSIC_ID_FRONTVIEW    3
#define MUSIC_ID_CHECKPOINT   4
#define MUSIC_ID_RANKING      5
#define MUSIC_ID_GAMEOVER     6
#define MUSIC_ID_VIVANY       7
#define MUSIC_ID_COUNT        8


 
void Disk_LoadAsset(UBYTE* dest, char* filename);
void* Disk_AllocAndLoadAsset(char* filename, UBYTE mem_type);
ULONG findSize(char* fileName);

void Preloader_Init(void);
void Preloader_LoadAll(void);

/* Decompress on demand — no disk I/O */
void Preloader_UnpackTilesheet(UBYTE sheet_id, UBYTE *chip_dest);
void Preloader_UnpackMap(UBYTE stage_id, UBYTE *fast_dest);
void Preloader_UnpackCollision(UBYTE stage_id, UBYTE *fast_dest);

/* Music — already uncompressed, just return pointer + size */
UBYTE *Preloader_GetMusic(UBYTE music_id);
ULONG  Preloader_GetMusicSize(UBYTE music_id);


#endif

