#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <hardware/custom.h>
#include <proto/exec.h>
#include "game.h"
#include "hardware.h"
#include "blitter.h"
#include "memory.h"
#include "timers.h"
#include "cars.h"
#include "motorbike.h"
#include "barreltruck.h"
 
extern volatile struct Custom *custom;
extern UBYTE *draw_buffer;
extern WORD mapposy;

/* ---- Blit params (same pattern as planes — mask stored after planes) ---- */
#define TRUCK_WIDTH_WORDS       3    /* 2 words visible + 1 shift overflow */
#define TRUCK_BLIT_LINES        (TRUCK_BOB_HEIGHT * 4)   /* interleaved */
#define TRUCK_SRC_MOD           0
#define TRUCK_MASK_OFFSET       ((TRUCK_WIDTH_WORDS << 1) * TRUCK_BOB_HEIGHT * 4)

#define STRIP_WIDTH_WORDS       2
#define STRIP_BLIT_LINES        (LEFT_STRIP_HEIGHT * 4)
#define STRIP_SRC_MOD           0
#define STRIP_MASK_OFFSET       ((STRIP_WIDTH_WORDS << 1) * LEFT_STRIP_HEIGHT * 4)

#define BARREL_WIDTH_WORDS      2
#define BARREL_BLIT_LINES       (BARREL_BOB_HEIGHT * 4)
#define BARREL_SRC_MOD          0
#define BARREL_MASK_OFFSET      ((BARREL_WIDTH_WORDS << 1) * BARREL_BOB_HEIGHT * 4)

#define BARREL_DROP_INTERVAL_MS 1500
#define BARREL_WORLD_SPEED      12    /* slow forward — ~0.5 px/frame */

/* ---- Truck state ---- */
static BOOL  truck_active = FALSE;
static WORD  truck_x, truck_y;
static WORD  truck_old_x, truck_old_y;
static WORD  truck_prev_old_x, truck_prev_old_y;
static UBYTE truck_needs_restore = 0;
static UBYTE truck_wheel_frame = 0;
static UBYTE truck_wheel_counter = 0;
static UBYTE barrel_zigzag_frame = 0;
static UBYTE barrel_zigzag_counter = 0;
static WORD  truck_speed = 0;
static WORD  truck_accumulator = 0;

static BOOL truck_pending = FALSE;
static WORD pending_x, pending_y;

/* ---- Drop state ---- */
static Barrel barrels[MAX_BARRELS];
static GameTimer barrel_drop_timer;
static UBYTE next_drop_side = BARREL_SIDE_LEFT;

/* ---- BOB assets ---- */
static UBYTE *truck_data_f1 = NULL;
static UBYTE *truck_data_f2 = NULL;
static UBYTE *left_strip_f1 = NULL;
static UBYTE *left_strip_f2 = NULL;
static UBYTE *right_strip_f1 = NULL;
static UBYTE *right_strip_f2 = NULL;
static UBYTE *barrel_data = NULL;

WORD barrel_truck_speed = 38;   /* slightly less than MIN_CRUISING_SPEED */

/* ---- Helpers ---- */

static UBYTE BarrelTruck_CountSide(UBYTE side)
{
    UBYTE count = 0;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (barrels[i].active && barrels[i].side == side)
            count++;
    }
    return count;
}

static void BarrelTruck_DropBarrel(void)
{
    UBYTE target = next_drop_side;
    
    /* If target side full, try other side */
    if (BarrelTruck_CountSide(target) >= MAX_BARRELS_PER_SIDE)
    {
        target ^= 1;
        if (BarrelTruck_CountSide(target) >= MAX_BARRELS_PER_SIDE)
            return;  /* Both sides full — skip this drop */
    }
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) { slot = i; break; }
    }
    if (slot < 0) return;
    
    Barrel *b = &barrels[slot];
    
    if (target == BARREL_SIDE_LEFT)
        b->x = truck_x + 4;
    else
        b->x = truck_x + TRUCK_BOB_WIDTH - BARREL_BOB_WIDTH - 4;
    
    b->y = truck_y + TRUCK_BOB_HEIGHT;
    b->old_x = b->x;
    b->old_y = b->y;
    b->prev_old_x = b->x;
    b->prev_old_y = b->y;
    b->accumulator = 0;
    b->side = target;
    b->needs_restore = 0;
    b->active = TRUE;
    
    next_drop_side ^= 1;   /* alternate for next drop */
    
    /* TODO: SFX_Play(SFX_BARREL_DROP); */
}

/* ---- Public API ---- */

void BarrelTruck_Initialize(void)
{
    truck_data_f1  = Disk_AllocAndLoadAsset(TRUCK_BOB_FILE1, MEMF_CHIP);
    truck_data_f2  = Disk_AllocAndLoadAsset(TRUCK_BOB_FILE2, MEMF_CHIP);
    left_strip_f1  = Disk_AllocAndLoadAsset(LEFT_STRIP_FILE1, MEMF_CHIP);
    left_strip_f2  = Disk_AllocAndLoadAsset(LEFT_STRIP_FILE2, MEMF_CHIP);
    right_strip_f1 = Disk_AllocAndLoadAsset(RIGHT_STRIP_FILE1, MEMF_CHIP);
    right_strip_f2 = Disk_AllocAndLoadAsset(RIGHT_STRIP_FILE2, MEMF_CHIP);
    barrel_data    = Disk_AllocAndLoadAsset(BARREL_BOB_FILE, MEMF_CHIP);
    
    BarrelTruck_Reset();
}

void BarrelTruck_Spawn(WORD x, WORD y)
{
    if (truck_active) return;
   
    truck_x = x;
    truck_y = y;
    truck_old_x = x;
    truck_old_y = y;
    truck_prev_old_x = x;
    truck_prev_old_y = y;
    truck_needs_restore = 0;
    truck_wheel_frame = 0;
    truck_wheel_counter = 0;
    barrel_zigzag_frame = 0;
    barrel_zigzag_counter = 0;
    truck_accumulator = 0;
    truck_speed = GetScrollAmount(barrel_truck_speed);
    
    next_drop_side = BARREL_SIDE_LEFT;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
        barrels[i].needs_restore = 0;
    }
    
    Timer_StartMs(&barrel_drop_timer, BARREL_DROP_INTERVAL_MS);
    truck_active = TRUE;
    
    KPrintF("BarrelTruck: spawned at (%ld, %ld)\n", (LONG)x, (LONG)y);
}

void BarrelTruck_Stop(void)
{
    truck_active = FALSE;
    truck_pending = FALSE;
    truck_needs_restore = 0;
    
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
        barrels[i].needs_restore = 0;
    }
    
    Timer_Stop(&barrel_drop_timer);
    
    Cars_EnableSpawning();   /* Cars start spawning again */
}

void BarrelTruck_Reset(void)
{
    BarrelTruck_Stop();
}

BOOL BarrelTruck_IsActive(void)
{
    return truck_active;
}

void BarrelTruck_Update(void)
{
    /* Pending — wait for cars to clear screen */
    if (truck_pending && !truck_active)
    {
        if (!Cars_AnyOnScreen())
        {
            truck_pending = FALSE;
            BarrelTruck_Spawn(pending_x, pending_y);
        }
        return;
    }

    if (!truck_active) return;
    
    /* Move truck forward — world Y decreases */
    truck_accumulator += truck_speed;
    if (truck_accumulator >= 256)
    {
        WORD pixels = truck_accumulator >> 8;
        truck_y -= pixels;
        truck_accumulator &= 0xFF;
    }
    
    /* Wheel animation */
    truck_wheel_counter++;
    if (truck_wheel_counter >= 6)
    {
        truck_wheel_counter = 0;
        truck_wheel_frame ^= 1;
    }
    
    /* Barrel zigzag animation (slightly slower than wheels) */
    barrel_zigzag_counter++;
    if (barrel_zigzag_counter >= 10)
    {
        barrel_zigzag_counter = 0;
        barrel_zigzag_frame ^= 1;
    }
    
    /* Truck fell off bottom of screen? */
    WORD truck_screen_y = truck_y - mapposy;
    if (truck_screen_y > SCREENHEIGHT + TRUCK_BOB_HEIGHT)
    {
        BarrelTruck_Stop();
        return;
    }
    
    /* Move dropped barrels forward slowly */
    WORD barrel_scroll = GetScrollAmount(BARREL_WORLD_SPEED);
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        barrels[i].accumulator += barrel_scroll;
        if (barrels[i].accumulator >= 256)
        {
            WORD pixels = barrels[i].accumulator >> 8;
            barrels[i].y -= pixels;
            barrels[i].accumulator &= 0xFF;
        }
        
        /* Barrel off bottom? */
        WORD barrel_screen_y = barrels[i].y - mapposy;
        if (barrel_screen_y > SCREENHEIGHT + BARREL_BOB_HEIGHT)
            barrels[i].active = FALSE;
    }
    
    /* Drop new barrel on interval */
    if (Timer_HasElapsed(&barrel_drop_timer))
    {
        BarrelTruck_DropBarrel();
        Timer_Reset(&barrel_drop_timer);
    }
}

void BarrelTruck_Restore(void)
{
    /* Restore truck (this also covers the barrel strips drawn on top) */
    if (truck_needs_restore > 0)
    {
        WORD old_screen_y = truck_prev_old_y - mapposy;
        if (old_screen_y >= -TRUCK_BOB_HEIGHT && old_screen_y < SCREENHEIGHT)
        {
            BlitRestoreBobs(draw_buffer, truck_prev_old_x, old_screen_y,
                       TRUCK_WIDTH_WORDS, TRUCK_BLIT_LINES);
        }
        truck_needs_restore--;
    }
    
    /* Restore dropped barrels */
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (barrels[i].needs_restore == 0) continue;
        
        WORD old_screen_y = barrels[i].prev_old_y - mapposy;
        if (old_screen_y >= -BARREL_BOB_HEIGHT && old_screen_y < SCREENHEIGHT)
        {
            BlitRestoreBobs(draw_buffer, barrels[i].prev_old_x, old_screen_y,
                       BARREL_WIDTH_WORDS, BARREL_BLIT_LINES);
        }
        barrels[i].needs_restore--;
    }
}

void BarrelTruck_RequestSpawn(WORD x, WORD y)
{
    if (truck_active || truck_pending) return;
    
    truck_pending = TRUE;
    pending_x = x;
    pending_y = y;
    
    Cars_DisableSpawning();  /* Stop new cars — existing ones finish naturally */
}

void BarrelTruck_Draw(void)
{
    if (!truck_active) return;
    
    /* Track positions for double-buffer restore */
    truck_prev_old_x = truck_old_x;
    truck_prev_old_y = truck_old_y;
    truck_old_x = truck_x;
    truck_old_y = truck_y;
    
    WORD truck_screen_y = truck_y - mapposy;
    
    /* Truck body */
    if (truck_screen_y >= -TRUCK_BOB_HEIGHT && truck_screen_y < SCREENHEIGHT)
    {
        UBYTE *tsrc  = (truck_wheel_frame == 0) ? truck_data_f1 : truck_data_f2;
        UBYTE *tmask = tsrc + TRUCK_MASK_OFFSET;
        
        UWORD tsrc_mod = TRUCK_SRC_MOD;
        UWORD tdst_mod = BITMAPBYTESPERROW - (TRUCK_WIDTH_WORDS << 1);
        ULONG tadmod = ((ULONG)tdst_mod << 16) | tsrc_mod;
        UWORD tbltsize = (TRUCK_BLIT_LINES << 6) | TRUCK_WIDTH_WORDS;
        
        BlitBob2(SCREENWIDTH_WORDS, truck_x, truck_screen_y,
                 tadmod, tbltsize, TRUCK_BOB_WIDTH,
                 NULL, tsrc, tmask, draw_buffer);
        
        /* Left barrel strip */
        UBYTE *lsrc  = (barrel_zigzag_frame == 0) ? left_strip_f1 : left_strip_f2;
        UBYTE *lmask = lsrc + STRIP_MASK_OFFSET;
        
        UWORD ssrc_mod = STRIP_SRC_MOD;
        UWORD sdst_mod = BITMAPBYTESPERROW - (STRIP_WIDTH_WORDS << 1);
        ULONG sadmod = ((ULONG)sdst_mod << 16) | ssrc_mod;
        UWORD sbltsize = (STRIP_BLIT_LINES << 6) | STRIP_WIDTH_WORDS;
        
        BlitBob2(SCREENWIDTH_WORDS,
                 truck_x + LEFT_STRIP_OFFSET_X,
                 truck_screen_y + LEFT_STRIP_OFFSET_Y,
                 sadmod, sbltsize, LEFT_STRIP_WIDTH,
                 NULL, lsrc, lmask, draw_buffer);
        
        /* Right barrel strip (uses the OPPOSITE zigzag frame for counter-sway) */
        UBYTE *rsrc  = (barrel_zigzag_frame == 0) ? right_strip_f2 : right_strip_f1;
        UBYTE *rmask = rsrc + STRIP_MASK_OFFSET;
        
        BlitBob2(SCREENWIDTH_WORDS,
                 truck_x + RIGHT_STRIP_OFFSET_X,
                 truck_screen_y + RIGHT_STRIP_OFFSET_Y,
                 sadmod, sbltsize, RIGHT_STRIP_WIDTH,
                 NULL, rsrc, rmask, draw_buffer);
        
        truck_needs_restore = 2;
    }
    
    /* Dropped barrels */
    UBYTE *bsrc  = barrel_data;
    UBYTE *bmask = bsrc + BARREL_MASK_OFFSET;
    
    UWORD bsrc_mod = BARREL_SRC_MOD;
    UWORD bdst_mod = BITMAPBYTESPERROW - (BARREL_WIDTH_WORDS << 1);
    ULONG badmod = ((ULONG)bdst_mod << 16) | bsrc_mod;
    UWORD bbltsize = (BARREL_BLIT_LINES << 6) | BARREL_WIDTH_WORDS;
    
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        barrels[i].prev_old_x = barrels[i].old_x;
        barrels[i].prev_old_y = barrels[i].old_y;
        barrels[i].old_x = barrels[i].x;
        barrels[i].old_y = barrels[i].y;
        
        WORD barrel_screen_y = barrels[i].y - mapposy;
        if (barrel_screen_y >= -BARREL_BOB_HEIGHT && barrel_screen_y < SCREENHEIGHT)
        {
            BlitBob2(SCREENWIDTH_WORDS, barrels[i].x, barrel_screen_y,
                     badmod, bbltsize, BARREL_BOB_WIDTH,
                     NULL, bsrc, bmask, draw_buffer);
            barrels[i].needs_restore = 2;
        }
    }
}

BOOL BarrelTruck_CheckCollision(WORD bike_cx, WORD bike_top)
{
    /* Truck body itself is solid */
    if (truck_active)
    {
        WORD truck_screen_y = truck_y - mapposy;
        if (truck_screen_y > -TRUCK_BOB_HEIGHT && truck_screen_y < SCREENHEIGHT)
        {
            WORD tx_dist = ABS(bike_cx - (truck_x + (TRUCK_BOB_WIDTH >> 1)));
            WORD ty_dist = ABS(bike_top + 14 - (truck_screen_y + (TRUCK_BOB_HEIGHT >> 1)));
            if (tx_dist < (TRUCK_BOB_WIDTH >> 1) && ty_dist < (TRUCK_BOB_HEIGHT >> 1))
                return TRUE;
        }
    }
    
    /* Dropped barrels */
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        WORD barrel_screen_y = barrels[i].y - mapposy;
        if (barrel_screen_y < -BARREL_BOB_HEIGHT || barrel_screen_y > SCREENHEIGHT)
            continue;
        
        WORD x_dist = ABS(bike_cx - (barrels[i].x + (BARREL_BOB_WIDTH >> 1)));
        WORD y_dist = ABS(bike_top + 14 - (barrel_screen_y + (BARREL_BOB_HEIGHT >> 1)));
        if (x_dist < 12 && y_dist < 16)
            return TRUE;
    }
    
    return FALSE;
}
 