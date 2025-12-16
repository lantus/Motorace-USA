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

BOOL g_is_music_ready = FALSE;

// Demo - Module Player - ThePlayer 6.1a: https://www.pouet.net/prod.php?which=19922
// The Player® 6.1A: Copyright © 1992-95 Jarno Paananen
// P61.testmod - Module by Skylord/Sector 7 
INCBIN(player, "mus/player610.6.no_cia.bin")

//INCBIN_CHIP(mod_start, "mus/P61.onroadstag")
INCBIN_CHIP(mod_onroad, "mus/P61.onroadstag")
//INCBIN_CHIP(mod_checkpoint, "")
//INCBIN_CHIP(mod_ranking, "")
//INCBIN_CHIP(mod_offroad,"" )
//INCBIN_CHIP(mod_frontview,"" )
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
	register volatile const void* _a3 ASM("a3") = player;
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
        //p61Init(mod_start);
        break;
    case MUSIC_ONROAD:
        p61Init(mod_onroad);
        break;
    case MUSIC_CHECKPOINT:
        //p61Init(mod_checkpoint);
        break;
    case MUSIC_RANKING:
        //p61Init(mod_ranking);
        break;
    case MUSIC_OFFROAD:
        //p61Init(mod_offroad);
        break;
    case MUSIC_FRONTVIEW:
        //p61Init(mod_frontview);
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

void Music_Play()
{
    if (g_is_music_ready == TRUE)
    {
        p61Music();
    }
}

void Music_Stop()
{
    p61End();
    g_is_music_ready = FALSE;
}

void Sfx_Play()
{

}

void Sfx_Stop()
{

}

