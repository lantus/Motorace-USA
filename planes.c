#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include "game.h"
#include "map.h"
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "sprites.h"
#include "font.h"
#include "roadsystem.h"
#include "pak.h"
#include "city_approach.h"
#include "blitter.h"
#include "planes.h"
 
extern volatile struct Custom *custom;

/* ---- BOB data ---- */
static UBYTE *plane_data_f1;
static UBYTE *plane_data_f2;
static UBYTE *plane_rev_f1;
static UBYTE *plane_rev_f2;
static UBYTE *smoke_data;

/* ---- Common state ---- */
static BOOL planes_active = FALSE;
static BOOL planes_done = FALSE;
static UBYTE plane_anim_frame = 0;
static GameTimer plane_anim_timer;
static UBYTE plane_mode = PLANE_MODE_HOUSTON;

/* ---- Houston mode ---- */
static const WORD plane_init_x[NUM_PLANES]  = { -24, -32 };
static const WORD plane_y_pos[NUM_PLANES]   = {  8,  20 };
static const WORD plane_speeds[NUM_PLANES]  = {  3,   3 };

#define PLANE_EXIT_X      (192 + 16)

#define PLANE_WIDTH_WORDS   2
#define PLANE_SRC_MOD       (PLANE_WIDTH_WORDS * 2)
#define PLANE_MASK_OFFSET   (PLANE_WIDTH_WORDS * 2)
#define PLANE_BLIT_LINES    (PLANE_BOB_HEIGHT * PLANE_BOB_DEPTH)

typedef struct {
    WORD x, y;
    WORD old_x, old_y;
    WORD prev_old_x, prev_old_y;
    BOOL needs_restore;
    BOOL prev_needs_restore;
} PlaneInstance;

static PlaneInstance plane_inst[NUM_PLANES];
static UBYTE nyc_loop_delay = 0;

/* ---- NYC flyover mode ---- */
typedef struct {
    WORD x, y;
    WORD speed;
    BOOL drop_smoke;
    BYTE facing;      /* -1 = left (rev sprites), +1 = right (normal) */
    BYTE action;       /* 0=move, 1=start loop 1, 2=start loop 2 */
} Waypoint;

 
#define LOOP_STEPS 16

 
static const WORD loop1_x[LOOP_STEPS] = {
    152, 155, 158, 160, 162, 164, 165, 166,
    166, 165, 164, 162, 160, 158, 155, 152
};
static const WORD loop1_y[LOOP_STEPS] = {
     16,  16,  15,  14,  13,  12,  10,   9,   
     7,   6,   4,   3,   2,   1,   0,   0
};

 
static const WORD loop2_x[LOOP_STEPS] = {
     40,  35,  30,  26,  22,  19,  17,  16,  
     16,  17,  19,  22,  26,  30,  35,  40
};
static const WORD loop2_y[LOOP_STEPS] = {
      0,  0,  1,  2,  3,  4,  6,  7,  9,  10,  12,  13,  14,  15,  16,   16
};

/* Facing direction flips at midpoint of each loop */
#define LOOP_FLIP_STEP  8
#define ACTION_MOVE     0
#define ACTION_LOOP1    1
#define ACTION_LOOP2    2

static const Waypoint nyc_path[] = {
   
    { -16,  16,   0,  FALSE,  +1,  ACTION_MOVE },   /*  0: start */
    {  48,  16,   2,  TRUE,   +1,  ACTION_MOVE },   /*  1: smoke start */
    {  72,  16,   2,  TRUE,   +1,  ACTION_MOVE },   /*  2: */
    {  96,  16,   2,  TRUE,   +1,  ACTION_MOVE },   /*  3: */
    { 128,  16,   2,  TRUE,   +1,  ACTION_MOVE },   /*  4: smoke end */
    { 152,  16,   1,  FALSE,  +1,  ACTION_LOOP1 },  /*  5: enter loop 1 */

    
    { 128,   0,   2,  FALSE,  -1,  ACTION_MOVE },   /*  6: */
    {  96,   0,   2,  FALSE,  -1,  ACTION_MOVE },   /*  7: center */
    {  64,   0,   2,  FALSE,  -1,  ACTION_MOVE },   /*  8: */
    {  40,   0,   1,  FALSE,  -1,  ACTION_LOOP2 },  /*  9: enter loop 2 */

   
    {  48,  16,   2,  FALSE,  +1,  ACTION_MOVE },   /* 10: into smoke */
    {  72,  16,   2,  FALSE,  +1,  ACTION_MOVE },   /* 11: */
    {  96,  16,   2,  FALSE,  +1,  ACTION_MOVE },   /* 12: */
    { 128,  16,   2,  FALSE,  +1,  ACTION_MOVE },   /* 13: */
    { 152,  16,   2,  FALSE,  +1,  ACTION_MOVE },   /* 14: trigger VIVA!NY */
    { 210,  16,   3,  FALSE,  +1,  ACTION_MOVE },   /* 15: exit right */
};

#define NUM_NYC_WAYPOINTS       (sizeof(nyc_path) / sizeof(nyc_path[0]))
#define VIVANY_TRIGGER_WAYPOINT  14

static BOOL  nyc_in_loop = FALSE;
static UBYTE nyc_loop_step = 0;
static UBYTE nyc_loop_id = 0;   /* 1 or 2 */

static WORD  nyc_plane_x, nyc_plane_y;
static WORD  nyc_old_x, nyc_old_y;
static WORD  nyc_prev_old_x, nyc_prev_old_y;
static UBYTE nyc_needs_restore = 0;
static BYTE  nyc_facing = +1;
static UBYTE nyc_waypoint = 0;
static BOOL  nyc_vivany_triggered = FALSE;

/* ---- Smoke puffs ---- */
typedef struct {
    WORD  x, y;
    WORD  old_x, old_y;
    WORD  prev_old_x, prev_old_y;
    UBYTE needs_restore;
    BOOL  active;
} SmokePuff;

static SmokePuff smoke[MAX_SMOKE_PUFFS];
static UBYTE next_smoke = 0;
static GameTimer smoke_timer;

/* ============================================================ */
/*  INITIALIZATION                                               */
/* ============================================================ */

void Planes_Initialize(void)
{
    plane_data_f1 = Pak_LoadAsset(PLANE_BPL1, MEMF_CHIP);
    plane_data_f2 = Pak_LoadAsset(PLANE_BPL2, MEMF_CHIP);
    plane_rev_f1  = Pak_LoadAsset(PLANE_REV_BPL1, MEMF_CHIP);
    plane_rev_f2  = Pak_LoadAsset(PLANE_REV_BPL2, MEMF_CHIP);
    smoke_data    = Pak_LoadAsset(SMOKE_BPL, MEMF_CHIP);
}

/* ============================================================ */
/*  HOUSTON MODE                                                 */
/* ============================================================ */

void Planes_Start(void)
{
    if (planes_done) return;

    for (int i = 0; i < NUM_PLANES; i++)
    {
        plane_inst[i].x = plane_init_x[i];
        plane_inst[i].y = plane_y_pos[i];
        plane_inst[i].needs_restore = FALSE;
        plane_inst[i].prev_needs_restore = FALSE;
    }

    plane_anim_frame = 0;
    Timer_StartMs(&plane_anim_timer, 120);
    plane_mode = PLANE_MODE_HOUSTON;
    planes_active = TRUE;

    Copper_SetPalette(12, 0xD00);
}

/* ============================================================ */
/*  NYC FLYOVER MODE                                             */
/* ============================================================ */

void Planes_StartNYC(void)
{
    if (planes_done) return;

    nyc_plane_x = nyc_path[0].x;
    nyc_plane_y = nyc_path[0].y;
    nyc_old_x = nyc_plane_x;
    nyc_old_y = nyc_plane_y;
    nyc_prev_old_x = nyc_plane_x;
    nyc_prev_old_y = nyc_plane_y;
    nyc_needs_restore = 0;
    nyc_facing = +1;
    nyc_waypoint = 0;
    nyc_vivany_triggered = FALSE;
    nyc_loop_delay = 0;

    nyc_in_loop = FALSE;
    nyc_loop_step = 0;
    nyc_loop_id = 0;

    for (int i = 0; i < MAX_SMOKE_PUFFS; i++)
    {
        smoke[i].active = FALSE;
        smoke[i].needs_restore = 0;
    }
    next_smoke = 0;

    plane_anim_frame = 0;
    Timer_StartMs(&plane_anim_timer, 120);
    Timer_StartMs(&smoke_timer, 60);
    plane_mode = PLANE_MODE_NYC;
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

BOOL Planes_IsComplete(void)
{
    return planes_done;
}
 

static void DropSmoke(WORD x, WORD y)
{
    if (next_smoke >= MAX_SMOKE_PUFFS) return;
    
    SmokePuff *s = &smoke[next_smoke];
    s->x = x;
    s->y = y;
    s->old_x = x;
    s->old_y = y;
    s->prev_old_x = x;
    s->prev_old_y = y;
    s->needs_restore = 0;
    s->active = TRUE;
    next_smoke++;
}
 
 
static void Planes_UpdateHouston(void)
{
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

static void Planes_UpdateNYC(void)
{
    /* ---- Loop mode: step through precalculated curve ---- */
    if (nyc_in_loop)
    {
        nyc_loop_delay++;
        if (nyc_loop_delay < 1)
            return;
        nyc_loop_delay = 0;

        const WORD *lx = (nyc_loop_id == 1) ? loop1_x : loop2_x;
        const WORD *ly = (nyc_loop_id == 1) ? loop1_y : loop2_y;
        
        nyc_plane_x = lx[nyc_loop_step];
        nyc_plane_y = ly[nyc_loop_step];
        
        /* Flip facing at midpoint */
        if (nyc_loop_step < LOOP_FLIP_STEP)
            nyc_facing = (nyc_loop_id == 1) ? +1 : -1;
        else
            nyc_facing = (nyc_loop_id == 1) ? -1 : +1;
        
        nyc_loop_step++;
        if (nyc_loop_step >= LOOP_STEPS)
        {
            nyc_in_loop = FALSE;
            nyc_waypoint++;  /* Resume next waypoint after loop */
        }
        return;
    }

    /* ---- Waypoint mode ---- */
    if (nyc_waypoint >= NUM_NYC_WAYPOINTS)
    {
        Planes_Stop();
        return;
    }

    const Waypoint *wp = &nyc_path[nyc_waypoint];
    WORD dx = wp->x - nyc_plane_x;
    WORD dy = wp->y - nyc_plane_y;
    WORD speed = wp->speed;

    if (dx > speed) nyc_plane_x += speed;
    else if (dx < -speed) nyc_plane_x -= speed;
    else nyc_plane_x = wp->x;

    if (dy > speed) nyc_plane_y += speed;
    else if (dy < -speed) nyc_plane_y -= speed;
    else nyc_plane_y = wp->y;

    nyc_facing = wp->facing;

    /* Drop smoke */
    if (wp->drop_smoke && Timer_HasElapsed(&smoke_timer))
    {
        WORD smoke_x = (nyc_facing > 0) ? nyc_plane_x - 8 : nyc_plane_x + 16;
        if (smoke_x >= SMOKE_START_X && smoke_x <= SMOKE_END_X)
        {
            DropSmoke(smoke_x, SMOKE_Y);
        }
        Timer_Reset(&smoke_timer);
    }

    /* Reached waypoint? */
    if (nyc_plane_x == wp->x && nyc_plane_y == wp->y)
    {
        /* VIVA!NY trigger */
        if (nyc_waypoint == VIVANY_TRIGGER_WAYPOINT && !nyc_vivany_triggered)
        {
            NYCVictory_Start();
            nyc_vivany_triggered = TRUE;
        }

        /* Check for loop entry */
        if (wp->action == ACTION_LOOP1)
        {
            nyc_in_loop = TRUE;
            nyc_loop_step = 0;
            nyc_loop_id = 1;
            return;
        }
        else if (wp->action == ACTION_LOOP2)
        {
            nyc_in_loop = TRUE;
            nyc_loop_step = 0;
            nyc_loop_id = 2;
            return;
        }

        nyc_waypoint++;
        if (nyc_waypoint >= NUM_NYC_WAYPOINTS)
        {
            Planes_Stop();
            return;
        }
    }
}

void Planes_Update(void)
{
    if (!planes_active) return;

    if (Timer_HasElapsed(&plane_anim_timer))
    {
        plane_anim_frame ^= 1;
        Timer_Reset(&plane_anim_timer);
    }

    if (plane_mode == PLANE_MODE_HOUSTON)
        Planes_UpdateHouston();
    else
        Planes_UpdateNYC();
}
 
void Planes_Restore(UBYTE *buffer)
{
    if (plane_mode == PLANE_MODE_HOUSTON)
    {
        for (int i = 0; i < NUM_PLANES; i++)
        {
            if (!plane_inst[i].prev_needs_restore) continue;
            
            if (plane_inst[i].prev_old_x < 0 || plane_inst[i].prev_old_x >= VIEWPORT_WIDTH)
            {
                plane_inst[i].prev_needs_restore = FALSE;
                continue;
            }

            /* Clip restore width too */
            WORD restore_words = PLANE_WIDTH_WORDS;
            WORD dest_last = (plane_inst[i].prev_old_x >> 4) + restore_words;
            WORD vp_words = VIEWPORT_WIDTH >> 4;
            
            if (dest_last > vp_words)
                restore_words = vp_words - (plane_inst[i].prev_old_x >> 4);
            
            if (restore_words <= 0)
            {
                plane_inst[i].prev_needs_restore = FALSE;
                continue;
            }
            
            BlitRestoreBobs(buffer, plane_inst[i].prev_old_x, plane_inst[i].prev_old_y,
                       restore_words, PLANE_BLIT_LINES);
            plane_inst[i].prev_needs_restore = FALSE;
        }
    }
    else
    {
        if (nyc_needs_restore > 0)
        {
            if (nyc_prev_old_x < 0 || nyc_prev_old_x >= VIEWPORT_WIDTH)
            {
                nyc_needs_restore = 0;
            }
            else
            {
                WORD restore_words = PLANE_WIDTH_WORDS;
                WORD dest_last = (nyc_prev_old_x >> 4) + restore_words;
                WORD vp_words = VIEWPORT_WIDTH >> 4;
                
                if (dest_last > vp_words)
                    restore_words = vp_words - (nyc_prev_old_x >> 4);
                
                if (restore_words > 0)
                {
                    BlitRestoreBobs(buffer, nyc_prev_old_x, nyc_prev_old_y,
                               restore_words, PLANE_BLIT_LINES);
                }
                nyc_needs_restore--;
            }
        }

        for (int i = 0; i < MAX_SMOKE_PUFFS; i++)
        {
            if (smoke[i].needs_restore == 0) continue;
            
            if (smoke[i].prev_old_x < 0 || smoke[i].prev_old_x >= VIEWPORT_WIDTH)
            {
                smoke[i].needs_restore = 0;
                continue;
            }
            
            WORD restore_words = PLANE_WIDTH_WORDS;
            WORD dest_last = (smoke[i].prev_old_x >> 4) + restore_words;
            WORD vp_words = VIEWPORT_WIDTH >> 4;
            
            if (dest_last > vp_words)
                restore_words = vp_words - (smoke[i].prev_old_x >> 4);
            
            if (restore_words > 0)
            {
                BlitRestoreBobs(buffer, smoke[i].prev_old_x, smoke[i].prev_old_y,
                           restore_words, PLANE_BLIT_LINES);
            }
            smoke[i].needs_restore--;
        }
    }
}
 

void Planes_Draw(UBYTE *buffer)
{
    if (!planes_active) return;

    if (plane_mode == PLANE_MODE_HOUSTON)
    {
        for (int i = 0; i < NUM_PLANES; i++)
        {
            plane_inst[i].prev_old_x = plane_inst[i].old_x;
            plane_inst[i].prev_old_y = plane_inst[i].old_y;
            plane_inst[i].prev_needs_restore = plane_inst[i].needs_restore;

            if (plane_inst[i].x < 0 || plane_inst[i].x >= VIEWPORT_WIDTH)
            {
                plane_inst[i].needs_restore = FALSE;
                continue;
            }

            UBYTE *source = (plane_anim_frame == 0) ? plane_data_f1 : plane_data_f2;
            UBYTE *mask = source + PLANE_MASK_OFFSET;

            UWORD source_mod = PLANE_SRC_MOD;
            UWORD dest_mod   = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
            ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
            UWORD bltsize = (PLANE_BLIT_LINES << 6) | PLANE_WIDTH_WORDS;

            BlitBob2Clip(SCREENWIDTH_WORDS, plane_inst[i].x, plane_inst[i].y,
                     admod, bltsize, PLANE_BOB_WIDTH,
                     NULL, source, mask, buffer);

            plane_inst[i].old_x = plane_inst[i].x;
            plane_inst[i].old_y = plane_inst[i].y;
            plane_inst[i].needs_restore = TRUE;
        }
    }
    else
    {
        /* ---- NYC plane ---- */
        nyc_prev_old_x = nyc_old_x;
        nyc_prev_old_y = nyc_old_y;
        nyc_old_x = nyc_plane_x;
        nyc_old_y = nyc_plane_y;

        if (nyc_plane_x >= 0 && nyc_plane_x < VIEWPORT_WIDTH &&
            nyc_plane_y >= -16 && nyc_plane_y < 60)
        {
            UBYTE *source;
            if (nyc_facing > 0)
                source = (plane_anim_frame == 0) ? plane_data_f1 : plane_data_f2;
            else
                source = (plane_anim_frame == 0) ? plane_rev_f1 : plane_rev_f2;

            UBYTE *mask = source + PLANE_MASK_OFFSET;

            UWORD source_mod = PLANE_SRC_MOD;
            UWORD dest_mod   = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
            ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
            UWORD bltsize = (PLANE_BLIT_LINES << 6) | PLANE_WIDTH_WORDS;

            BlitBob2Clip(SCREENWIDTH_WORDS, nyc_plane_x, nyc_plane_y,
                     admod, bltsize, PLANE_BOB_WIDTH,
                     NULL, source, mask, buffer);

            nyc_needs_restore = 2;
        }

        /* ---- Smoke puffs ---- */
        for (int i = 0; i < MAX_SMOKE_PUFFS; i++)
        {
            if (!smoke[i].active) continue;

            smoke[i].prev_old_x = smoke[i].old_x;
            smoke[i].prev_old_y = smoke[i].old_y;
            smoke[i].old_x = smoke[i].x;
            smoke[i].old_y = smoke[i].y;

            if (smoke[i].x >= 0 && smoke[i].x < VIEWPORT_WIDTH &&
                smoke[i].y >= 0 && smoke[i].y < 60)
            {
                UBYTE *src = smoke_data;
                UBYTE *smask = src + PLANE_MASK_OFFSET;

                UWORD source_mod = PLANE_SRC_MOD;
                UWORD dest_mod   = BITMAPBYTESPERROW - (PLANE_WIDTH_WORDS << 1);
                ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
                UWORD bltsize = (PLANE_BLIT_LINES << 6) | PLANE_WIDTH_WORDS;

                BlitBob2(SCREENWIDTH_WORDS, smoke[i].x, smoke[i].y,
                         admod, bltsize, SMOKE_BOB_WIDTH,
                         NULL, src, smask, buffer);

                smoke[i].needs_restore = 2;
            }
        }
    }
}