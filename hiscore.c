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
 
#include "memory.h"
#include "sprites.h"
#include "game.h"
#include "hiscore.h"
#include "hud.h"
#include "font.h"


static HiScoreTable hiscore_table;

// Default high score table (from your image)
static const HiScoreEntry default_scores[MAX_HISCORE_ENTRIES] = {
    {68000, 01, "AMI"},
    {32700, 03, "RRT"},
    {29400, 02, "ESE"},
    {24800, 10, "MEM"},
    {21900, 00, "NUO"},     // 0 = Retire
    {16100, 11, "OSI"},
    {13300, 00, "TAI"},     // 0 = Retire
    {9600,  40, "IEO"},    
    {7200,  40, "DIM"},    
    {5000,  00, "MVG"}      // 0 = Retire
};

void HiScore_Initialize(void)
{
    // Copy default scores
    for (int i = 0; i < MAX_HISCORE_ENTRIES; i++)
    {
        hiscore_table.entries[i] = default_scores[i];
    }
    
    // Try to load saved scores (implement later)
    // HiScore_Load();
}

UBYTE HiScore_CheckQualifies(ULONG score)
{
    // Check if score is higher than any entry
    for (UBYTE i = 0; i < MAX_HISCORE_ENTRIES; i++)
    {
        if (score > hiscore_table.entries[i].score)
        {
            return i + 1;  // Return position (1-10)
        }
    }
    
    return 0;  // Doesn't qualify
}

UBYTE HiScore_Insert(ULONG score, UBYTE rank, const char *name)
{
    UBYTE insert_pos = HiScore_CheckQualifies(score);
    
    if (insert_pos == 0)
        return 0;  // Doesn't qualify
    
    insert_pos--;  // Convert to 0-based index
    
    // Shift entries down
    for (int i = MAX_HISCORE_ENTRIES - 1; i > insert_pos; i--)
    {
        hiscore_table.entries[i] = hiscore_table.entries[i - 1];
    }
    
    // Insert new entry
    hiscore_table.entries[insert_pos].score = score;
    hiscore_table.entries[insert_pos].rank = rank;
    
    // Copy name (max 3 chars)
    //strncpy(hiscore_table.entries[insert_pos].name, name, HISCORE_NAME_LENGTH);
    hiscore_table.entries[insert_pos].name[HISCORE_NAME_LENGTH] = '\0';
    
    // Save to disk (implement later)
    // HiScore_Save();
    
    return insert_pos + 1;  // Return 1-based position
}

HiScoreEntry* HiScore_GetEntry(UBYTE position)
{
    if (position < 1 || position > MAX_HISCORE_ENTRIES)
        return NULL;
    
    return &hiscore_table.entries[position - 1];
}

HiScoreTable* HiScore_GetTable(void)
{
    return &hiscore_table;
}

ULONG HiScore_GetTopScore(void)
{
    return hiscore_table.entries[0].score;
}

void HiScore_FormatRank(UBYTE rank, char *buffer)
{
    if (rank == 0)
    {
        // Manually copy "Retire"
        buffer[0] = 'R';
        buffer[1] = 'e';
        buffer[2] = 't';
        buffer[3] = 'i';
        buffer[4] = 'r';
        buffer[5] = 'e';
        buffer[6] = '\0';
        return;
    }
    
    int pos = 0;
    
    // Convert rank to string
    if (rank >= 100)
    {
        buffer[pos++] = '0' + (rank / 100);
        buffer[pos++] = '0' + ((rank / 10) % 10);
        buffer[pos++] = '0' + (rank % 10);
    }
    else if (rank >= 10)
    {
        buffer[pos++] = '0' + (rank / 10);
        buffer[pos++] = '0' + (rank % 10);
    }
    else
    {
        buffer[pos++] = '0' + rank;
    }
    
    // Determine suffix (st, nd, rd, th)
    UBYTE last_two_digits = rank % 100;
    const char *suffix;
    
// Special cases: 11th, 12th, 13th
    if (last_two_digits >= 11 && last_two_digits <= 13)
    {
        suffix = "\x9c\x9d";  // "th" tiles
    }
    else
    {
        UBYTE last_digit = rank % 10;
        switch (last_digit)
        {
            case 1: suffix = "\x9e\x9f"; break;  // "st" tiles
            case 2: suffix = "\x98\x99"; break;  // "nd" tiles
            case 3: suffix = "\x9a\x9b"; break;  // "rd" tiles
            default: suffix = "\x9c\x9d"; break; // "th" tiles
        }
    }
    
    // Append suffix
    buffer[pos++] = suffix[0];
    buffer[pos++] = suffix[1];
    buffer[pos] = '\0';
}

void HiScore_Draw(UBYTE *buffer, UWORD start_y, UBYTE color)
{
    char line_buffer[32];
    UWORD y = start_y;
    
    WaitBlit();
    // Draw title
    Font_DrawStringCentered(buffer, " BEST 10 PLAYERS", y, color);
    y += 20;
    
    WaitBlit();
    // Draw header
    Font_DrawString(buffer, "NO", 8, y, color);
    Font_DrawString(buffer, "SCORE", 32, y, color);
    Font_DrawString(buffer, "RANK", 88, y, color);
    Font_DrawString(buffer, "NAME", 144, y, color);
    y += 16;
    WaitBlit();
    // Draw each entry
    for (int i = 0; i < MAX_HISCORE_ENTRIES; i++)
    {
        HiScoreEntry *entry = &hiscore_table.entries[i];
        
        // Position number - use your existing function
        ULongToString(i + 1, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, 8, y, color);
        
        // Score - use your existing function
        ULongToString(entry->score, line_buffer, 5, ' ');
        Font_DrawString(buffer, line_buffer, 32, y, color);
        
        // Rank
        char rank_str[8];
        HiScore_FormatRank(entry->rank, rank_str);
        Font_DrawString(buffer, rank_str, 88, y, color);
        
        // Name
        Font_DrawString(buffer, entry->name, 152, y, color);
        
        y += 15;
    }
}

// Placeholder functions for save/load (implement later)
void HiScore_Save(void)
{
    // TODO: Save to file
    // Could use dos.library to write to "PROGDIR:hiscore.dat"
}

void HiScore_Load(void)
{
    // TODO: Load from file
    // If file doesn't exist, keep default scores
}