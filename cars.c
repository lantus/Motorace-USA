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
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "memory.h"
#include "blitter.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "cars.h"
#include "audio.h"

#define CAR_BG_SIZE 768
extern volatile struct Custom *custom;

#define CAR_SPEED_OFFSET_EASY   5   // Before 15 cars passed
#define CAR_SPEED_OFFSET_HARD   4   // After 15 cars passed
#define CAR_STEER_AMOUNT        2   // Pixels per steer  

WORD next_respawn_id = 0;     // Start with car 0
WORD respawn_timer = 60;
WORD respawn_interval = 120;
WORD respawn_distance = 400;
WORD last_respawned_id = -1;  // -1 = none respawned yet
UWORD cars_passed = 0;

BlitterObject car[MAX_CARS];

static LONG car_last_y[MAX_CARS];
static BOOL car_was_ahead[MAX_CARS];
static BOOL passing_initialized = FALSE;

/* Restore pointers */
APTR restore_ptrs[MAX_CARS * 4];
APTR *restore_ptr;

void Cars_LoadSprites()
{
    car[0].data = Disk_AllocAndLoadAsset(CAR0_FRAME0, MEMF_CHIP);
    car[0].data_frame2 = Disk_AllocAndLoadAsset(CAR0_FRAME1, MEMF_CHIP);

    car[1].data = Disk_AllocAndLoadAsset(CAR1_FRAME0, MEMF_CHIP);
    car[1].data_frame2 = Disk_AllocAndLoadAsset(CAR1_FRAME1, MEMF_CHIP);

    car[2].data = Disk_AllocAndLoadAsset(CAR2_FRAME0, MEMF_CHIP);
    car[2].data_frame2 = Disk_AllocAndLoadAsset(CAR2_FRAME1, MEMF_CHIP);

    car[3].data = Disk_AllocAndLoadAsset(CAR3_FRAME0, MEMF_CHIP);
    car[3].data_frame2 = Disk_AllocAndLoadAsset(CAR3_FRAME1, MEMF_CHIP);

    car[4].data = Disk_AllocAndLoadAsset(CAR4_FRAME0, MEMF_CHIP);
    car[4].data_frame2 = Disk_AllocAndLoadAsset(CAR4_FRAME1, MEMF_CHIP);   
}

void Cars_Initialize(void)
{
    car[0].background = Mem_AllocChip(CAR_BG_SIZE);
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
  
    Cars_LoadSprites();

}
 
void Cars_ResetPositions(void)
{
    car[0].id = 0;
    car[0].visible = TRUE;
    car[0].x = FAR_RIGHT_LANE;
    car[0].y = mapposy + 220;   
    car[0].old_x = car[0].x;
    car[0].old_y = car[0].y;
    car[0].prev_old_x  = car[0].x;
    car[0].prev_old_y = car[0].y;
    car[0].speed = 41;
    car[0].accumulator = 0;
    car[0].target_speed = 125;
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;
    car[0].anim_frame = 0;
    car[0].anim_counter = 0;
    car[0].has_blocked_bike = FALSE;
    car[0].block_timer = 0;

    car[1].id = 1;
    car[1].visible = TRUE;
    car[1].x = FAR_LEFT_LANE;
    car[1].y = mapposy + 220;
    car[1].speed = 63;
    car[1].accumulator = 0;
    car[1].target_speed = 155;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    car[1].anim_frame = 0;
    car[1].anim_counter = 0;
    car[1].has_blocked_bike = FALSE;
    car[1].block_timer = 0;
    car[1].old_x = car[1].x;
    car[1].old_y = car[1].y;
    car[1].prev_old_x  = car[1].x;
    car[1].prev_old_y = car[1].y;
   
    car[2].id = 2;
    car[2].visible = TRUE;
    car[2].x = FAR_LEFT_LANE;
    car[2].y = mapposy + 140;
    car[2].speed = 65;
    car[2].target_speed = 165;
    car[2].accumulator = 0;
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    car[2].anim_frame = 0;
    car[2].anim_counter = 0;
    car[2].has_blocked_bike = FALSE;
    car[2].block_timer = 0;  
    car[2].old_x = car[2].x;
    car[2].old_y = car[2].y;
    car[2].prev_old_x  = car[2].x;
    car[2].prev_old_y = car[2].y;
   
    car[3].id = 3;
    car[3].visible = TRUE;
    car[3].x = FAR_RIGHT_LANE;
    car[3].y = mapposy + 140;  
    car[3].speed = 45;
    car[3].target_speed = 165;
    car[3].accumulator = 0;
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
    car[3].anim_frame = 0;
    car[3].anim_counter = 0;
    car[3].has_blocked_bike = FALSE;
    car[3].block_timer = 0;  
    car[3].old_x = car[3].x;
    car[3].old_y = car[3].y;
    car[3].prev_old_x  = car[3].x;
    car[3].prev_old_y = car[3].y; 
   
    car[4].id = 4;
    car[4].visible = TRUE;
    car[4].x = CENTER_LANE;
    car[4].y = mapposy + 40;
    car[4].speed = 56;
    car[4].target_speed = 165;
    car[4].accumulator = 0;
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE;
    car[4].anim_frame = 0;
    car[4].anim_counter = 0;
    car[4].has_blocked_bike = FALSE;
    car[4].block_timer = 0;
    car[4].old_x = car[4].x;
    car[4].old_y = car[4].y;
    car[4].prev_old_x  = car[4].x;
    car[4].prev_old_y = car[4].y;   

    // Reset passing tracking
    for (int i = 0; i < MAX_CARS; i++)
    {
        car_last_y[i] = car[i].y;
        car_was_ahead[i] = TRUE;
        
        Cars_AssignLane(&car[i]);
    }

    passing_initialized = TRUE;
    cars_passed = 0;
}
 
void Cars_RenderBOB(BlitterObject *car)
{

    /* 32x32
    Barrel Shifting is a pain...
    bitplanes = 4
    words per scanline - 2 words (32/16)
              - 3 words with -AW padding
              
    Bytes per scanline = (48/8) = 6 bytes + 6 bytes mask
    Total scanline = 32

    BlitSize (32x4) << 6 | 3
 
    src mod = 3 words x 2 Bytes
    dst mod = 40 - 6 = 34; for 192 = 24 - 6 = 18
    */
   
    if (!car->visible) return;
    
    WORD screen_y = car->y - mapposy;
    
    // Calculate clipping
    WORD src_y_offset = 0;
    WORD clip_height = BOB_HEIGHT;
    WORD dest_y = screen_y;
    
    // Clip TOP edge
    if (screen_y < -BOB_HEIGHT)
    {
        src_y_offset = -screen_y;
        clip_height -= src_y_offset;
        dest_y = 0;
        
        if (clip_height <= 0)
        {
            car->off_screen = TRUE;
            return;
        }
    }
    
    // Clip BOTTOM edge
    if (screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - screen_y;
        
        if (clip_height <= 0)
        {
            car->off_screen = TRUE;
            return;
        }
    }
    
    // Check X bounds
    if (car->x < -16 || car->x > SCREENWIDTH + 16)
    {
        car->off_screen = TRUE;
        return;
    }
    
    car->off_screen = FALSE;
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    
    // Handle wraparound
    if (buffer_y < 0)
        buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT)
        buffer_y -= BITMAPHEIGHT;
    
    WORD x = car->x;
    
    // Select animation frame
    UBYTE *source = (car->anim_frame == 0) ? car->data : car->data_frame2;
    UBYTE *mask = source + 6;
    
    // Offset source: src_y_offset * 24 = (src_y_offset << 4) + (src_y_offset << 3)
    ULONG src_offset = (src_y_offset << 4) + (src_y_offset << 3);  // * 16 + * 8 = * 24
    source += src_offset;
    mask += src_offset;
    
    UWORD source_mod = 6;
    UWORD dest_mod = 18;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    APTR car_restore_ptrs[4];
    
    // Check if BOB crosses buffer split
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        // SPLIT BLIT
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after = clip_height - lines_before;
        
        // bltsize: lines * 4 = lines << 2
        UWORD bltsize1 = ((lines_before << 2) << 6) | 3;
        BlitBob2(SCREENWIDTH_WORDS, x, buffer_y, admod, bltsize1, 
                 BOB_WIDTH, car_restore_ptrs, source, mask, draw_buffer);
        
        // Advance source: lines_before * 24
        ULONG advance = (lines_before << 4) + (lines_before << 3);
        UBYTE *source2 = source + advance;
        UBYTE *mask2 = mask + advance;
        APTR restore_ptrs2[4];
        UWORD bltsize2 = ((lines_after << 2) << 6) | 3;
        
        BlitBob2(SCREENWIDTH_WORDS, x, 0, admod, bltsize2,
                 BOB_WIDTH, restore_ptrs2, source2, mask2, draw_buffer);
    }
    else
    {
        // Normal single blit
        UWORD bltsize = ((clip_height << 2) << 6) | 3;
        
        BlitBob2(SCREENWIDTH_WORDS, x, buffer_y, admod, bltsize,
                 BOB_WIDTH, car_restore_ptrs, source, mask, draw_buffer);
    }
}

void Cars_AccelerateCar(BlitterObject *car)
{
    if (car->crashed) return;
    
    // Accelerate toward target speed
    if (car->speed < car->target_speed)
    {
        car->speed += 2;  // Acceleration rate
        if (car->speed > car->target_speed)
        {
            car->speed = car->target_speed;
        }
    }

}
 

void Cars_PreDraw(void)
{
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen)
        {
            // Use prev_old position (2 frames back)
            WORD temp_x = car[i].old_x;
            WORD temp_y = car[i].old_y;
            
            car[i].old_x = car[i].prev_old_x;
            car[i].old_y = car[i].prev_old_y;
  
            car[i].needs_restore = 1;
            
            // Only restore if prev position was on screen
            WORD prev_screen_y = car[i].prev_old_y - mapposy;
            if (prev_screen_y >= -BOB_HEIGHT && prev_screen_y < SCREENHEIGHT + BOB_HEIGHT)
            {
                Cars_CopyPristineBackground(&car[i]);
            }   
            
            car[i].old_x = temp_x;
            car[i].old_y = temp_y;

            // Save position history
            car[i].prev_old_x = car[i].old_x;
            car[i].prev_old_y = car[i].old_y;
            
            car[i].old_x = car[i].x;
            car[i].old_y = car[i].y;
            
    
            Cars_RenderBOB(&car[i]);
 
        }
    }

}


void Cars_Update(void)
{
    bike_world_y = mapposy + bike_position_y;
    Cars_CheckForRespawn();

    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible) continue;
        
        // Just pass the car pointer
        Cars_CheckPassing(&car[i]);
        
       
        Cars_Tick(&car[i]);
    }  
}

void Cars_CopyPristineBackground(BlitterObject *car)
{
    if (!car->needs_restore) return;
    
    WORD old_screen_y = car->old_y - mapposy;

    // Skip restore if old position was off screen
    if (old_screen_y < -BOB_HEIGHT || old_screen_y >= SCREENHEIGHT + BOB_HEIGHT)
        return;
    
    // Calculate clipping
    WORD clip_height = BOB_HEIGHT;
    WORD dest_y = old_screen_y;
    
    // Clip TOP edge
    if (old_screen_y < -BOB_HEIGHT)
    {
        clip_height += old_screen_y;
        dest_y = 0;
        if (clip_height <= 0) return;
    }
    
    // Clip BOTTOM edge
    if (old_screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - old_screen_y;
        if (clip_height <= 0) return;
    }
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    
    // Handle wraparound
    if (buffer_y < 0)
        buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT)
        buffer_y -= BITMAPHEIGHT;
    
    // X position word-aligned: (car->old_x >> 4) << 1
    WORD x_word_aligned = (car->old_x >> 3) & 0xFFFE;  // Even faster
    
    // Check if restore crosses buffer split
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        // SPLIT RESTORE
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after = clip_height - lines_before;
        
        // buffer_y * SCREENWIDTH_WORDS: SCREENWIDTH_WORDS = 96 = 64 + 32
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
        
        // Part 2: Y=0
        ULONG offset2 = x_word_aligned;
        
        WaitBlit();
        custom->bltapt = screen.pristine + offset2;
        custom->bltdpt = draw_buffer + offset2;
        custom->bltsize = ((lines_after << 2) << 6) | 3;
    }
    else
    {
        // Single blit
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
 

void Cars_HandleSpinout(UBYTE car_index)
{
    if (car_index >= MAX_CARS) return;
    
    BlitterObject *crashed_car = &car[car_index];
    
    // Mark car as crashed
    crashed_car->crashed = TRUE;
    crashed_car->crash_timer = 120;  // Stopped for 60 frames (1 second)
    
    // Decelerate car extremely quickly
    crashed_car->speed = 0;
    crashed_car->accumulator = 0;
}

#define CAR_STEER_HARD  3    // Emergency — center blocked
#define CAR_STEER_SOFT  1    // Gentle — side blocked or pursuit

void Cars_CheckLaneAndSteer(BlitterObject *car)
{
 if (!car->visible || car->crashed) return;
    
    WORD cx = car->x + 16;
    WORD check_y = car->y;
    
    UBYTE left   = Collision_GetLane(cx - 16, check_y);
    UBYTE center = Collision_GetLane(cx, check_y);
    UBYTE right  = Collision_GetLane(cx + 16, check_y);
    
    // Cars should only drive in LANE_1 through LANE_4
    // Shoulders are drivable but NOT desirable — treat as blocked for AI
    BOOL ok_l = (left >= LANE_1 && left <= LANE_4);
    BOOL ok_c = (center >= LANE_1 && center <= LANE_4);
    BOOL ok_r = (right >= LANE_1 && right <= LANE_4);
    
    // ===== CENTER BLOCKED — STEER EVERY FRAME =====
    if (!ok_c)
    {
        if (ok_r && !ok_l)
            car->x += CAR_STEER_HARD;
        else if (ok_l && !ok_r)
            car->x -= CAR_STEER_HARD;
        else if (ok_l && ok_r)
        {
            // Both sides open — pick the side with more room
            UBYTE far_l = Collision_GetLane(cx - 32, check_y);
            UBYTE far_r = Collision_GetLane(cx + 32, check_y);
            if (far_r != LANE_OFFROAD)
                car->x += CAR_STEER_HARD;
            else
                car->x -= CAR_STEER_HARD;
        }
        else
        {
            // All blocked — desperate random steer
            car->x += ((game_frame_count + car->id) & 1) ? CAR_STEER_HARD : -CAR_STEER_HARD;
        }
        return;
    }
    
    // ===== SIDE BLOCKED — NUDGE AWAY EVERY FRAME =====
    if (!ok_l || !ok_r)
    {
        if (!ok_l)
            car->x += CAR_STEER_SOFT;
        else
            car->x -= CAR_STEER_SOFT;
        return;
    }
    
    // ===== LOOK AHEAD — early warning for curves =====
    UBYTE ahead_l = Collision_GetLane(cx - 16, check_y - 32);
    UBYTE ahead_c = Collision_GetLane(cx, check_y - 32);
    UBYTE ahead_r = Collision_GetLane(cx + 16, check_y - 32);
    
    if (ahead_c == LANE_OFFROAD)
    {
        // Road curves ahead — start turning early
        if (ahead_r != LANE_OFFROAD && ahead_l == LANE_OFFROAD)
            car->x += CAR_STEER_SOFT;
        else if (ahead_l != LANE_OFFROAD && ahead_r == LANE_OFFROAD)
            car->x -= CAR_STEER_SOFT;
        return;
    }
    
    if (ahead_l == LANE_OFFROAD)
    {
        car->x += CAR_STEER_SOFT;
        return;
    }
    
    if (ahead_r == LANE_OFFROAD)
    {
        car->x -= CAR_STEER_SOFT;
        return;
    }
    
    // ===== ALL CLEAR — PURSUE PLAYER (throttled) =====
    if ((game_frame_count + car->id) & 1) return;  // Only pursue every other frame
    
    Cars_PursuePlayer(car);
}

void Cars_PursuePlayer(BlitterObject *car)
{
   WORD cx = car->x + 16;
    WORD car_screen_y = car->y - mapposy;
    
    // Only pursue when car is AHEAD of bike and within chase range
    // (car_screen_y < bike_position_y means car is ABOVE bike on screen)
    if (car_screen_y < 16) return;                    // Too close to top
    if (car_screen_y > bike_position_y - 16) return;  // Car is behind bike
    if (car_screen_y < bike_position_y - 96) return;  // Too far ahead
    
    WORD x_diff = bike_position_x - cx;
    WORD abs_diff = ABS(x_diff);
    
    // Already lined up — do nothing
    if (abs_diff < 8) return;
    
    // Too far apart — give up
    if (abs_diff > 48) return;
    
    // Only pursue ~25% of frames — keeps it beatable (NES does the same)
    UBYTE rng = (game_frame_count * 7 + car->id * 13) & 0x0F;
    if (rng > 3) return;
    
    // Gentle 1px nudge
    if (x_diff > 0)
        car->x += 1;
    else
        car->x -= 1;
}

// When spawning/resetting a car:
void Cars_AssignLane(BlitterObject *car)
{
    // Use car id + frame count for variety
    UBYTE roll = (car->id * 7 + game_frame_count) & 3;
    static const UBYTE lanes[] = { LANE_1, LANE_2, LANE_3, LANE_4 };
    car->preferred_lane = lanes[roll];
}

void Cars_UpdatePosition(BlitterObject *c)
{
   if (!c->visible) return;
    
    if (c->crashed)
    {
        if (c->crash_timer > 0) { c->crash_timer--; return; }
        else { c->crashed = FALSE; }
    }
    
    c->moved = TRUE;
    
    // ===== FIXED FORWARD SPEED =====
    // Car always drives forward (decreasing Y) at a constant rate.
    // SmoothScroll changes mapposy, which handles relative motion:
    //   - Bike fast → mapposy drops fast → screen_y increases → car drifts down (passed)
    //   - Bike slow → mapposy drops slow → screen_y decreases → car drifts up (pulls away)
    // This matches NES where offset=4 is the car's OWN speed.
    
    WORD car_speed;
    if (c->target_speed > 150)
        car_speed = 1280;    // 5 px/frame — fast car, hard to pass
    else if (c->target_speed > 120)
        car_speed = 1024;    // 4 px/frame — NES default
    else
        car_speed = 768;     // 3 px/frame — slow car, easy to pass
    
    // Early game: cars slightly slower
    if (cars_passed < 15)
        car_speed -= 128;    // -0.5 px/frame
    
    // Player crashed: cars speed up (pull away)
    if (collision_state == COLLISION_TRAFFIC)
        car_speed += 512;
    
    c->accumulator += car_speed;
    
    if (c->accumulator >= 256)
    {
        WORD pixels = c->accumulator >> 8;
        c->y -= pixels;     // ALWAYS forward (decreasing Y = up the road)
        c->accumulator &= 0xFF;
    }
    
    // Wheel animation
    c->anim_counter++;
    WORD threshold = (bike_speed < 84) ? 8 : (bike_speed < 126) ? 4 : 2;
    if (c->anim_counter >= threshold)
    {
        c->anim_counter = 0;
        c->anim_frame ^= 1;
    }
}

void Cars_CheckBikeOvertake(BlitterObject *car, WORD bike_x)
{
    if (car->off_screen || car->crashed || car->has_blocked_bike) return;
    
    WORD car_screen_y = car->y - mapposy;
    
    if (car_screen_y > 64) return;
    
    WORD x_diff = ABS(car->x - bike_x);
    
    if (x_diff <= game_car_block_x_threshold)
    {
        car->block_timer++;
        
        if (car->block_timer >= game_car_block_move_rate)
        {
            car->block_timer = 0;
            
            // Move toward bike's X
            if (car->x < bike_x)
            {
                car->x += game_car_block_move_speed;
                if (car->x > bike_x) car->x = bike_x;
            }
            else if (car->x > bike_x)
            {
                car->x -= game_car_block_move_speed;
                if (car->x < bike_x) car->x = bike_x;
            }
            
            if (car->x == bike_x)
            {
                car->has_blocked_bike = TRUE;
                int car_index = car - &car[0];
            }
        }
    }
}

void Cars_Tick(BlitterObject *car)
{
    WORD temp_x = car->old_x;
    WORD temp_y = car->old_y;
    
    car->old_x = car->prev_old_x;
    car->old_y = car->prev_old_y;
    
    // Only restore if prev_old position was on screen
    WORD prev_screen_y = car->prev_old_y - mapposy;
    if (prev_screen_y >= -BOB_HEIGHT && prev_screen_y < SCREENHEIGHT + BOB_HEIGHT)
    {
        Cars_CopyPristineBackground(car);
    }
    
    car->old_x = temp_x;
    car->old_y = temp_y;

 
   // Cars_AccelerateCar(car);
    Cars_UpdatePosition(car);
    Cars_CheckLaneAndSteer(car);
    Cars_CheckForCollision(car);

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    car->old_x = car->x;
    car->old_y = car->y;

    Cars_RenderBOB(car);
    car->needs_restore = TRUE;
}

void Cars_CheckForCollision(BlitterObject *c)
{
    for (int j = c->id + 1; j < MAX_CARS; j++)
    {
        if (!car[c->id].visible || car[c->id].crashed) continue;
        // Bounding box collision check (32x32 pixels)
        WORD x_overlap = ABS(car[c->id].x - car[j].x) < 32;
        LONG y_distance = ABS(car[c->id].y - car[j].y);

        if (x_overlap && y_distance < CAR_COLLISION_DISTANCE)
        {
            // Determine which car is behind (higher Y = behind)
            BlitterObject *car_behind = (car[c->id].y > car[j].y) ? &car[c->id] : &car[j];
            BlitterObject *car_ahead = (car[c->id].y > car[j].y) ? &car[j] : &car[c->id];

            // Slow down the car behind
            if (car_behind->speed > car_ahead->speed)
            {
                car_behind->speed = car_ahead->speed - 4;
            }
        }
    }
}

void Cars_CheckForRespawn(void)
{
    if (respawn_timer > 0) { respawn_timer--; return; }
    
    WORD active_count = 0;
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen)
            active_count++;
    }
    
    WORD max_active;
    if (cars_passed < 3)
        max_active = 1;
    else if (cars_passed < 8)
        max_active = 2;
    else if (cars_passed < 15)
        max_active = 3;
    else
        max_active = 4;
    
    if (active_count >= max_active) return;
    
    // Rotate which slot to check — not always starting at 0
    static UBYTE next_slot = 0;
    
    for (int attempt = 0; attempt < MAX_CARS; attempt++)
    {
        int i = (next_slot + attempt) % MAX_CARS;
        
        if (car[i].off_screen || !car[i].visible)
        {
            WORD spawn_y;
            WORD spawn_x;
            WORD attempts = 0;
            
            WORD scroll = GetScrollAmount(bike_speed);
            
            if (scroll >= 768)
                spawn_y = mapposy - 32;
            else
                spawn_y = mapposy + SCREENHEIGHT + 32;
            
            do {
                spawn_x = 16 + ((game_frame_count * 7 + i * 31 + attempts * 13) & 127);
                UBYTE lane = Collision_GetLane(spawn_x + 16, spawn_y);
                if (lane != LANE_OFFROAD) break;
                attempts++;
            } while (attempts < 10);
            
            if (attempts >= 10) continue;  // Try next slot
            
            car[i].x = spawn_x;
            car[i].y = spawn_y;
            car[i].off_screen = TRUE;
            car[i].crashed = FALSE;
            car[i].has_blocked_bike = FALSE;
            car[i].accumulator = 0;
            car[i].needs_restore = 0;
            car[i].old_x = car[i].x;
            car[i].old_y = car[i].y;
            car[i].prev_old_x = car[i].x;
            car[i].prev_old_y = car[i].y;
            
            // Advance to next slot for next respawn
            next_slot = (i + 1) % MAX_CARS;
            
            respawn_timer = 60;
            return;
        }
    }
}

void Cars_CheckPassing(BlitterObject *c)
{
     if (stage_state != STAGE_PLAYING) return;
    
    LONG car_world_y = c->y;
    BOOL car_is_ahead = (car_world_y < bike_world_y);
    
    // Use c->id instead of passing index
    if (car_was_ahead[c->id] && !car_is_ahead)
    {
        SFX_Play(SFX_OVERHEADOVERTAKE);
        game_score += 500;
        
        if (game_rank > 1)
            game_rank--;
        
        KPrintF("=== PASSED CAR #%d! Rank: %d ===\n", c->id, game_rank);
    }
    else if (!car_was_ahead[c->id] && car_is_ahead)
    {
        if (game_rank < 99)
            game_rank++;
        
        KPrintF("=== GOT PASSED BY CAR #%d! Rank: %d ===\n", c->id, game_rank);
    }
    
    car_was_ahead[c->id] = car_is_ahead;
}

