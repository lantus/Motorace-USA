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
    
    char line_buffer[32];
    UWORD y = 10;
    UBYTE normal_color = 12;
    UBYTE entry_color = 2;   /* Red for the entry row */
    
    WaitBlit();
    Font_DrawStringCentered(buffer, " BEST 10 PLAYERS", y, normal_color);
    y += 20;
    
    WaitBlit();
    Font_DrawString(buffer, "NO", 8, y, normal_color);
    Font_DrawString(buffer, "SCORE", 32, y, normal_color);
    Font_DrawString(buffer, "RANK", 88, y, normal_color);
    Font_DrawString(buffer, "NAME", 144, y, normal_color);
    y += 16;
    
    WaitBlit();
    
    HiScoreTable *table = HiScore_GetTable();
    UBYTE insert_idx = entry_state.table_position - 1;  /* 0-based */
    
    for (int i = 0; i < MAX_HISCORE_ENTRIES; i++)
    {
        BOOL is_entry_row = (i == insert_idx);
        UBYTE color = is_entry_row ? entry_color : normal_color;
        
        /* Position number */
        ULongToString(i + 1, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, 8, y, color);
        
        if (is_entry_row)
        {
            /* Score — show the player's score */
            ULongToString(game_score, line_buffer, 5, ' ');
            Font_DrawString(buffer, line_buffer, 32, y, color);
            
            /* Rank */
            char rank_str[8];
            HiScore_FormatRank(game_rank, rank_str);
            Font_DrawString(buffer, rank_str, 88, y, color);
            
            /* Name — show current entry with cursor */
            char name_display[4];
            name_display[0] = entry_state.name[0];
            name_display[1] = entry_state.name[1];
            name_display[2] = entry_state.name[2];
            name_display[3] = '\0';
            
            /* Draw each character — blink the active one */
            for (int c = 0; c < 3; c++)
            {
                char ch[2] = { entry_state.name[c], '\0' };
                WORD char_x = 152 + (c * 8);
                
                if (c == entry_state.cursor_pos && cursor_visible)
                {
                    Font_DrawString(buffer, ch, char_x, y, entry_color);
                }
                else if (c == entry_state.cursor_pos && !cursor_visible)
                {
                    /* Blink — clear this character */
                    Font_DrawString(buffer, " ", char_x, y, entry_color);
                }
                else
                {
                    Font_DrawString(buffer, ch, char_x, y, entry_color);
                }
            }
        }
        else
        {
            HiScoreEntry *entry = &table->entries[i];
            
            /* Score */
            ULongToString(entry->score, line_buffer, 5, ' ');
            Font_DrawString(buffer, line_buffer, 32, y, color);
            
            /* Rank */
            char rank_str[8];
            HiScore_FormatRank(entry->rank, rank_str);
            Font_DrawString(buffer, rank_str, 88, y, color);
            
            /* Name */
            Font_DrawString(buffer, entry->name, 152, y, color);
        }
        
        y += 15;
    }
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