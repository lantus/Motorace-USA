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
#include "hud.h"
#include "title.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "timers.h"
#include "hiscore.h"
#include "font.h"
#include "city_approach.h"
 
#define ZIPPY_LOGO_WIDTH 80
#define ZIPPY_LOGO_HEIGHT 48
#define ZIPPY_LOGO_WIDTH_WORDS 6            // (includes padding)
#define ATTRACT_MOTORBIKE_CENTER_X 80       // X position of the bike during attract

#define ATTRACTMODE_TILES_WIDTH 256
#define ATTRACTMODE_TILES_HEIGHT 512
#define NUM_ROAD_FRAMES 14

#define AMI_LOGO_WIDTH 32
#define AMI_LOGO_HEIGHT 32
#define AMI_LOGO_WIDTH_WORDS 2

extern volatile struct Custom *custom;

BlitterObject zippy_logo;
BlitterObject ami_logo;

APTR zippy_logo_restore_ptrs[4];
APTR *zippy_logo_restore_ptr;

static UWORD title_frames = 0;
static UWORD title_flash_counter = 0;
 
RawMap *city_attract_map;
BitMapEx *city_attract_tiles;

GameTimer attract_timer;
GameTimer logo_flash_timer;
GameTimer blink_timer;
GameTimer hiscore_timer;    // timer for high score table display
GameTimer bike_anim_timer;  // Add this

UBYTE bike_anim_frame = 0;  // Add this to track current frame
BOOL text_visible = TRUE;
UBYTE title_state = TITLE_ATTRACT_INIT;
 

void Title_LoadSprites()
{
    zippy_logo.data = Disk_AllocAndLoadAsset(ZIPPY_LOGO, MEMF_CHIP);
    ami_logo.data = Disk_AllocAndLoadAsset(AMIGA_LOGO, MEMF_CHIP);
}

void Title_Initialize(void)
{
    // Load Zippy Race Logo Bob

    zippy_logo.background = Mem_AllocChip((ZIPPY_LOGO_WIDTH_WORDS * 2) * ZIPPY_LOGO_HEIGHT * 4);
  
    zippy_logo.visible = TRUE;
    zippy_logo.off_screen = FALSE;

    Title_LoadSprites();
 
    // Attract Mode Tiles/Tilemap
    Title_OpenMap();
    Title_OpenBlocks();

    // Palette
    Game_LoadPalette("palettes/intro.pal", intro_colors, BLOCKSCOLORS);
    Game_LoadPalette("palettes/cars_frontview_lv.pal", lv_colors, BLOCKSCOLORS);
    Game_ApplyPalette(intro_colors,BLOCKSCOLORS);
   
    title_state = TITLE_ATTRACT_INIT;
    
    // Reset Positions
    Title_Reset();

    title_state = TITLE_ATTRACT_START;
}

void Title_Draw()
{
    if (title_state == TITLE_ATTRACT_START)
    {
        if (title_frames == 0)
        {
            Copper_SetPalette(0, 0x000);

            BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 16);
            BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 16);

            // So we pre-draw the city skyline ones
            // then on update we only draw the tiles being replaced
      

            City_PreDrawRoad();
            Title_SaveBackground();
            // Draw the hud
            HUD_DrawAll();

            #if defined (DEBUG)
            char line_buffer[8] = {0};
            
            ULongToString(Mem_GetFreeChip(), line_buffer, 8, ' ');
            Font_DrawString(draw_buffer, line_buffer, 32, 170, 9);
            ULongToString(Mem_GetFreeFast(), line_buffer, 8, ' ');
            Font_DrawString(draw_buffer, line_buffer, 32, 180, 10);
            #endif

            title_state = TITLE_ATTRACT_ACCEL;            

        }
    }
 
    if (title_state < TITLE_ATTRACT_INSERT_COIN)
    {
        // Update approach frame based on Y position (perspective)
        MotorBike_UpdateApproachFrame(bike_position_y);
        
        // Only do the 2-frame animation for the closest view (frames 1-2)
        if (bike_position_y >= 176)
        {
            // Calculate animation speed based on bike speed
            UWORD anim_speed = 15 - (bike_speed / 20);
            if (anim_speed < 3) anim_speed = 3;
            
            if (title_frames % anim_speed == 0)
            {
                bike_anim_frame ^= 1;
                
                if (bike_anim_frame == 0)
                {
                    MotorBike_SetFrame(BIKE_FRAME_APPROACH1);
                }
                else
                {
                    MotorBike_SetFrame(BIKE_FRAME_APPROACH2);
                }
            }
        }
        
        City_DrawRoad();
    }
 
    if (title_state == TITLE_ATTRACT_LOGO_DROP)
    { 
        // Blit Logo
        Title_RestoreLogo();
        Title_SaveBackground();
        Title_BlitLogo();
 
        if (Timer_HasElapsed(&logo_flash_timer) == FALSE)
        {
            if (title_flash_counter % 4 == 0) 
            {          
                Copper_SwapColors(7, 12);
            }
        }
        else
        {
            Game_ApplyPalette(intro_colors,BLOCKSCOLORS);
            Timer_Stop(&logo_flash_timer);
            Timer_Start(&hiscore_timer,10);     // 10 Seconds 
            title_state = TITLE_ATTRACT_INSERT_COIN;

            BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 64);
            BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 64);

            WaitBlit();
            Title_BlitLogo();

            AttractMode_DrawText();
 
        }   
    }
    else if (title_state == TITLE_ATTRACT_INSERT_COIN)
    {
        if (Timer_HasElapsed(&hiscore_timer) == TRUE)
        {
            Timer_Reset(&hiscore_timer);
            title_state = TITLE_ATTRACT_HIGHSCORE;

            BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 64);
            BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 64);
            AttractMode_ShowHiScores();
           
        }
 
    }  
    else if (title_state == TITLE_ATTRACT_HIGHSCORE)
    {
        if (Timer_HasElapsed(&hiscore_timer) == TRUE)
        {
            Timer_Reset(&hiscore_timer);
            title_state = TITLE_ATTRACT_CREDITS;

            BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 64);
            BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 64);
            AttractMode_ShowCredits();
           
        }
 
    }  
 
    // Apply vibration offset for distant sprites (ADD THIS)
    WORD vibration_x = MotorBike_GetVibrationOffset();
    MotorBike_Draw(bike_position_x + vibration_x, bike_position_y, 0);

    title_frames++;
    title_flash_counter++;
}

void AttractMode_DrawText(void)
{
    Font_DrawStringCentered(draw_buffer, "PRESS FIRE BUTTON", 120, 12);  

    Font_DrawStringCentered(draw_buffer, COPYRIGHT "1983 IREM CORP", 148, 5);
}

void AttractMode_ShowHiScores(void)
{
    // Draw high score table starting at Y=10
    HiScore_Draw(draw_buffer, 10, 12);   
}

void AttractMode_ShowCredits(void)
{
    Font_DrawString(draw_buffer, "Amiga Conversion : ", 8, 30, 2);     
    Font_DrawString(draw_buffer, "MVG", 12, 40, 5);     
    Font_DrawString(draw_buffer, "Music/SFX Conversion :", 8, 50, 2);     
    Font_DrawString(draw_buffer, "ESTRAYK", 12, 60, 5);     
 
    Font_DrawString(draw_buffer, "Greetings To:", 8, 120, 2);     
    Font_DrawString(draw_buffer, "EAB Forum - Amiga Bill - Tony Galvez", 12, 130, 3);   
    Font_DrawString(draw_buffer, "McGeezer - h0ffman", 12, 140, 3);  
    Font_DrawString(draw_buffer, "JOTD - Galahad", 12, 150, 3);  
    Font_DrawString(draw_buffer, "Wei ju Wu - Bartman", 12, 160, 3);  
    Font_DrawString(draw_buffer, "Toni Willen - Bartman", 12, 170, 3); 
    
    Font_DrawStringCentered(draw_buffer, "Amiga Forever", 200, 7);  

    AttractMode_BlitAmiLogo();
}

void AttractMode_UpdateHiScores(void)
{
     if (Timer_HasElapsed(&hiscore_timer) == TRUE)
     {
        title_state = TITLE_ATTRACT_CREDITS;
        Timer_Reset(&hiscore_timer);

        BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 64);
        BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 64);

        AttractMode_ShowCredits();

     }
}

void AttractMode_UpdateCredits(void)
{
     if (Timer_HasElapsed(&hiscore_timer) == TRUE)
     {
        title_state = TITLE_ATTRACT_INSERT_COIN;
        Timer_Reset(&hiscore_timer);

        BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 64);
        BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 64);

        Title_BlitLogo();

        AttractMode_DrawText();
 
     }
}

void AttractMode_Update(void)
{
    if (!Timer_IsActive(&blink_timer))
    {
        Timer_StartMs(&blink_timer, 500);  // Blink every 500ms
    }
    
    if (Timer_HasElapsed(&blink_timer))
    {
        text_visible = !text_visible;
        
        if (text_visible)
        {
           Font_DrawStringCentered(draw_buffer, "PRESS FIRE BUTTON", 120, 12);  // Color 15 = white
        }
        else
        {
            Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);
        }
        
        Timer_Reset(&blink_timer);
    }
 
}


void Title_Update()
{
    if (JoyFirePressed())
    {
        // Clear screens
        BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
        BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 256);
        
        // Turn off sprites
        Sprites_ClearLower();
        Sprites_ClearHigher();
        
        // Transition to game start
        game_state = GAME_READY;
        GameReady_Initialize();  // Initialize the ready screen
        return;  // Exit early, don't process rest of attract mode
    }
 
    // Animate bike sprite during attract mode - speed based
    if (title_state < TITLE_ATTRACT_INSERT_COIN)
    {
        // Calculate animation speed based on bike speed
        // At min speed (42), change every ~12 frames
        // At max speed (210), change every ~3 frames
        UWORD anim_speed = 15 - (bike_speed / 20);  // Faster bike = smaller number
        if (anim_speed < 3) anim_speed = 3;  // Cap at minimum 3 frames
        
        if (title_frames % anim_speed == 0)
        {
            bike_anim_frame ^= 1;
            
            if (bike_anim_frame == 0)
            {
                MotorBike_SetFrame(BIKE_FRAME_APPROACH1);
            }
            else
            {
                MotorBike_SetFrame(BIKE_FRAME_APPROACH2);
            }
        }
    }

     if (title_state == TITLE_ATTRACT_ACCEL)
    {
        // Slower acceleration - more cinematic buildup
        bike_speed += 1;  // Changed from ACCEL_RATE (2) to 1
        if (bike_speed > MAX_SPEED)
        {
            bike_speed = MAX_SPEED;
        }
        
        UpdateRoadScroll(bike_speed, title_frames);

        if (bike_speed >= MAX_SPEED)
        {
            if (Timer_IsActive(&attract_timer) == FALSE)
            {
                Timer_Start(&attract_timer, 4);  // Changed from 4 to 6 seconds
            }
        }
 
        if (Timer_IsActive(&attract_timer) == TRUE)
        {
            if (Timer_HasElapsed(&attract_timer))
            {
                Timer_Stop(&attract_timer);
                title_state = TITLE_ATTRACT_INTO_HORIZON;
            }     
        }
    }
    else if (title_state == TITLE_ATTRACT_INTO_HORIZON  || 
         title_state == TITLE_ATTRACT_LOGO_DROP)
    {
        // Gentler deceleration
        if (bike_speed > 80 && title_frames % 3 == 0)
        {
            bike_speed -= 1;
        }

        // Faster Y movement for more dramatic perspective - speed based
        UWORD y_speed;

        if (bike_speed > 150)
        {
            y_speed = 2;  
        }
        else if (bike_speed > 100)
        {
            y_speed = 2;   
        }
        else
        {
            y_speed = 2;   
        }
        
        if (title_frames % y_speed == 0)
        {
            bike_position_y--;
        }

        // Trigger logo drop when bike reaches frame 4 (Y <= 128)
        if (bike_position_y <= 128 && title_state == TITLE_ATTRACT_INTO_HORIZON)
        {
            title_state = TITLE_ATTRACT_LOGO_DROP;
        }

        if (title_state == TITLE_ATTRACT_INTO_HORIZON)
        {
            if (bike_speed <= 80)
            {
                bike_speed = 80;
            }
        }

        UpdateRoadScroll(bike_speed, title_frames);

        if (title_state == TITLE_ATTRACT_LOGO_DROP)
        {
            // Logo drops every frame for smooth motion

            // Slower logo drop
            if (title_frames % 4 == 0)
            {
                zippy_logo.y++;
            }

            if (zippy_logo.y >= 16)
            {
                zippy_logo.y = 16;

                if (Timer_IsActive(&logo_flash_timer) == FALSE)
                {
                    Timer_Start(&logo_flash_timer, 3);
                }   
            }
        }
    }
    else if (title_state == TITLE_ATTRACT_INSERT_COIN)
    {
        AttractMode_Update();
    }
    else if (title_state == TITLE_ATTRACT_HIGHSCORE)
    {
        AttractMode_UpdateHiScores();
    } 
    else if (title_state == TITLE_ATTRACT_CREDITS)
    {
        AttractMode_UpdateCredits();
    }

    if (road_tile_idx > NUM_ROAD_FRAMES)
    {
        road_tile_idx = 0;
    }
    else if (road_tile_idx < 0)
    {
        road_tile_idx = NUM_ROAD_FRAMES;
    }
 
}

void Title_Reset()
{
    zippy_logo.x = 56;
    zippy_logo.y = -32;
    zippy_logo.old_x = 56;   // Same as starting x
    zippy_logo.old_y = -32;  // Same as starting y
    road_tile_idx = 0;
    title_frames = 0;

    bike_position_x = 80;
    bike_position_y = 200;

    bike_anim_frame = 0;

    MotorBike_SetFrame(BIKE_FRAME_APPROACH1);
    Timer_StartMs(&bike_anim_timer, 150);
 
}

void Title_BlitLogo()
{
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;

    UWORD source_mod = 12;
    UWORD dest_mod = (SCREENWIDTH / 8) - (6 * 2);
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT<<2) << 6) | 6;
    
    UBYTE *source = (UBYTE*)&zippy_logo.data[0];
    UBYTE *mask = source + 12;
    UBYTE *dest = draw_buffer;
 
    BlitBob2(SCREENWIDTH/2, x, y, admod, bltsize, ZIPPY_LOGO_WIDTH, 
             zippy_logo_restore_ptrs, source, mask, dest);
}

void AttractMode_BlitAmiLogo()
{
    WORD x = 32;
    WORD y = 180;

    UWORD source_mod = 0;
    UWORD dest_mod = (SCREENWIDTH / 8) - (2);
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((AMI_LOGO_HEIGHT<<2) << 6) | 2;
    
    UBYTE *source = (UBYTE*)&ami_logo.data[0];
    
    UBYTE *dest = draw_buffer;
 
    BlitBobSimple(SCREENWIDTH/2, x, y, admod, bltsize, source, dest);
}

void Title_SaveBackground()
{
    WORD x = zippy_logo.x;
    WORD y = zippy_logo.y;

    APTR background = zippy_logo.background;
 
    UWORD source_mod = (SCREENWIDTH / 8) - (6 * 2);  
    UWORD dest_mod = 0;  // Tight-packed
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT << 2) << 6) | 6;  // 192 lines, 6 words
    
    UBYTE *source = draw_buffer;
    UBYTE *dest = background;
    
    BlitBobSimpleSave(SCREENWIDTH/2, x, y, admod, bltsize, source, dest);
}

void Title_RestoreLogo()
{
    WORD old_x = zippy_logo.old_x;
    WORD old_y = zippy_logo.old_y;

    APTR background = zippy_logo.background;
 
    UWORD source_mod = 0;   
    UWORD dest_mod = (SCREENWIDTH / 8) - (6 * 2); 
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    UWORD bltsize = ((ZIPPY_LOGO_HEIGHT << 2) << 6) | 6;  // 192 lines, 6 words
    
    UBYTE *source = background;
    UBYTE *dest = draw_buffer;
    
    BlitBobSimple(SCREENWIDTH/2, old_x, old_y, admod, bltsize, source, dest);
    
    zippy_logo.old_x = zippy_logo.x;
    zippy_logo.old_y = zippy_logo.y;

}

void Title_OpenMap(void)
{
   
	city_attract_map = Disk_AllocAndLoadAsset("maps/city_attract.map",MEMF_PUBLIC);
	
	mapdata = (UWORD *)city_attract_map->data;
	mapwidth = city_attract_map->mapwidth;
	mapheight = city_attract_map->mapheight;   
}
 
void Title_OpenBlocks(void)
{
	city_attract_tiles = BitMapEx_Create(BLOCKSDEPTH, ATTRACTMODE_TILES_WIDTH, ATTRACTMODE_TILES_HEIGHT+64);
	Disk_LoadAsset((UBYTE *)city_attract_tiles->planes[0],"tiles/city_attract.BPL");  
}
 