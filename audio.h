#ifndef AUDIO_H
#define AUDIO_H

#include <exec/types.h>

extern volatile struct Custom *custom;
extern BYTE _mt_Enable;

/* ptplayer symbol references */
extern void _mt_install_cia();
extern void _mt_remove_cia();
extern void _mt_init();
extern void _mt_end();
extern void _mt_musicmask();
extern void _mt_playfx();
 

typedef struct __attribute__((packed)) {
    APTR   sfx_ptr;
    UWORD  sfx_len;
    UWORD  sfx_per;
    UWORD  sfx_vol;
    BYTE   sfx_cha;
    UBYTE  sfx_pri;
} SFXStruct;

#define SFX_OVERHEADOVERTAKE  0
#define SFX_FRONTVIEWOVERTAKE 1
#define SFX_HORN              2
#define SFX_CRASH             3
#define SFX_SKID              4
#define SFX_CRASHSKID         5
#define SFX_BRAKE             6
#define SFX_MAX               7


/* music */
#define MUSIC_START         0
#define MUSIC_ONROAD        1
#define MUSIC_FRONTVIEW     2
#define MUSIC_RANKING       3
#define MUSIC_GAMEOVER      4
#define MUSIC_OFFROAD       5
#define MUSIC_CHECKPOINT    6
#define MUSIC_VIVA_NY       7

/* API */
void Audio_Initialize(void);
void Audio_Shutdown(void);
void Audio_InstallPlayer(void);
void Music_LoadModule(UBYTE music_id);

void Music_Stop(void);
void SFX_Play(UBYTE sfx_id);
void SFX_LoadAll(void);

#endif