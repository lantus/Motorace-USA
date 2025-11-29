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
#include <string.h>
#include "game.h"
#include "map.h"
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
#include "hud.h"
#include "title.h"
#include "hiscore_entry.h"
#include "hiscore.h"
#include "font.h"
 

static NameEntryState entry_state;
static GameTimer cursor_blink_timer;
static BOOL cursor_visible = TRUE;

void NameEntry_Init(UBYTE position)
{
    entry_state.name[0] = 'A';
    entry_state.name[1] = 'A';
    entry_state.name[2] = 'A';
    entry_state.name[3] = '\0';
    entry_state.cursor_pos = 0;
    entry_state.active = TRUE;
    entry_state.table_position = position;
    
    Timer_StartMs(&cursor_blink_timer, 500);
}

void NameEntry_Update(void)
{
    if (!entry_state.active) return;
    
    // Blink cursor
    if (Timer_HasElapsed(&cursor_blink_timer))
    {
        cursor_visible = !cursor_visible;
        Timer_Reset(&cursor_blink_timer);
    }
}

void NameEntry_Draw(UBYTE *buffer)
{
    if (!entry_state.active) return;
    
    // Draw prompt
    Font_DrawStringCentered(buffer, "ENTER YOUR NAME", 60, 15);
    
    // Draw name with cursor
    char display_name[10];
   // sprintf(display_name, "%c %c %c", 
   //         entry_state.name[0],
   //         entry_state.name[1],
    //        entry_state.name[2]);
    
    Font_DrawStringCentered(buffer, display_name, 80, 15);
    
    // Draw cursor under current letter
    if (cursor_visible)
    {
        char cursor = '_';
        UWORD name_width = Font_MeasureText(display_name);
        UWORD name_x = (192 - name_width) / 2;
        UWORD cursor_x = name_x + (entry_state.cursor_pos * 16);  // 16 pixels per char with spacing
        
        Font_DrawChar(buffer, cursor, cursor_x, 88, 15);
    }
    
    // Draw instructions
    Font_DrawStringCentered(buffer, "UP/DOWN = CHANGE", 110, 15);
    Font_DrawStringCentered(buffer, "FIRE = NEXT", 120, 15);
}

void NameEntry_HandleInput(UBYTE joystick, BOOL fire)
{
    if (!entry_state.active) return;
    
    static UBYTE prev_joy = 0;
    static BOOL prev_fire = FALSE;
    
    // Joystick up - increment letter
    if ((joystick & 0x01) && !(prev_joy & 0x01))  // Up edge triggered
    {
        entry_state.name[entry_state.cursor_pos]++;
        if (entry_state.name[entry_state.cursor_pos] > 'Z')
            entry_state.name[entry_state.cursor_pos] = 'A';
    }
    
    // Joystick down - decrement letter
    if ((joystick & 0x02) && !(prev_joy & 0x02))  // Down edge triggered
    {
        entry_state.name[entry_state.cursor_pos]--;
        if (entry_state.name[entry_state.cursor_pos] < 'A')
            entry_state.name[entry_state.cursor_pos] = 'Z';
    }
    
    // Fire button - move to next letter or finish
    if (fire && !prev_fire)  // Fire edge triggered
    {
        entry_state.cursor_pos++;
        if (entry_state.cursor_pos >= 3)
        {
            entry_state.active = FALSE;  // Done!
        }
    }
    
    prev_joy = joystick;
    prev_fire = fire;
}

BOOL NameEntry_IsComplete(void)
{
    return !entry_state.active;
}

const char* NameEntry_GetName(void)
{
    return entry_state.name;
}