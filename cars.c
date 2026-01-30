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

#define CAR_BG_SIZE 768
extern volatile struct Custom *custom;

WORD next_respawn_id = 0;     // Start with car 0
WORD respawn_timer = 60;
WORD respawn_interval = 120;
WORD respawn_distance = 400;
WORD last_respawned_id = -1;  // -1 = none respawned yet

BlitterObject car[MAX_CARS];
 
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
    car[1].target_speed = 120;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    car[1].anim_frame = 0;
    car[1].anim_counter = 0;
    car[1].has_blocked_bike = FALSE;
    car[1].block_timer = 0;
   
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
            
            Cars_CopyPristineBackground(&car[i]);
            
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
 
        // Check if car should block bike
        Cars_CheckBikeOvertake(&car[i], bike_position_x);
        
        // Update and draw car
        Cars_Tick(&car[i]);
    }  
 
}

void Cars_CopyPristineBackground(BlitterObject *car)
{
    if (!car->needs_restore) return;
    
    WORD old_screen_y = car->old_y - mapposy;
    
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

void Cars_CheckLaneAndSteer(BlitterObject *car)
{
    if (!car->visible || car->crashed) return;
 
    // Look ahead distance in pixels
    WORD lookahead = 32;  // Check 3 tiles ahead
    WORD check_y = car->y - lookahead;
    
    // sample left , center and right points of the bob
    WORD center_x = car->x + 16;
    WORD center_y = check_y;
   
    TileAttribute leftside_tile = Tile_GetAttrib(center_x-16, center_y);
    TileAttribute rightside_tile = Tile_GetAttrib(center_x+16, center_y);
    TileAttribute center_tile = Tile_GetAttrib(center_x, center_y);

    if (center_tile == TILEATTRIB_ROAD_RIGHT_HALF && leftside_tile == TILEATTRIB_CRASH)
    {
       // slide car left
       car->x += 1;  
    }
    else if (center_tile == TILEATTRIB_ROAD && leftside_tile == TILEATTRIB_ROAD_RIGHT_HALF)
    {
       // slide car left
       car->x += 1;  
    }
    else if (center_tile == TILEATTRIB_ROAD_LEFT_HALF && rightside_tile == TILEATTRIB_CRASH)
    {
       // slide right left
       car->x -= 1;  
    }
    else if (center_tile == TILEATTRIB_ROAD && rightside_tile == TILEATTRIB_ROAD_LEFT_HALF)
    {
       // slide right left
       car->x -= 1;  
    }
    else if (center_tile == TILEATTRIB_CRASH)
    {
        if (rightside_tile == TILEATTRIB_CRASH)
        {
            car->x -= 2;
        }
        else if (leftside_tile == TILEATTRIB_CRASH)
        {
            car->x += 2;
        }
    }
 
}

void Cars_UpdatePosition(BlitterObject *c)
{
    if (!c->visible) return;
    
    // Handle crash state
    if (c->crashed)
    {
        if (c->crash_timer > 0)
        {
            c->crash_timer--;
            c->speed = 0;
            return;
        }
        else
        {
            c->crashed = FALSE;
            c->speed = 20;
        }
    }
    
    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;
 
    // Normal movement
    WORD car_movement = -GetScrollAmount(c->speed);
    c->accumulator += car_movement;

    if (c->accumulator >= 256)
    {
        WORD pixels = c->accumulator >> 8;  
        c->y += pixels;
        c->accumulator &= 0xFF;  
    }
    else if (c->accumulator <= -256)
    {
        WORD pixels = (-c->accumulator) >> 8;  
        c->y -= pixels;
        c->accumulator += (pixels << 8);
    }
 
    // AI steering - check ahead and adjust lane
    Cars_CheckLaneAndSteer(c);

    // Car Wheel Animation  
    c->anim_counter++;
    WORD threshold = (c->speed < 84) ? 8 : 
                    (c->speed < 126) ? 4 : 2;

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
    // Erase old position
    WORD temp_x = car->old_x;
    WORD temp_y = car->old_y;
    
    car->old_x = car->prev_old_x;
    car->old_y = car->prev_old_y;
    
    Cars_CopyPristineBackground(car);
    
    car->old_x = temp_x;
    car->old_y = temp_y;

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    
    car->old_x = car->x;
    car->old_y = car->y;
 
    Cars_AccelerateCar(car);
    Cars_UpdatePosition(car);
    Cars_CheckForCollision(car);

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
    if (respawn_timer > 0)
    {
        respawn_timer--;
        return;
    }
    
    // Don't respawn ANYTHING if ANY car is still visible on screen
 
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen)
        {
            return; // We are done
        }
    }
 
    // If we recently respawned a car, check if it's moved far enough
    if (last_respawned_id != -1)
    {
        BlitterObject *last_car = &car[last_respawned_id];
        LONG distance = last_car->y - bike_world_y;
        
        BOOL far_enough = (distance > 150) || (distance < -150);
        
        if (!far_enough)
        {
            return;
        }
        
        last_respawned_id = -1;
    }
    
    // Now check ONLY the next car in queue
    BlitterObject *car_to_check = &car[next_respawn_id];
    
    if (!car_to_check->visible) return;
    
    LONG distance = car_to_check->y - bike_world_y;
    
    // Bike overtook this car - respawn ahead
    if (distance > 100)
    {
        WORD safe_lanes[] = {FAR_LEFT_LANE, CENTER_LANE, FAR_RIGHT_LANE};
        car_to_check->x = safe_lanes[next_respawn_id % 3];
        car_to_check->y = bike_world_y - respawn_distance - (next_respawn_id * 60);
        car_to_check->off_screen = TRUE;
        car_to_check->speed = car_to_check->target_speed;
        
        KPrintF("Respawned car #%ld AHEAD at y=%ld\n", next_respawn_id, car_to_check->y);
        
        last_respawned_id = next_respawn_id;
        next_respawn_id = (next_respawn_id + 1) % MAX_CARS;
        respawn_timer = respawn_interval;
 
    }
    // Car overtook bike - respawn behind
    else if (distance < -100)
    {
        WORD safe_lanes[] = {FAR_LEFT_LANE, CENTER_LANE, FAR_RIGHT_LANE};
        LONG base_spawn_y = bike_world_y + 400 + (next_respawn_id * 80);
        WORD spawn_x = safe_lanes[next_respawn_id % 3];
        
        WORD x_distance = ABS(spawn_x - bike_position_x);
        LONG y_distance = base_spawn_y - bike_world_y;
        
        if (y_distance < 120 && x_distance < 32)
        {
            base_spawn_y = bike_world_y + 500;
        }
        
        car_to_check->x = spawn_x;
        car_to_check->y = base_spawn_y;
        car_to_check->off_screen = FALSE;
        car_to_check->speed = car_to_check->target_speed;
        
        KPrintF("Respawned car #%ld BEHIND at y=%ld\n", next_respawn_id, car_to_check->y);
        
        last_respawned_id = next_respawn_id;
        next_respawn_id = (next_respawn_id + 1) % MAX_CARS;
        respawn_timer = respawn_interval;
    }
}