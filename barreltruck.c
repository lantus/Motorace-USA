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

#define BARREL_DROP_COOLDOWN_MS  1400  /* time between drops */

static GameTimer barrel_drop_cooldown;

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
static WORD pending_x;

/* ---- Drop state ---- */
static Barrel barrels[MAX_BARRELS];
static GameTimer barrel_drop_timer;
static UBYTE next_drop_side = BARREL_SIDE_LEFT;

/* ---- BOB assets ---- */
static UBYTE *truck_data = NULL;
static UBYTE *left_strip_f1 = NULL;
static UBYTE *left_strip_f2 = NULL;
static UBYTE *barrel_data_f1 = NULL;
static UBYTE *barrel_data_f2 = NULL;

static UBYTE barrel_anim_frame = 0;
static UBYTE barrel_anim_counter = 0;
 
static const WORD sway_table[10] = { -2, 0, 2, 4, 6, 6, 4, 2, 0, -2 };

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
     
}

/* ---- Public API ---- */

void BarrelTruck_Initialize(void)
{
    truck_data  = Disk_AllocAndLoadAsset(TRUCK_BOB_FILE, MEMF_CHIP);
    left_strip_f1  = Disk_AllocAndLoadAsset(LEFT_STRIP_FILE1, MEMF_CHIP);
    left_strip_f2  = Disk_AllocAndLoadAsset(LEFT_STRIP_FILE2, MEMF_CHIP);
    barrel_data_f1 = Disk_AllocAndLoadAsset(BARREL_BOB_FILE1, MEMF_CHIP);
    barrel_data_f2 = Disk_AllocAndLoadAsset(BARREL_BOB_FILE2, MEMF_CHIP);    
    
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
    
    barrel_anim_frame = 0;
    barrel_anim_counter = 0;
  
    next_drop_side = BARREL_SIDE_LEFT;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
        barrels[i].needs_restore = 0;
    }
    
    Timer_StartMs(&barrel_drop_timer, BARREL_DROP_INTERVAL_MS);
    Timer_StartMs(&barrel_drop_cooldown, BARREL_DROP_COOLDOWN_MS);
    truck_active = TRUE;
    
    KPrintF("BarrelTruck: spawned at (%ld, %ld)\n", (LONG)x, (LONG)y);
}

void BarrelTruck_Stop(void)
{
   if (truck_active)
    {
        /* Restore both current AND prev position to clear both buffers */
        WORD positions[2][2] = {
            { truck_old_x, truck_old_y },
            { truck_prev_old_x, truck_prev_old_y }
        };
        
        for (int p = 0; p < 2; p++)
        {
            WORD old_screen_y = positions[p][1] - mapposy;
            if (old_screen_y < -TRUCK_BOB_HEIGHT || old_screen_y >= SCREENHEIGHT)
                continue;
            
            WORD clip_height = TRUCK_BOB_HEIGHT;
            WORD dest_y = old_screen_y;
            
            if (old_screen_y < -TRUCK_BOB_HEIGHT)
            {
                clip_height += old_screen_y;
                dest_y = 0;
            }
            if (old_screen_y + clip_height > SCREENHEIGHT)
                clip_height = SCREENHEIGHT - old_screen_y;
            if (clip_height <= 0) continue;
            
            WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
            if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
            else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
            
            WORD x_word_aligned = (positions[p][0] >> 3) & 0xFFFE;
            
            /* Restore to BOTH buffers — draw and display */
            UBYTE *buffers[2] = { draw_buffer, display_buffer };
            for (int b = 0; b < 2; b++)
            {
                if (buffer_y + clip_height > BITMAPHEIGHT)
                {
                    WORD lines_before = BITMAPHEIGHT - buffer_y;
                    WORD lines_after  = clip_height - lines_before;
                    ULONG offset1 = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
                    
                    WaitBlit();
                    custom->bltcon0 = 0x9F0;
                    custom->bltcon1 = 0;
                    custom->bltafwm = 0xFFFF;
                    custom->bltalwm = 0xFFFF;
                    custom->bltamod = 18;
                    custom->bltdmod = 18;
                    custom->bltapt = screen.pristine + offset1;
                    custom->bltdpt = buffers[b] + offset1;
                    custom->bltsize = ((lines_before << 2) << 6) | 3;
                    
                    ULONG offset2 = x_word_aligned;
                    WaitBlit();
                    custom->bltapt = screen.pristine + offset2;
                    custom->bltdpt = buffers[b] + offset2;
                    custom->bltsize = ((lines_after << 2) << 6) | 3;
                }
                else
                {
                    ULONG offset = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
                    WaitBlit();
                    custom->bltcon0 = 0x9F0;
                    custom->bltcon1 = 0;
                    custom->bltafwm = 0xFFFF;
                    custom->bltalwm = 0xFFFF;
                    custom->bltamod = 18;
                    custom->bltdmod = 18;
                    custom->bltapt = screen.pristine + offset;
                    custom->bltdpt = buffers[b] + offset;
                    custom->bltsize = ((clip_height << 2) << 6) | 3;
                }
            }
        }
    }
    
    truck_active = FALSE;
    truck_pending = FALSE;

    barrel_anim_frame = 0;
    barrel_anim_counter = 0;
 
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        barrels[i].active = FALSE;
    }
    
    Timer_Stop(&barrel_drop_timer);
    Timer_Stop(&barrel_drop_cooldown);

    Cars_EnableSpawning();
}

void BarrelTruck_Reset(void)
{
    BarrelTruck_Stop();
}

BOOL BarrelTruck_IsActive(void)
{
    return truck_active;
}

static BOOL BarrelTruck_CanSpawnBarrel(void)
{
    UBYTE count = 0;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (barrels[i].active)
            count++;
    }
    return count < 2;   /* max 2 on screen total */
}

static void BarrelTruck_DropBarrelAt(UBYTE side)
{
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) { slot = i; break; }
    }
    if (slot < 0) return;
    
    Barrel *b = &barrels[slot];
    
    /* Position behind truck, aligned with the strip it fell from */
    if (side == BARREL_SIDE_LEFT)
        b->x = truck_x + LEFT_STRIP_OFFSET_X - 1;   /* slight left of strip */
    else
        b->x = truck_x + RIGHT_STRIP_OFFSET_X - 1;  /* slight left of right strip */
    
    b->y = truck_y + TRUCK_BOB_HEIGHT - 4;   /* just behind truck */
    b->old_x = b->x;
    b->old_y = b->y;
    b->prev_old_x = b->x;
    b->prev_old_y = b->y;
    b->accumulator = 0;
    b->side = side;
    b->needs_restore = 0;
    b->active = TRUE;
}

static void BarrelTruck_DrawBarrel(Barrel *b)
{
    WORD screen_y = b->y - mapposy;
    WORD src_y_offset = 0;
    WORD clip_height = BARREL_BOB_HEIGHT;
    WORD dest_y = screen_y;
    
    if (screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - screen_y;
        if (clip_height <= 0) return;
    }
    
    if (screen_y < -BARREL_BOB_HEIGHT)
    {
        src_y_offset = -screen_y;
        clip_height -= src_y_offset;
        dest_y = 0;
        if (clip_height <= 0) return;
    }
    
    if (b->x < -16 || b->x > SCREENWIDTH + 16) return;
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
    
    /* Animate using the shared barrel_anim_frame */
    UBYTE *source = (barrel_anim_frame == 0) ? barrel_data_f1 : barrel_data_f2;
    UBYTE *mask = source + 4;
    
    ULONG src_offset = (ULONG)src_y_offset << 5;   /* * 32 */
    source += src_offset;
    mask   += src_offset;
    
    UWORD source_mod = 4;
    UWORD dest_mod   = BITMAPBYTESPERROW - 4;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    APTR tmp_restore[4];
    
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after  = clip_height - lines_before;
        
        UWORD bltsize1 = ((lines_before << 2) << 6) | 2;
        BlitBob2(SCREENWIDTH_WORDS, b->x, buffer_y, admod, bltsize1,
                 BARREL_BOB_WIDTH, tmp_restore, source, mask, draw_buffer);
        
        ULONG advance = (ULONG)lines_before << 5;
        UWORD bltsize2 = ((lines_after << 2) << 6) | 2;
        
        BlitBob2(SCREENWIDTH_WORDS, b->x, 0, admod, bltsize2,
                 BARREL_BOB_WIDTH, tmp_restore, source + advance, mask + advance, draw_buffer);
    }
    else
    {
        UWORD bltsize = ((clip_height << 2) << 6) | 2;
        BlitBob2(SCREENWIDTH_WORDS, b->x, buffer_y, admod, bltsize,
                 BARREL_BOB_WIDTH, tmp_restore, source, mask, draw_buffer);
    }
}

static void BarrelTruck_RestoreBarrel(Barrel *b)
{
    WORD old_screen_y = b->prev_old_y - mapposy;
    if (old_screen_y < -BARREL_BOB_HEIGHT || old_screen_y >= SCREENHEIGHT + BARREL_BOB_HEIGHT)
        return;
    
    WORD clip_height = BARREL_BOB_HEIGHT;
    WORD dest_y = old_screen_y;
    
    if (old_screen_y < -BARREL_BOB_HEIGHT)
    {
        clip_height += old_screen_y;
        dest_y = 0;
    }
    if (old_screen_y + clip_height > SCREENHEIGHT)
        clip_height = SCREENHEIGHT - old_screen_y;
    if (clip_height <= 0) return;
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
    
    WORD x_word_aligned = (b->prev_old_x >> 3) & 0xFFFE;
    
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after  = clip_height - lines_before;
        ULONG offset1 = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
        
        WaitBlit();
        custom->bltcon0 = 0x9F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = 20;     /* 2-word wide: 24 - 4 = 20 */
        custom->bltdmod = 20;
        custom->bltapt = screen.pristine + offset1;
        custom->bltdpt = draw_buffer + offset1;
        custom->bltsize = ((lines_before << 2) << 6) | 2;
        
        ULONG offset2 = x_word_aligned;
        WaitBlit();
        custom->bltapt = screen.pristine + offset2;
        custom->bltdpt = draw_buffer + offset2;
        custom->bltsize = ((lines_after << 2) << 6) | 2;
    }
    else
    {
        ULONG offset = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
        WaitBlit();
        custom->bltcon0 = 0x9F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = 20;
        custom->bltdmod = 20;
        custom->bltapt = screen.pristine + offset;
        custom->bltdpt = draw_buffer + offset;
        custom->bltsize = ((clip_height << 2) << 6) | 2;
    }
}

static void BarrelTruck_DrawStrip(UBYTE *source, WORD world_x, WORD world_y)
{
    WORD screen_y = world_y - mapposy;
    WORD src_y_offset = 0;
    WORD clip_height = LEFT_STRIP_HEIGHT;
    WORD dest_y = screen_y;
    
    if (screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - screen_y;
        if (clip_height <= 0) return;
    }
    
    if (screen_y < -LEFT_STRIP_HEIGHT)
    {
        src_y_offset = -screen_y;
        clip_height -= src_y_offset;
        dest_y = 0;
        if (clip_height <= 0) return;
    }
    
    if (world_x < -16 || world_x > SCREENWIDTH + 16) return;
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
    
    /* Strip: 2-word wide BOB, 32 bytes per line */
    UBYTE *mask = source + 4;
    
    ULONG src_offset = (ULONG)src_y_offset << 5;   /* * 32 */
    source += src_offset;
    mask   += src_offset;
    
    UWORD source_mod = 4;
    UWORD dest_mod   = BITMAPBYTESPERROW - 4;   /* 20 */
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    APTR tmp_restore[4];
    
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after  = clip_height - lines_before;
        
        UWORD bltsize1 = ((lines_before << 2) << 6) | 2;
        BlitBob2(SCREENWIDTH_WORDS, world_x, buffer_y, admod, bltsize1,
                 LEFT_STRIP_WIDTH, tmp_restore, source, mask, draw_buffer);
        
        ULONG advance = (ULONG)lines_before << 5;
        UWORD bltsize2 = ((lines_after << 2) << 6) | 2;
        
        BlitBob2(SCREENWIDTH_WORDS, world_x, 0, admod, bltsize2,
                 LEFT_STRIP_WIDTH, tmp_restore, source + advance, mask + advance, draw_buffer);
    }
    else
    {
        UWORD bltsize = ((clip_height << 2) << 6) | 2;
        BlitBob2(SCREENWIDTH_WORDS, world_x, buffer_y, admod, bltsize,
                 LEFT_STRIP_WIDTH, tmp_restore, source, mask, draw_buffer);
    }
}

void BarrelTruck_Update(void)
{
    if (truck_pending && !truck_active)
    {
        if (!Cars_AnyOnScreen())
        {
            truck_pending = FALSE;
            BarrelTruck_Spawn(pending_x, mapposy - TRUCK_BOB_HEIGHT - 32);
        }
        return;
    }
    
    if (!truck_active) return;
    
    WORD effective_speed = 40;
    
    truck_speed = GetScrollAmount(effective_speed);
    
    truck_accumulator += truck_speed;
    if (truck_accumulator >= 256)
    {
        WORD pixels = truck_accumulator >> 8;
        truck_y -= pixels;
        truck_accumulator &= 0xFF;
    }
    
    truck_wheel_counter++;
    if (truck_wheel_counter >= 6)
    {
        truck_wheel_counter = 0;
        truck_wheel_frame ^= 1;
    }
    
    WORD truck_screen_y = truck_y - mapposy;
    if (truck_screen_y > SCREENHEIGHT + TRUCK_BOB_HEIGHT)
    {
        BarrelTruck_Stop();
        return;
    }
 
    /* In BarrelTruck_Update */
    barrel_anim_counter++;
    if (barrel_anim_counter >= 2)
    {
        barrel_anim_counter = 0;
        barrel_anim_frame ^= 1;
    }

    barrel_zigzag_counter++;
    if (barrel_zigzag_counter >= 5)
    {
        barrel_zigzag_counter = 0;
        barrel_zigzag_frame++;
        if (barrel_zigzag_frame >= 10) barrel_zigzag_frame = 0;
    }
    
    /* ---- Barrel dropping ---- */
 
    if (Timer_HasElapsed(&barrel_drop_cooldown))
    {
        static UBYTE last_drop_phase = 255;
        
        if (barrel_zigzag_counter == 0 && barrel_zigzag_frame != last_drop_phase)
        {
            if (barrel_zigzag_frame == 4)
            {
                if (BarrelTruck_CanSpawnBarrel())
                {
                    BarrelTruck_DropBarrelAt(BARREL_SIDE_LEFT);
                    last_drop_phase = 4;
                    Timer_StartMs(&barrel_drop_cooldown, BARREL_DROP_COOLDOWN_MS);
                }
            }
            else if (barrel_zigzag_frame == 9)
            {
                if (BarrelTruck_CanSpawnBarrel())
                {
                    BarrelTruck_DropBarrelAt(BARREL_SIDE_RIGHT);
                    last_drop_phase = 9;
                    Timer_StartMs(&barrel_drop_cooldown, BARREL_DROP_COOLDOWN_MS);
                }
            }
        }
    }
    
    /* ---- Update falling barrels ---- */
    WORD barrel_scroll = GetScrollAmount(BARREL_WORLD_SPEED);
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        barrels[i].accumulator += barrel_scroll;
        if (barrels[i].accumulator >= 256)
        {
            WORD pixels = barrels[i].accumulator >> 8;
            barrels[i].y -= pixels;   /* moves forward (decreasing Y) slowly */
            barrels[i].accumulator &= 0xFF;
        }
        
        /* Deactivate when off bottom of screen */
        WORD barrel_screen_y = barrels[i].y - mapposy;
        if (barrel_screen_y > SCREENHEIGHT + BARREL_BOB_HEIGHT)
            barrels[i].active = FALSE;
    }
}
 
void BarrelTruck_RequestSpawn(WORD x)
{
    if (truck_active || truck_pending) return;
    
    truck_pending = TRUE;
    pending_x = x;
     
    Cars_DisableSpawning();  /* Stop new cars — existing ones finish naturally */
}

void BarrelTruck_Draw(void)
{
    if (!truck_active) return;
    
    /* ===== RESTORE from 2 frames ago ===== */
    {
        WORD old_screen_y = truck_prev_old_y - mapposy;
        
        /* Only restore if prev position was on screen */
        if (old_screen_y >= -TRUCK_BOB_HEIGHT && old_screen_y < SCREENHEIGHT + TRUCK_BOB_HEIGHT)
        {
            WORD clip_height = TRUCK_BOB_HEIGHT;
            WORD dest_y = old_screen_y;
            
            /* Clip TOP */
            if (old_screen_y < -TRUCK_BOB_HEIGHT)
            {
                clip_height += old_screen_y;
                dest_y = 0;
            }
            
            /* Clip BOTTOM */
            if (old_screen_y + clip_height > SCREENHEIGHT)
            {
                clip_height = SCREENHEIGHT - old_screen_y;
            }
            
            if (clip_height > 0)
            {
                WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
                if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
                else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
                
                WORD x_word_aligned = (truck_prev_old_x >> 3) & 0xFFFE;
                
                if (buffer_y + clip_height > BITMAPHEIGHT)
                {
                    /* Split restore across bitmap wrap */
                    WORD lines_before = BITMAPHEIGHT - buffer_y;
                    WORD lines_after  = clip_height - lines_before;
                    
                    ULONG offset1 = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
                    
                    WaitBlit();
                    custom->bltcon0 = 0x9F0;
                    custom->bltcon1 = 0;
                    custom->bltafwm = 0xFFFF;
                    custom->bltalwm = 0xFFFF;
                    custom->bltamod = 18;
                    custom->bltdmod = 18;
                    custom->bltapt = screen.pristine + offset1;
                    custom->bltdpt = draw_buffer + offset1;
                    custom->bltsize = ((lines_before << 2) << 6) | 3;
                    
                    ULONG offset2 = x_word_aligned;
                    WaitBlit();
                    custom->bltapt = screen.pristine + offset2;
                    custom->bltdpt = draw_buffer + offset2;
                    custom->bltsize = ((lines_after << 2) << 6) | 3;
                }
                else
                {
                    /* Single restore */
                    ULONG offset = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
                    
                    WaitBlit();
                    custom->bltcon0 = 0x9F0;
                    custom->bltcon1 = 0;
                    custom->bltafwm = 0xFFFF;
                    custom->bltalwm = 0xFFFF;
                    custom->bltamod = 18;
                    custom->bltdmod = 18;
                    custom->bltapt = screen.pristine + offset;
                    custom->bltdpt = draw_buffer + offset;
                    custom->bltsize = ((clip_height << 2) << 6) | 3;
                }
            }
        }
    }
    
    /* ===== ROLL position history forward ===== */
    truck_prev_old_x = truck_old_x;
    truck_prev_old_y = truck_old_y;
    truck_old_x = truck_x;
    truck_old_y = truck_y;
    
    /* ===== DRAW at current position ===== */
    {
        WORD screen_y = truck_y - mapposy;
        WORD src_y_offset = 0;
        WORD clip_height = TRUCK_BOB_HEIGHT;
        WORD dest_y = screen_y;
        
        /* Clip BOTTOM */
        if (screen_y + clip_height > SCREENHEIGHT)
        {
            clip_height = SCREENHEIGHT - screen_y;
            if (clip_height <= 0) return;
        }
        
        /* Clip TOP */
        if (screen_y < -TRUCK_BOB_HEIGHT)
        {
            src_y_offset = -screen_y;
            clip_height -= src_y_offset;
            dest_y = 0;
            if (clip_height <= 0) return;
        }
        
        /* X bounds */
        if (truck_x < -16 || truck_x > SCREENWIDTH + 16) return;
        
        WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
        if (buffer_y < 0) buffer_y += BITMAPHEIGHT;
        else if (buffer_y >= BITMAPHEIGHT) buffer_y -= BITMAPHEIGHT;
        
        UBYTE *source = truck_data;
        UBYTE *mask   = source + 6;
        
        ULONG src_offset = (src_y_offset << 4) + (src_y_offset << 3);  /* * 24 */
        source += src_offset;
        mask   += src_offset;
        
        UWORD source_mod = 6;
        UWORD dest_mod   = 18;
        ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
        
        APTR tmp_restore[4];
        
        if (buffer_y + clip_height > BITMAPHEIGHT)
        {
            /* Split draw across bitmap wrap */
            WORD lines_before = BITMAPHEIGHT - buffer_y;
            WORD lines_after  = clip_height - lines_before;
            
            UWORD bltsize1 = ((lines_before << 2) << 6) | 3;
            BlitBob2(SCREENWIDTH_WORDS, truck_x, buffer_y, admod, bltsize1,
                     TRUCK_BOB_WIDTH, tmp_restore, source, mask, draw_buffer);
            
            ULONG advance = (lines_before << 4) + (lines_before << 3);
            UBYTE *source2 = source + advance;
            UBYTE *mask2   = mask + advance;
            UWORD bltsize2 = ((lines_after << 2) << 6) | 3;
            
            BlitBob2(SCREENWIDTH_WORDS, truck_x, 0, admod, bltsize2,
                     TRUCK_BOB_WIDTH, tmp_restore, source2, mask2, draw_buffer);
        }
        else
        {
            UWORD bltsize = ((clip_height << 2) << 6) | 3;
            BlitBob2(SCREENWIDTH_WORDS, truck_x, buffer_y, admod, bltsize,
                     TRUCK_BOB_WIDTH, tmp_restore, source, mask, draw_buffer);
        }
    }

    /* ===== DRAW strips on top of truck bed ===== */
    {

        WORD left_sway  = sway_table[barrel_zigzag_frame];
         
        /* Animate barrel frame every phase step */
        UBYTE anim = barrel_anim_frame;
  
        
        UBYTE *lsrc = (anim == 0) ? left_strip_f1 : left_strip_f2;
        BarrelTruck_DrawStrip(lsrc,
                              truck_x + LEFT_STRIP_OFFSET_X,
                              truck_y + LEFT_STRIP_OFFSET_Y  + left_sway);
        
        /* Right strip — opposite frame for counter-sway */
        UBYTE *rsrc = (anim == 1) ? left_strip_f2 : left_strip_f1;
        BarrelTruck_DrawStrip(rsrc,
                              truck_x + RIGHT_STRIP_OFFSET_X,
                              truck_y + RIGHT_STRIP_OFFSET_Y - left_sway);
    }

    /* ===== DRAW dropped barrels (restore + roll + draw) ===== */
    for (int i = 0; i < MAX_BARRELS; i++)
    {
        if (!barrels[i].active) continue;
        
        /* Restore from 2 frames ago */
        WORD prev_screen_y = barrels[i].prev_old_y - mapposy;
        if (prev_screen_y >= -BARREL_BOB_HEIGHT && prev_screen_y < SCREENHEIGHT + BARREL_BOB_HEIGHT)
        {
            BarrelTruck_RestoreBarrel(&barrels[i]);
        }
        
        /* Roll position history */
        barrels[i].prev_old_x = barrels[i].old_x;
        barrels[i].prev_old_y = barrels[i].old_y;
        barrels[i].old_x = barrels[i].x;
        barrels[i].old_y = barrels[i].y;
        
        /* Draw */
        BarrelTruck_DrawBarrel(&barrels[i]);
    }
}

BOOL BarrelTruck_CheckCollision(WORD bike_cx, WORD bike_top)
{
    if (!truck_active) return FALSE;

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
 