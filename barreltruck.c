#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include "game.h"
#include "hardware.h"
#include "blitter.h"
#include "memory.h"
#include "timers.h"
#include "cars.h"
#include "motorbike.h"
#include "barreltruck.h"

#if BARRELTRUCK_ENABLED

extern volatile struct Custom *custom;

/* ---- Truck state ---- */
static BOOL  truck_active = FALSE;
static WORD  truck_x, truck_y;
static WORD  truck_old_x, truck_old_y;
static WORD  truck_prev_old_x, truck_prev_old_y;
static UBYTE truck_needs_restore = 0;
static UBYTE truck_anim_frame = 0;
static UBYTE truck_anim_counter = 0;
static WORD  truck_speed = 768;         /* 8.8 fixed point — same system as cars */
static WORD  truck_accumulator = 0;

/* ---- Barrel state ---- */
static Barrel barrels[MAX_BARRELS];
static GameTimer barrel_drop_timer;
static UBYTE next_barrel = 0;

/* ---- BOB data (chip RAM) ---- */
static UBYTE *truck_data_f1 = NULL;
static UBYTE *truck_data_f2 = NULL;
static UBYTE *barrel_data = NULL;

/* ---- Barrel drop interval (frames) ---- */
#define BARREL_DROP_INTERVAL_MS  1500

WORD barrel_truck_speed = 41;

void BarrelTruck_Initialize(void)
{
    truck_data_f1 = Disk_AllocAndLoadAsset(TRUCK_BOB_FILE1, MEMF_CHIP);
    truck_data_f2 = Disk_AllocAndLoadAsset(TRUCK_BOB_FILE2, MEMF_CHIP);
    barrel_data   = Disk_AllocAndLoadAsset(BARREL_BOB_FILE, MEMF_CHIP);
    
    truck_active = FALSE;
    
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
        barrels[i].needs_restore = 0;
    }
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
    truck_anim_frame = 0;
    truck_anim_counter = 0;
    truck_accumulator = 0;
    truck_speed = GetScrollAmount(barrel_truck_speed);
    
    next_barrel = 0;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
        barrels[i].needs_restore = 0;
    }
    
    Timer_StartMs(&barrel_drop_timer, BARREL_DROP_INTERVAL_MS);
    truck_active = TRUE;

    KPrintF("BarrelTruck: spawned at (%d, %d)\n", x, y);
}

void BarrelTruck_Stop(void)
{
    truck_active = FALSE;
    
    for (int i = 0; i < MAX_BARRELS; i++)
        barrels[i].active = FALSE;
    
    Timer_Stop(&barrel_drop_timer);
}

void BarrelTruck_Reset(void)
{
    BarrelTruck_Stop();
    truck_needs_restore = 0;
    
    for (int i = 0; i < MAX_BARRELS; i++)
        barrels[i].needs_restore = 0;
}

BOOL BarrelTruck_IsActive(void)
{
    return truck_active;
}

static void BarrelTruck_DropBarrel(void)
{
    Barrel *b = &barrels[next_barrel];
    
    /* Drop barrel at truck's current rear position */
    b->x = truck_x + (TRUCK_BOB_WIDTH / 2) - (BARREL_BOB_WIDTH / 2);
    b->y = truck_y + TRUCK_BOB_HEIGHT;
    b->old_x = b->x;
    b->old_y = b->y;
    b->prev_old_x = b->x;
    b->prev_old_y = b->y;
    b->needs_restore = 0;
    b->active = TRUE;
    
    next_barrel = (next_barrel + 1) % MAX_BARRELS;
    
    /* TODO: SFX_Play(SFX_BARREL_DROP); */
}

void BarrelTruck_Update(void)
{
    if (!truck_active) return;
    
    /* ---- Move truck forward (decreasing Y, same as cars) ---- */
    truck_accumulator += truck_speed;
    if (truck_accumulator >= 256)
    {
        WORD pixels = truck_accumulator >> 8;
        truck_y -= pixels;
        truck_accumulator &= 0xFF;
    }
    
    /* ---- Wheel animation ---- */
    truck_anim_counter++;
    if (truck_anim_counter >= 6)
    {
        truck_anim_counter = 0;
        truck_anim_frame ^= 1;
    }
    
    /* ---- Check if truck has left the screen ---- */
    WORD truck_screen_y = truck_y - mapposy;
    if (truck_screen_y < -TRUCK_BOB_HEIGHT)
    {
        BarrelTruck_Stop();
        return;
    }
    
    /* ---- Drop barrels at interval ---- */
    if (Timer_HasElapsed(&barrel_drop_timer))
    {
        BarrelTruck_DropBarrel();
        Timer_Reset(&barrel_drop_timer);
    }
    
    /* ---- Barrels don't move — they stay where dropped ---- */
    /* Check if barrels have scrolled off screen */
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        WORD barrel_screen_y = barrels[i].y - mapposy;
        if (barrel_screen_y > SCREENHEIGHT + BARREL_BOB_HEIGHT)
        {
            barrels[i].active = FALSE;
        }
    }
}

void BarrelTruck_Restore(void)
{
    /* ---- Restore truck ---- */
    if (truck_needs_restore > 0)
    {
        WORD old_screen_y = truck_prev_old_y - mapposy;
        if (old_screen_y >= -TRUCK_BOB_HEIGHT && old_screen_y < SCREENHEIGHT + TRUCK_BOB_HEIGHT)
        {
            /* TODO: Restore truck BOB from pristine buffer */
            /* Same pattern as Cars_CopyPristineBackground but with TRUCK dimensions */
        }
        truck_needs_restore--;
    }
    
    /* ---- Restore barrels ---- */
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (barrels[i].needs_restore == 0) continue;
        
        WORD old_screen_y = barrels[i].prev_old_y - mapposy;
        if (old_screen_y >= -BARREL_BOB_HEIGHT && old_screen_y < SCREENHEIGHT + BARREL_BOB_HEIGHT)
        {
            /* TODO: Restore barrel BOB from pristine buffer */
        }
        barrels[i].needs_restore--;
    }
}

void BarrelTruck_Draw(void)
{
    if (!truck_active) return;
    
    /* ---- Track positions for double-buffer restore ---- */
    truck_prev_old_x = truck_old_x;
    truck_prev_old_y = truck_old_y;
    truck_old_x = truck_x;
    truck_old_y = truck_y;
    
    /* ---- Draw truck BOB ---- */
    WORD truck_screen_y = truck_y - mapposy;
    if (truck_screen_y >= -TRUCK_BOB_HEIGHT && truck_screen_y < SCREENHEIGHT)
    {
        /* TODO: BlitBob2 for truck — same cookie-cutter as car BOBs
         * but with TRUCK_BOB_WIDTH/HEIGHT dimensions.
         * Use truck_anim_frame to select truck_data_f1 or truck_data_f2.
         */
        truck_needs_restore = 2;
    }
    
    /* ---- Draw barrel BOBs ---- */
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
            /* TODO: BlitBob2 for barrel — 16x16 cookie-cutter */
            barrels[i].needs_restore = 2;
        }
    }
}

#else /* BARRELTRUCK_ENABLED == FALSE */

/* Stubs — compile to nothing */
void BarrelTruck_Initialize(void) {}
void BarrelTruck_Spawn(WORD x, WORD y) { (void)x; (void)y; }
void BarrelTruck_Stop(void) {}
void BarrelTruck_Update(void) {}
void BarrelTruck_Restore(void) {}
void BarrelTruck_Draw(void) {}
BOOL BarrelTruck_IsActive(void) { return FALSE; }
void BarrelTruck_Reset(void) {}

#endif /* BARRELTRUCK_ENABLED */
