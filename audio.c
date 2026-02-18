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
#include "game.h"
#include "map.h"
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
#include "hud.h"
#include "font.h"
#include "title.h"
#include "hiscore.h"
#include "city_approach.h"

#include "audio.h"

extern volatile struct Custom *custom;

static UWORD master_volume = 64;
 
static BYTE silent_buffer[2] = {0, 0};

static ULONG SwapLong(ULONG v)
{
    return ((v) << 24) | 
           ((v & 0xFF00) << 8) | 
           ((v & 0xFF0000) >> 8) | 
           ((v & 0xFF000000) >> 24);
}

static UWORD SwapWord(UWORD v)
{
    return ((v & 0xFF) << 8) | ((v & 0xFF00) >> 8);
}

SFXHandle sfx_crash;
SFXHandle sfx_skid;
SFXHandle sfx_honk;
SFXHandle sfx_overtake;

BOOL g_is_music_ready = FALSE;

// Track which channels are playing one-shot SFX
UBYTE channel_play_count[4] = {0, 0, 0, 0};
 
// Demo - Module Player - ThePlayer 6.1a: https://www.pouet.net/prod.php?which=19922
// The Player® 6.1A: Copyright © 1992-95 Jarno Paananen

INCBIN(player, "mus/610.6.bin_new")
INCBIN(player_nocia, "mus/player610.6.no_cia.bin");

INCBIN_CHIP(mod_start, "mus/P61.start")
INCBIN_CHIP(mod_onroad, "mus/P61.onroadstag")
INCBIN_CHIP(mod_checkpoint, "mus/P61.checkpoint")
INCBIN_CHIP(mod_ranking, "mus/P61.ranking")
//INCBIN_CHIP(mod_offroad,"" )
INCBIN_CHIP(mod_frontview,"mus/P61.frontview" )
//INCBIN_CHIP(mod_vivany,"" )
//INCBIN_CHIP(mod_gameover,"" )
 
int p61Init(const void* module) { // returns 0 if success, non-zero otherwise
	register volatile const void* _a0 ASM("a0") = module;
	register volatile const void* _a1 ASM("a1") = NULL;
	register volatile const void* _a2 ASM("a2") = NULL;
	register volatile const void* _a3 ASM("a3") = player;
	register                int   _d0 ASM("d0"); // return value
	__asm volatile (
		"movem.l %%d1-%%d7/%%a4-%%a6,-(%%sp)\n"
		"jsr 0(%%a3)\n"
		"movem.l (%%sp)+,%%d1-%%d7/%%a4-%%a6"
	: "=r" (_d0), "+rf"(_a0), "+rf"(_a1), "+rf"(_a2), "+rf"(_a3)
	:
	: "cc", "memory");
	return _d0;
}

void p61Music() {
	register volatile const void* _a3 ASM("a3") = player;
	register volatile const void* _a6 ASM("a6") = (void*)0xdff000;
	__asm volatile (
		"movem.l %%d0-%%d7/%%a0-%%a2/%%a4-%%a5,-(%%sp)\n"
		"jsr 4(%%a3)\n"
		"movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a2/%%a4-%%a5"
	: "+rf"(_a3), "+rf"(_a6)
	:
	: "cc", "memory");
}

void p61End() {
	register volatile const void* _a3 ASM("a3") = player_nocia;
	register volatile const void* _a6 ASM("a6") = (void*)0xdff000;
	__asm volatile (
		"movem.l %%d0-%%d1/%%a0-%%a1,-(%%sp)\n"
		"jsr 8(%%a3)\n"
		"movem.l (%%sp)+,%%d0-%%d1/%%a0-%%a1"
	: "+rf"(_a3), "+rf"(_a6)
	:
	: "cc", "memory");
}

void Music_LoadModule(BYTE track)
{
    switch (track)
    {
    case MUSIC_START:
        p61Init(mod_start);
        break;
    case MUSIC_ONROAD:
        p61Init(mod_onroad);
        break;
    case MUSIC_CHECKPOINT:
        p61Init(mod_checkpoint);
        break;
    case MUSIC_RANKING:
        p61Init(mod_ranking);
        break;
    case MUSIC_OFFROAD:
        //p61Init(mod_offroad);
        break;
    case MUSIC_FRONTVIEW:
        p61Init(mod_frontview);
        break;
    case MUSIC_VIVANY:
        //p61Init(mod_vivany);
        break;
    case MUSIC_GAMEOVER:
        //p61Init(mod_gameover);
        break;
    }
    custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB | INTF_EXTER;
    custom->intreq = (1<<INTB_VERTB);  // Clear pending

    g_is_music_ready = TRUE;
}
 
void Music_Stop()
{
    p61End();
    g_is_music_ready = FALSE;
}

BOOL SFX_LoadRaw(const char *filename, SFXHandle *sfx, UWORD sample_rate)
{
    if (!sfx) return FALSE;
    
    // Initialize handle
    sfx->sample_data = NULL;
    sfx->sample_length = 0;
    sfx->sample_rate = sample_rate;
    sfx->volume = 64;
    sfx->loaded = FALSE;
    
    // Open file
    BPTR file = Open((STRPTR)filename, MODE_OLDFILE);
    if (!file)
    {
        KPrintF("SFX_LoadRaw: Failed to open %s\n", filename);
        return FALSE;
    }
    
    // Get file size
    Seek(file, 0, OFFSET_END);
    LONG file_size = Seek(file, 0, OFFSET_BEGINNING);
    
    if (file_size <= 0)
    {
        KPrintF("SFX_LoadRaw: Invalid file size\n");
        Close(file);
        return FALSE;
    }
    
    // Allocate CHIP RAM
    sfx->sample_data = Mem_AllocChip(file_size);
    if (!sfx->sample_data)
    {
        KPrintF("SFX_LoadRaw: Failed to allocate CHIP RAM\n");
        Close(file);
        return FALSE;
    }
    
    // Read entire file
    if (Read(file, sfx->sample_data, file_size) != file_size)
    {
        KPrintF("SFX_LoadRaw: Failed to read file\n");
       
        Close(file);
        return FALSE;
    }
    
    Close(file);
    
    sfx->sample_length = file_size;
    sfx->loaded = TRUE;
    
    KPrintF("SFX_LoadRaw: Loaded %s (%ld bytes @ %d Hz)\n", 
            filename, sfx->sample_length, sfx->sample_rate);
    
    return TRUE;
}
 
void SFX_Free(SFXHandle *sfx)
{
    if (!sfx || !sfx->loaded) return;
    
    if (sfx->sample_data)
    {
        
        sfx->sample_data = NULL;
    }
    
    sfx->sample_length = 0;
    sfx->loaded = FALSE;
}

void Audio_Initialize(void)
{
    SFX_LoadRaw("sfx/crash.8svx", &sfx_crash, 22050);
    SFX_LoadRaw("sfx/honk.8svx", &sfx_honk, 22050);
    SFX_LoadRaw("sfx/skid.8svx", &sfx_skid, 22050);

    SFX_StopAll();
    
    // Clear all channel states
    for (int i = 0; i < 4; i++)
    {
        channel_play_count[i] = 0;
    }
 
}

void SFX_Play(UBYTE effect)
{
    switch (effect)
    {
        case SFX_CRASH:
            SFX_PlayEffect(&sfx_crash, SFX_CHANNEL_3, 0);  
            break;
        case SFX_SKID:
            SFX_PlayEffect(&sfx_skid, SFX_CHANNEL_3, 0);  
            break;
        case SFX_HONK:
            SFX_PlayEffect(&sfx_honk, SFX_CHANNEL_3, 0);  
            break;
        case SFX_CRASHSKID:
            SFX_PlayEffect(&sfx_crash, SFX_CHANNEL_3, 0); 
            SFX_PlayEffect(&sfx_skid, SFX_CHANNEL_2, 0);   
            break;
    } 
}

void SFX_PlayEffect(SFXHandle *sfx, SFXChannel channel, UWORD volume)
{
    if (!sfx || !sfx->loaded || channel > 3) return;
    
    if (volume == 0) volume = sfx->volume;
    volume = (volume * master_volume) >> 6;
    if (volume > 64) volume = 64;
  
    UWORD period = 3579545UL / sfx->sample_rate;
 
    custom->intena = INTF_SETCLR | (1 << 7 + channel);
    volatile struct AudChannel *chan = (struct AudChannel *)&custom->aud[channel];
 
    // One-shot setup
    chan->ac_ptr = (UWORD *)silent_buffer;
    chan->ac_len = 1;
    chan->ac_per = period;
    chan->ac_vol = volume;

    LONG vpos = VBeamPos();
    while (VBeamPos() - vpos < 6) ;

    // Set real sample
    chan->ac_ptr = (UWORD *)sfx->sample_data;
    chan->ac_len = sfx->sample_length >> 1;

    channel_play_count[channel] = 0;

    // Enable DMA
    custom->dmacon = DMAF_SETCLR | (1 << channel);
 
}

void SFX_Stop(SFXChannel channel)
{
    if (channel > 3) return;
    
    // Disable DMA for this channel
    custom->dmacon = (1 << channel);
}

void SFX_StopAll(void)
{
    custom->dmacon = DMAF_AUD0 | DMAF_AUD1 | DMAF_AUD2 | DMAF_AUD3;
}

void SFX_SetMasterVolume(UWORD volume)
{
    if (volume > 64) volume = 64;
    master_volume = volume;
}
