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
#include "memory.h"
#include "sprites.h"
#include "font.h"

FontSheet game_font;

BOOL LoadFontSheet(const char *filename)
{
    BPTR font_handle = Open(filename, MODE_OLDFILE);
    if (!font_handle) {
        KPrintF("Failed to open font file: %s\n", filename);
        return FALSE;
    }
    
    // Assuming font sheet is 128x64 pixels (16x16 characters)
    LONG font_size = 128 * 64 / 8;  // 1 bitplane, packed
    
    game_font.font_data = (UBYTE*)Mem_AllocChip(font_size);
    if (!game_font.font_data) {
        Close(font_handle);
        return FALSE;
    }
    
    if (Read(font_handle, game_font.font_data, font_size) != font_size) 
    {
        Mem_Free(game_font.font_data, font_size);
        Close(font_handle);
        return FALSE;
    }
    
    game_font.chars_per_row = 16;
    game_font.char_width = 8;
    game_font.char_height = 8;
    
    Close(font_handle);
    return TRUE;
}

void GetFontChar(char c, UBYTE *char_data)
{
    // Calculate character position in font sheet
    UBYTE char_index = (UBYTE)c;
    WORD char_x = (char_index % game_font.chars_per_row) * 8;
    WORD char_y = (char_index / game_font.chars_per_row) * 8;
    
    // Font sheet is 128 pixels wide = 16 bytes per row
    WORD bytes_per_row = 128 / 8;
    
    // Extract 8x8 character
    for (int row = 0; row < 8; row++) 
    {
        WORD src_offset = ((char_y + row) * bytes_per_row) + (char_x / 8);
        char_data[row] = game_font.font_data[src_offset];
        
        // If character spans byte boundary, handle bit shifting
        if (char_x % 8 != 0) 
        {
            UBYTE shift = char_x % 8;
            char_data[row] = (game_font.font_data[src_offset] << shift) |
                            (game_font.font_data[src_offset + 1] >> (8 - shift));
        }
    }
}