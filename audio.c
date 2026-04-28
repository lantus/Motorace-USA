#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <hardware/custom.h>
#include <proto/exec.h>

#include "audio.h"
#include "memory.h"
#include "pak.h"
#include "disk.h"

extern volatile struct Custom *custom;
extern APTR GetVBR(void);

/* Module storage — fast RAM, copied to chip on demand */
static APTR modules_fast[MUSIC_COUNT];
static ULONG module_sizes[MUSIC_COUNT];
static APTR module_chip_buffer = NULL;
static ULONG module_chip_size = 0;

/* SFX table */
static SFXStruct sfx_table[SFX_MAX];
static APTR sfx_samples[SFX_MAX];

/* ---- ptplayer wrappers ---- */

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

/* ---- SFX ---- */

static void SFX_Load8SVX(const char *filename, UBYTE sfx_id, UWORD sample_rate, UBYTE priority)
{
    ULONG file_size = Pak_GetSize((char *)filename);
    UBYTE *file_data = Pak_LoadAsset((char *)filename, MEMF_CHIP);
    if (!file_data) { KPrintF("SFX: Failed to load %s\n", filename); return; }
    
    UBYTE *ptr = file_data + 12;  /* Skip FORM header */
    UBYTE *end = file_data + file_size;
    APTR body_ptr = NULL;
    ULONG body_len = 0;
    
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
        if (chunk_size & 1) ptr++;
    }
    
    if (!body_ptr)
    {
        KPrintF("SFX: No BODY in %s\n", filename);
        return;
    }
    
    sfx_samples[sfx_id] = file_data;
    sfx_table[sfx_id].sfx_ptr = body_ptr;
    sfx_table[sfx_id].sfx_len = body_len / 2;
    sfx_table[sfx_id].sfx_per = 3579545 / sample_rate;
    sfx_table[sfx_id].sfx_vol = 64;
    sfx_table[sfx_id].sfx_cha = 3;
    sfx_table[sfx_id].sfx_pri = priority;
}

void SFX_LoadAll(void)
{
    SFX_Load8SVX("sfx/crashskid.8SVX", SFX_CRASHSKID, 22050, 2);
    SFX_Load8SVX("sfx/horn.8SVX", SFX_HORN, 22050, 3);
    SFX_Load8SVX("sfx/skid.8SVX", SFX_SKID, 22050, 3);
    SFX_Load8SVX("sfx/overtake.8SVX", SFX_OVERHEADOVERTAKE, 22050, 2);
    SFX_Load8SVX("sfx/fvovertake.8SVX", SFX_FRONTVIEWOVERTAKE, 22050, 2);
    SFX_Load8SVX("sfx/empty.8SVX", SFX_EMPTY, 22050, 1);

    sfx_table[SFX_BRAKE] = sfx_table[SFX_SKID];
    sfx_table[SFX_BRAKE].sfx_per = 250;
}

/* ---- Music loading ---- */

static const char *music_files[MUSIC_COUNT] = {
    "mus/start.mod",
    "mus/checkpoint.mod",
    "mus/frontview.mod",
    "mus/gameover.mod",
    "mus/offroad.mod",
    "mus/onroadstage.mod",
    "mus/ranking.mod",
    "mus/viva ny.mod",
};

static void Music_LoadAllToFast(void)
{
    ULONG max_size = 0;

    for (int i = 0; i < MUSIC_COUNT; i++)
    {
        module_sizes[i] = Pak_GetSize((char *)music_files[i]);
        modules_fast[i] = Pak_LoadAsset((char *)music_files[i], MEMF_ANY);

        if (module_sizes[i] > max_size)
            max_size = module_sizes[i];

        KPrintF("Music: '%s' (%ld bytes, fast)\n", music_files[i], module_sizes[i]);
    }

    module_chip_size = max_size;
    module_chip_buffer = Mem_AllocChip(max_size);
    KPrintF("Music: chip buffer=%ld bytes\n", max_size);
}

/* ---- Public API ---- */

void Audio_Initialize(void)
{
    SFX_LoadAll();
    Music_LoadAllToFast();
}

void Audio_InstallPlayer(void)
{
    APTR vbr = GetVBR();
    mt_install_cia(vbr, 1);
    mt_musicmask(0x0007);
}

void Music_LoadModule(UBYTE music_id)
{
    if (music_id >= MUSIC_COUNT) return;
    if (!modules_fast[music_id] || !module_chip_buffer) return;

    _mt_Enable = 0;
    mt_end();

    /* Copy fast → chip */
    ULONG *src = (ULONG *)modules_fast[music_id];
    ULONG *dst = (ULONG *)module_chip_buffer;
    ULONG longs = (module_sizes[music_id] + 3) >> 2;
    while (longs--)
        *dst++ = *src++;

    mt_init(module_chip_buffer, NULL, 0);
    _mt_Enable = 1;
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