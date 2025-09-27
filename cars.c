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
#include "cars.h"

extern volatile struct Custom *custom;

Car car[MAX_CARS];

void Cars_LoadSprites()
{

    BPTR car_handle = Open(CAR1_FILE, MODE_OLDFILE);

    if (!car_handle) 
    {
        KPrintF("Failed to Car Bob file: %s\n", CAR1_FILE);
        return;
    }
 
    if (Read(car_handle, car[0].bob->planes[0], BOB_DATA_SIZE) != BOB_DATA_SIZE) 
    {
        Close(car_handle);
        return;
    }

     // Load mask (128 bytes)
    car[0].mask = (UBYTE*)Mem_AllocChip(BOB_HEIGHT * (BOB_WIDTH/8));
    
    BPTR mask_handle = Open(CAR1_MASK, MODE_OLDFILE);
    if (mask_handle) {
        Read(mask_handle, car[0].mask, BOB_HEIGHT * (BOB_WIDTH/8));
        Close(mask_handle);
    } else {
        KPrintF("Failed to open mask file\n");
    }
   
    debug_register_bitmap(car[0].mask, "Car BOB", 32, 32, 1, debug_resource_bitmap_interleaved);
    
}

void Cars_Initialize(void)
{
// Only initialize car[0]
    car[0].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[0].background = NULL;  // Don't allocate, we won't use it
    car[0].visible = TRUE;
    car[0].x = 64;
    car[0].y = 100;
    car[0].off_screen = FALSE;
    
    if (!car[0].bob) {
        KPrintF("Failed to allocate car BOB\n");
    }
    
    Cars_LoadSprites();
}


void Cars_RestoreBackground(Car *c)
{

   
}

void Cars_SaveBackground(Car *c)
{

  
}

// Draw BOB to screen
void DrawCarBOB(Car *c)
{

   if (!c->visible) return;
 

    Blitter_CopyMask( c->bob, c->old_x,c->old_y, ScreenBitmap, c->x,c->y ,32, 32, c->mask );
    
   
}
 
 

void Cars_UpdatePosition(Car *c)
{
     if (!c->visible) return;
    
    // Store old position for background restore
    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;
    
 
}

// Main BOB update function
void Cars_Update(void)
{

    // Just draw once, no movement, no save/restore
    static BOOL drawn = FALSE;
 
        DrawCarBOB(&car[0]);
        drawn = TRUE;
     
}
 