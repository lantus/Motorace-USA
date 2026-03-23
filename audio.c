#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>

#include "audio.h"
#include "memory.h"
#include "disk.h"
 
extern volatile struct Custom *custom;
extern APTR GetVBR(void) ;

/* Module storage — loaded into chip RAM */
static APTR modules[4];

/* SFX table */
static SFXStruct sfx_table[SFX_MAX];
static APTR sfx_samples[SFX_MAX];

/* a6=custom, a0=VBR, d0=PAL */
void mt_install_cia(void *vbr, UBYTE pal)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    register volatile void *_a0 ASM("a0") = vbr;
    register volatile UBYTE _d0 ASM("d0") = pal;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_install_cia\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6), "+rf"(_a0), "+rf"(_d0)
    :
    : "cc", "memory");
}

/* a6=custom */
void mt_remove_cia(void)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_remove_cia\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6)
    :
    : "cc", "memory");
}

/* a6=custom, a0=module, a1=samples, d0=startpos */
void mt_init(void *module, void *samples, UBYTE startpos)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    register volatile void *_a0 ASM("a0") = module;
    register volatile void *_a1 ASM("a1") = samples;
    register volatile UBYTE _d0 ASM("d0") = startpos;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_init\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6), "+rf"(_a0), "+rf"(_a1), "+rf"(_d0)
    :
    : "cc", "memory");
}

/* a6=custom */
void mt_end(void)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_end\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6)
    :
    : "cc", "memory");
}

/* a6=custom, d0=mask */
void mt_musicmask(UWORD mask)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    register volatile UWORD _d0 ASM("d0") = mask;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_musicmask\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6), "+rf"(_d0)
    :
    : "cc", "memory");
}
 
void mt_playfx(void *sfx)
{
    register volatile void *_a6 ASM("a6") = (void *)custom;
    register volatile void *_a0 ASM("a0") = sfx;
    __asm volatile (
        "movem.l %%d0-%%d7/%%a0-%%a5,-(%%sp)\n"
        "jsr _mt_playfx\n"
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a5"
    : "+rf"(_a6), "+rf"(_a0)
    :
    : "cc", "memory");
}

void Audio_Initialize(void)
{
    SFX_LoadAll();
 
    modules[MUSIC_START]     = Disk_AllocAndLoadAsset("mus/start.mod", MEMF_CHIP);
    modules[MUSIC_ONROAD]    = Disk_AllocAndLoadAsset("mus/onroadstage.mod", MEMF_CHIP);
    modules[MUSIC_FRONTVIEW] = Disk_AllocAndLoadAsset("mus/frontview.mod", MEMF_CHIP);
    modules[MUSIC_RANKING]   = Disk_AllocAndLoadAsset("mus/ranking.mod", MEMF_CHIP);
}

static void SFX_Load8SVX(const char *filename, UBYTE sfx_id, UWORD sample_rate, UBYTE priority)
{
    ULONG file_size = findSize((char *)filename);
    UBYTE *file_data = Disk_AllocAndLoadAsset((char *)filename, MEMF_CHIP);
    if (!file_data) { KPrintF("SFX: Failed to load %s\n", filename); return; }
    
    /* Find BODY chunk */
    UBYTE *ptr = file_data;
    UBYTE *end = file_data + file_size;
    APTR body_ptr = NULL;
    ULONG body_len = 0;
    
    /* Skip FORM header: "FORM" + size + "8SVX" = 12 bytes */
    ptr += 12;
    
    while (ptr < end - 8)
    {
        ULONG chunk_id = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
        ULONG chunk_size = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
        
        if (chunk_id == 0x424F4459)  /* "BODY" */
        {
            body_ptr = ptr;
            body_len = chunk_size;
            break;
        }
        
        ptr += chunk_size;
        if (chunk_size & 1) ptr++;  /* Word-align */
    }
    
    if (!body_ptr)
    {
        KPrintF("SFX: No BODY in %s\n", filename);
        return;
    }
    
    sfx_samples[sfx_id] = file_data;  /* Keep file in memory (chip RAM) */
    sfx_table[sfx_id].sfx_ptr = body_ptr;
    sfx_table[sfx_id].sfx_len = body_len / 2;           /* Length in WORDS */
    sfx_table[sfx_id].sfx_per = 3579545 / sample_rate;  /* Period */
    sfx_table[sfx_id].sfx_vol = 64;                     /* Max volume */
    sfx_table[sfx_id].sfx_cha = 3;                      /* Channel 3 */
    sfx_table[sfx_id].sfx_pri = priority;               /* Priority */
    
    KPrintF("SFX: %s loaded, body=%ld bytes, per=%ld\n", 
            filename, body_len, (LONG)(3579545 / sample_rate));
}


void Audio_InstallPlayer(void)
{
    APTR vbr = GetVBR();
    mt_install_cia(vbr, 1);
    mt_musicmask(0x0007);
}

void Music_LoadModule(UBYTE music_id)
{
    mt_end();
    if (modules[music_id])
    {
        mt_init(modules[music_id], NULL, 0);
        _mt_Enable = 1;
    }
}

void Music_Stop(void)
{
    _mt_Enable = 0;
    mt_end();
}

void SFX_Play(UBYTE sfx_id)
{
    if (sfx_id >= SFX_MAX) return;
    sfx_table[sfx_id].sfx_cha = 3;
    
    mt_playfx(&sfx_table[sfx_id]);
}

void Audio_Shutdown(void)
{
    mt_end();
    mt_remove_cia();
}

void SFX_LoadAll(void)
{
    SFX_Load8SVX("sfx/overtake.8svx",   SFX_OVERHEADOVERTAKE, 22050, 1);
    SFX_Load8SVX("sfx/horn.8svx",       SFX_HORN,             22050, 2);
    SFX_Load8SVX("sfx/crash.8svx",      SFX_CRASH,            22050, 3);
    SFX_Load8SVX("sfx/skid.8svx",       SFX_SKID,             22050, 1);
    SFX_Load8SVX("sfx/fvovertake.8svx", SFX_FRONTVIEWOVERTAKE,22050, 1);
}