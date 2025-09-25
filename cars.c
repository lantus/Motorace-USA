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
#include "memory.h"
#include "sprites.h"
#include "motorbike.h"
#include "hardware.h"
#include "cars.h"

extern volatile struct Custom *custom;

CarBOB car_bobs[MAX_CARS];

void InitCarBOBs(void)
{
    for (int i = 0; i < MAX_CARS; i++) 
    {
        car_bobs[i].background = (UBYTE*)Mem_AllocChip(BOB_DATA_SIZE);
        car_bobs[i].bob_data = (UBYTE*)Mem_AllocChip(BOB_DATA_SIZE);
        car_bobs[i].visible = FALSE;
        car_bobs[i].moved = FALSE;
        
        if (!car_bobs[i].background || !car_bobs[i].bob_data) 
        {
            KPrintF("Failed to allocate BOB memory");
        }
    }
}

// Create test patterns for performance testing
void CreateTestBOBData(UBYTE *bob_data, int color_pattern)
{
memset(bob_data, 0, BOB_DATA_SIZE);
    
    // 3-plane interleaved: 12 bytes per line (4 bytes × 3 planes)
    const int bytes_per_line = 4 * BOB_PLANES; // 4 * 3 = 12
    
    for (int y = 0; y < BOB_HEIGHT; y++) {
        for (int x_byte = 0; x_byte < 4; x_byte++) {
            int pixel_offset = (y * bytes_per_line) + (x_byte * BOB_PLANES);
            
            UBYTE plane0 = 0, plane1 = 0, plane2 = 0;
            
            // Create car silhouette
            if (y >= 6 && y <= 26 && x_byte >= 1 && x_byte <= 2) {
                switch(color_pattern % 4) {
                    case 0: // Color 1 (BOB plane 0 -> screen plane 1)
                        plane0 = 0xFF;
                        break;
                    case 1: // Color 2 (BOB plane 1 -> screen plane 2)
                        plane1 = 0xFF;
                        break;
                    case 2: // Color 4 (BOB plane 2 -> screen plane 3)
                        plane2 = 0xFF;
                        break;
                    case 3: // Color 3 (BOB planes 0+1 -> screen planes 1+2)
                        plane0 = 0xFF;
                        plane1 = 0xFF;
                        break;
                }
            }
            
            // Wheels
            if (y >= 24 && y <= 27 && (x_byte == 0 || x_byte == 3)) {
                plane0 = plane1 = plane2 = 0xFF; // Color 7
            }
            
            bob_data[pixel_offset + 0] = plane0;
            bob_data[pixel_offset + 1] = plane1;
            bob_data[pixel_offset + 2] = plane2;
        }
    }
}


// Save background before drawing BOB
void SaveBOBBackground(CarBOB *bob)
{
 
}

// Restore background where BOB was
void RestoreBOBBackground(CarBOB *bob)
{
    
}

// Draw BOB to screen
void DrawCarBOB(CarBOB *bob)
{
    if (!bob->visible || !bob->bob_data) return;
    
    if (bob->x < 0 || bob->x > 160 || bob->y < -32 || bob->y > SCREENHEIGHT + 32) {
        bob->off_screen = TRUE;
        return;
    }
    bob->off_screen = FALSE;
    
    WORD screen_x_byte = bob->x >> 3;
    LONG screen_offset = (bob->y * 24 * BLOCKSDEPTH) + (screen_x_byte * BLOCKSDEPTH);
    
    // We need to blit to screen planes 1, 2, 3 (skipping plane 0)
    UBYTE *screen_plane1 = frontbuffer + screen_offset + 1; // Skip plane 0
    
    HardWaitBlit();
    
    // Blit 3-plane BOB data directly
    custom->bltcon0 = 0x9F0;  // Copy A to D
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    
    // Source: 3-plane interleaved (12 bytes per line)
    // Dest: 4-plane interleaved (16 bytes per line), but we write to planes 1-3 only
    custom->bltamod = 0;  // BOB: 12 bytes per line, copy 12 bytes = no modulo
    custom->bltdmod = (24 * BLOCKSDEPTH) - (4 * BOB_PLANES);  // Screen: 160 - 12 = 148
    
    custom->bltapt = bob->bob_data;
    custom->bltdpt = screen_plane1;  // Start at plane 1, not plane 0
    custom->bltsize = (BOB_HEIGHT << 6) | 2;  // 32 lines × 3 words (12 bytes)
}

// Spawn a new test car
void SpawnTestCar(int index)
{
    car_bobs[index].x = 16 + ((index * 32) % 128);  // Spread across game width
    car_bobs[index].y = -32;
    car_bobs[index].old_x = car_bobs[index].x;
    car_bobs[index].old_y = car_bobs[index].y;
    car_bobs[index].visible = TRUE;
    car_bobs[index].moved = TRUE;
    
    CreateTestBOBData(car_bobs[index].bob_data, index % 4);
}

// Initialize test cars
void InitTestBOBs(void)
{
    InitCarBOBs();
    
    for (int i = 0; i < MAX_CARS; i++) {
        SpawnTestCar(i);
        car_bobs[i].y = -64 - (i * 40);  // Stagger start positions
    }
}

void UpdateCarPosition(CarBOB *car)
{
    if (!car->visible) return;
    
    // Store old position for background restore
    car->old_x = car->x;
    car->old_y = car->y;
    car->moved = TRUE;
    
    // Move car down the screen - base speed plus variation
    // Cars move at different speeds to create traffic patterns
    static UBYTE car_speeds[MAX_CARS] = {3, 2, 4, 3, 2}; // Different speeds per car
    static UBYTE lane_positions[5] = {24, 48, 72, 96, 120}; // 5 lanes across 144px game area
    static UBYTE spawn_counter = 0;
    
    // Get car index by pointer arithmetic
    int car_index = car - car_bobs;
    
    // Move car down
    car->y += car_speeds[car_index];
    
    // Add some horizontal drift for realism (simple deterministic pattern)
    if ((spawn_counter + car_index * 17) % 60 < 30) {
        car->x += ((spawn_counter + car_index * 23) % 3) - 1; // -1, 0, or 1 pixel drift
    }
    
    // Keep cars in their lanes (with some tolerance)
    UBYTE target_lane = car_index % 5;
    if (car->x < lane_positions[target_lane] - 8) {
        car->x++;
    } else if (car->x > lane_positions[target_lane] + 8) {
        car->x--;
    }
    
    // Clamp to game area bounds (16-160 for 144px wide game area)
    if (car->x < 16) car->x = 16;
    if (car->x > 160) car->x = 160;
    
    // Check if car has moved off bottom of screen
    if (car->y > SCREENHEIGHT + 32) {
        // Respawn at top with staggered timing
        car->y = -32 - ((spawn_counter + car_index * 13) % 64); // Stagger -32 to -96
        car->x = lane_positions[(car_index + spawn_counter/30) % 5]; // Cycle through lanes
        
        // Vary the car graphics for visual variety
        CreateTestBOBData(car->bob_data, (car_index + spawn_counter/15) % 4);
        
        spawn_counter++;
    }
    
    // Handle collision with player bike (basic bounding box check)
    // Player bike is at bike_position_x, bike_position_y from motorbike.h
    if (car->visible && 
        car->x < bike_position_x + 24 && car->x + 32 > bike_position_x &&
        car->y < bike_position_y + 24 && car->y + 32 > bike_position_y) {
        
        // Collision detected - for now just move car aside
        // In full game this would trigger crash sequence
        car->x += (car->x < bike_position_x) ? -16 : 16;
    }
}

// Main BOB update function
void UpdateCars(void)
{
   // Only restore BOBs that were actually drawn last frame
    for (int i = 0; i < MAX_CARS; i++) {
        if (car_bobs[i].needs_restore && !car_bobs[i].off_screen) 
        {
            RestoreBOBBackground(&car_bobs[i]);
        }
        car_bobs[i].needs_restore = FALSE;
    }
    
    // Update positions and draw
    for (int i = 0; i < MAX_CARS; i++) 
    {
        UpdateCarPosition(&car_bobs[i]);
        
        car_bobs[i].off_screen = (car_bobs[i].x < -32 || car_bobs[i].x > 192 || 
                                  car_bobs[i].y < -32 || car_bobs[i].y > SCREENHEIGHT + 32);
        
        if (car_bobs[i].visible && !car_bobs[i].off_screen) 
        {
            SaveBOBBackground(&car_bobs[i]);
            DrawCarBOB(&car_bobs[i]);
            car_bobs[i].needs_restore = TRUE;
        }
    }
 
}

// Cleanup
void FreeCarBOBs(void)
{
    for (int i = 0; i < MAX_CARS; i++) {
        if (car_bobs[i].background) {
            Mem_Free(car_bobs[i].background, BOB_DATA_SIZE);
        }
        if (car_bobs[i].bob_data) {
            Mem_Free(car_bobs[i].bob_data, BOB_DATA_SIZE);
        }
    }
}