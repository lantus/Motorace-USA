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
#include "hiscore_entry.h"
#include "blitter.h"
#include "city_approach.h"
#include "roadsystem.h"
#include "ranking.h"
#include "stageprogress.h"
#include "fuel.h"
#include "planes.h"

extern volatile struct Custom *custom;
 
static UBYTE *plane_data_f1;
static UBYTE *plane_data_f2;

static BOOL planes_active = FALSE;
static BOOL planes_done = FALSE;
static UBYTE plane_anim_frame = 0;
static GameTimer plane_anim_timer;

static const WORD plane_init_x[NUM_PLANES]  = { -24, -32 };
static const WORD plane_y_pos[NUM_PLANES]   = {  8,  20 };
static const WORD plane_speeds[NUM_PLANES]  = {  3,   3 };

#define PLANE_EXIT_X      192+16

#define PLANE_WIDTH_WORDS   2
#define PLANE_SRC_MOD       (PLANE_WIDTH_WORDS * 2)   /* 4 — skip mask after image */
#define PLANE_MASK_OFFSET   (PLANE_WIDTH_WORDS * 2)   /* 4 — mask starts here */
#define PLANE_BLIT_LINES    (PLANE_BOB_HEIGHT * PLANE_BOB_DEPTH)  /* 64 */

typedef struct {
    WORD x, y;
    WORD old_x, old_y;
    WORD prev_old_x, prev_old_y;
    BOOL needs_restore;
    BOOL prev_needs_restore;
} PlaneInstance;
 
static PlaneInstance plane_inst[NUM_PLANES];

void Planes_Initialize(void)
{
    plane_data_f1 = Disk_AllocAndLoadAsset(PLANE_BPL1, MEMF_CHIP);
    plane_data_f2 = Disk_AllocAndLoadAsset(PLANE_BPL2, MEMF_CHIP);
}

void Planes_Start(void)
{
    if (planes_done) return;

    for (int i = 0; i < NUM_PLANES; i++)
    {
        plane_inst[i].x     = plane_init_x[i];
        plane_inst[i].y     = plane_y_pos[i];
        plane_inst[i].needs_restore = FALSE;
        plane_inst[i].prev_needs_restore = FALSE;
    }

    plane_anim_frame = 0;
    Timer_StartMs(&plane_anim_timer, 120);
    planes_active = TRUE;

    Copper_SetPalette(12, 0xD00);
}
void Planes_Stop(void)
{
    planes_active = FALSE;
    planes_done = TRUE;
}

void Planes_ResetDone(void)
{
    planes_done = FALSE;
}

BOOL Planes_IsActive(void)
{
    return planes_active;
}

void Planes_Update(void)
{
    if (!planes_active) return;

    if (Timer_HasElapsed(&plane_anim_timer))
    {
        plane_anim_frame ^= 1;
        Timer_Reset(&plane_anim_timer);
    }

    BOOL any_visible = FALSE;
    for (int i = 0; i < NUM_PLANES; i++)
    {
        plane_inst[i].x += plane_speeds[i];
        if (plane_inst[i].x < PLANE_EXIT_X)
            any_visible = TRUE;
    }

    if (!any_visible)
        Planes_Stop();
}

void Planes_Restore(UBYTE *buffer)
{
    if (!planes_active) return;

    for (int i = 0; i < NUM_PLANES; i++)
    {
        if (!plane_inst[i].prev_needs_restore) continue;

        WORD x_byte = (plane_inst[i].prev_old_x >> 3) & 0xFFFE;
        WORD y4 = plane_inst[i].prev_old_y << 2;
        LONG offset = ((LONG)y4 << 4) + ((LONG)y4 << 3) + x_byte;

        WaitBlit();
        custom->bltcon0 = 0x09F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
        custom->bltdmod = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
        custom->bltapt  = screen.pristine + offset;
        custom->bltdpt  = buffer + offset;
        custom->bltsize = (PLANE_BLIT_LINES << 6) | PLANE_WIDTH_WORDS;
    }
}

void Planes_Draw(UBYTE *buffer)
{
    if (!planes_active) return;

    for (int i = 0; i < NUM_PLANES; i++)
    {
        /* Shift: this buffer's last draw becomes prev for next restore */
        plane_inst[i].prev_old_x = plane_inst[i].old_x;
        plane_inst[i].prev_old_y = plane_inst[i].old_y;
        plane_inst[i].prev_needs_restore = plane_inst[i].needs_restore;

        if (plane_inst[i].x < 0 || plane_inst[i].x >= VIEWPORT_WIDTH)
        {
            plane_inst[i].needs_restore = FALSE;
            continue;
        }

        UBYTE *source = (plane_anim_frame == 0)
            ? plane_data_f1
            : plane_data_f2;

        UBYTE *mask = source + PLANE_MASK_OFFSET;

        UWORD source_mod = PLANE_SRC_MOD;
        UWORD dest_mod   = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
        ULONG admod = ((ULONG)dest_mod << 16) | source_mod;

        UWORD bltsize = (PLANE_BLIT_LINES << 6) | PLANE_WIDTH_WORDS;

        BlitBob2(SCREENWIDTH_WORDS, plane_inst[i].x, plane_inst[i].y,
                 admod, bltsize, PLANE_BOB_WIDTH,
                 NULL, source, mask, buffer);

        plane_inst[i].old_x = plane_inst[i].x;
        plane_inst[i].old_y = plane_inst[i].y;
        plane_inst[i].needs_restore = TRUE;
    }
}