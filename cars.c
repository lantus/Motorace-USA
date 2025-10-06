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
 
/* Restore pointers */
APTR restore_ptrs[MAX_CARS * 4];
APTR *restore_ptr;

void Cars_LoadSprites()
{
    car[0].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
    //car[1].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
   // car[2].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
   // car[3].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
   // car[4].data = Disk_AllocAndLoadAsset(CAR1_FILE, MEMF_CHIP);
}

void Cars_Initialize(void)
{
    car[0].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[0].background = Mem_AllocChip(512);
    car[0].visible = TRUE;
    car[0].x = 64;
    car[0].y = 100;
    car[0].off_screen = FALSE;

    /* 
        // Car 1 - Right lane, ahead
    car[1].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[1].background = NULL;
    car[1].visible = TRUE;
    car[1].x = 128;  // Right lane  
    car[1].y = 60;   // Ahead of car 0
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    
    // Car 2 - Center lane, behind
    car[2].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[2].background = NULL;
    car[2].visible = TRUE;
    car[2].x = 96;   // Center lane
    car[2].y = 140;  // Behind car 0
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    
    // Car 3 - Left lane, far ahead
    car[3].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[3].background = NULL;
    car[3].visible = TRUE;
    car[3].x = 48;   // Far left
    car[3].y = 20;   // Far ahead
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
    
    // Car 4 - Right lane, far behind
    car[4].bob = BitMapEx_Create(BLOCKSDEPTH, BOB_WIDTH, BOB_HEIGHT);
    car[4].background = NULL;
    car[4].visible = TRUE;
    car[4].x = 144;  // Far right
    car[4].y = 180;  // Far behind
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE; */
 
    Cars_LoadSprites();

      
 
}

 
  UBYTE *fg_buf32;
  WORD fg_offset2;
void Cars_SaveBackground(Car *c)
{

  
}
 
void Cars_UpdatePosition(Car *c)
{
    if (!c->visible) return;
    
    // Store old position for background restore
    c->old_x = c->x;
    c->old_y = c->y;
    c->moved = TRUE;
    
    // Move car down the screen (towards player)
    c->y += 2;  // Move 2 pixels down per frame
    
    // Wrap around when car goes off bottom of screen
    if (c->y > SCREENHEIGHT ) 
    {
        c->y = 0;  // Start above screen
        
        // Randomize X position within road bounds (16-160 pixels)
        // Simple pseudo-random using car's current position
        c->x =  ((c->x * 17 + c->y * 13) % 128);
        
        // Keep within road boundaries
        if (c->x < 16) c->x = 16;
        if (c->x > 160) c->x = 160;
    }
}

void SaveCarBOB(Car *car)
{
    if (!car->visible ) return;
    
    if (car->x < 0 || car->x > 160 || car->y < -32 || car->y > SCREENHEIGHT + 32) 
    {
        return;
    }
 
    // PARAMETERS FOR 32x32 CAR BOB:

    // Position
    WORD x = car->x;
    WORD y = car->y;

    // Modulos - Optimize these later
    UWORD dest_mod = 4; 
    UWORD source_mod =  (320 - 32) / 8;

    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;

    UWORD bltsize = (128 << 6) | 2;

    // Source data
    UBYTE *source = (UBYTE*)screen.bitplanes;

    // Mask data
    UBYTE *mask = source + 32 / 8  * 1;

    // Destination
    UBYTE *dest = car->background;

    // Background buffer info (using your existing background save system)
    UBYTE *fg_buf3 = car->background;  // Your background save buffer
    UWORD fg_offset = 0;               // No offset into background buffer
    UWORD fg_mod = 320/8 * BLOCKSDEPTH;  // 40 * 4 = 160 bytes per line

    fg_buf32 = fg_buf3;
    fg_offset2 = fg_offset;
    restore_ptrs[0] = fg_buf32;
    restore_ptr = &restore_ptrs[0];

    BlitBobSave(fg_mod, x , y ,  admod, bltsize, restore_ptr, &restore_ptrs[0], source, mask, dest);
 
    // Call the generic BlitBob function
  //  BlitBob(x, y, admod, bltsize,
  //          restore_ptrs,    // Will be filled with pointers
  //          source,          // Car sprite data
 //           mask,            // Mask data  
  //          dest,            // Screen buffer
  //          fg_buf3,         // Background save buffer  
  //          fg_offset,       // Offset into background buffer
  //          fg_mod);         // Bytes per line in screen
    
    // Store the restore pointers for later use
 
    car->needs_restore = TRUE;
}

int once  = 0;
void DrawCarBOB(Car *car)
{
    if (!car->visible ) return;
    
    if (car->x < 0 || car->x > 160 || car->y < -32 || car->y > SCREENHEIGHT + 32) 
    {
        return;
    }
 
    // PARAMETERS FOR 32x32 CAR BOB:

    // Position
    WORD x = car->x;
    WORD y = car->y;

    UWORD source_mod = 4; 
    UWORD dest_mod =  (320 - 32) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = (128 << 6) | 2;
    
    // Source data
    UBYTE *source = (UBYTE*)&car->data[0];
    
    // Mask data
    UBYTE *mask = source + 32 / 8  * 1;
    
    // Destination
    UBYTE *dest = screen.bitplanes;
    
 

    BlitBob2(160, x, y, admod, bltsize, restore_ptrs, source, mask, dest);
  
    car->needs_restore = TRUE;
}

// Main BOB update function
void Cars_Update(void)
{

    // First pass: Restore backgrounds from previous frame using BlitBob's restore pointers
    for (int i = 0; i < MAX_CARS; i++) 
    {
      if (car[i].needs_restore && !car[i].off_screen)
      {

        restore_ptr = &restore_ptrs[0];
        UWORD source_mod = 4; 
        UWORD dest_mod =  (320 - 32) / 8;
        ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
        UWORD bltsize = (128 << 6) | 2;

        //   if (once == 0)
        //   BlitClearScreen(screen.bitplanes, 320 * 2 << 6 | 256 / 8 );
          ULONG y_offset =  car->old_y * 160;
            WORD x_byte_offset = car->old_x >> 3;  // Divide by 8 for byte offset
            ULONG total_offset = y_offset + x_byte_offset;

        restore_ptr[1] = car[i].background;
        restore_ptr[0] = screen.bitplanes + total_offset ;
        BlitRestoreBobs(admod,bltsize,1,restore_ptr);
        once = 1;
      }

    car[i].needs_restore = FALSE;  // Clear restore flag

  }



  if (car[0].moved)
    SaveCarBOB(&car[0]);
  
  // Third pass: Draw all cars (BlitBob handles background saving automatically)
  for (int i = 0; i < MAX_CARS; i++)
  {
    if (car[i].visible && !car[i].off_screen) 
    {
      DrawCarBOB(&car[i]);  // This calls BlitBob which saves background and draws
    }

  }

  // Second pass: Update positions for all cars
  for (int i = 0; i < MAX_CARS; i++) 
  {
    if (car[i].visible) 
    {
      Cars_UpdatePosition(&car[i]);
    }
  }
  
}
 