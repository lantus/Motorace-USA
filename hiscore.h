#ifndef HISCORE_H
#define HISCORE_H

#include <exec/types.h>

#define MAX_HISCORE_ENTRIES 10
#define HISCORE_NAME_LENGTH 3

typedef struct {
    ULONG score;
    UBYTE rank;        // 1-99, or 0 for "Retire"
    char name[HISCORE_NAME_LENGTH + 1];  // 3 chars + null terminator
} HiScoreEntry;

typedef struct {
    HiScoreEntry entries[MAX_HISCORE_ENTRIES];
} HiScoreTable;

// Initialize with default high scores
void HiScore_Initialize(void);
// Check if a score qualifies for the table (returns position 1-10, or 0 if doesn't qualify)
UBYTE HiScore_CheckQualifies(ULONG score);
// Insert a new high score (returns position where inserted, 1-10)
UBYTE HiScore_Insert(ULONG score, UBYTE rank, const char *name);
// Get entry at position (1-10)
HiScoreEntry* HiScore_GetEntry(UBYTE position);
// Get the high score table
HiScoreTable* HiScore_GetTable(void);
// Render the high score table to viewport
void HiScore_Draw(UBYTE *buffer, UWORD start_y, UBYTE color);
// Save/Load high scores (optional - implement later for persistence)
void HiScore_Save(void);
void HiScore_Load(void);
// Format rank as string (e.g., "1st", "2nd", "Retire")
void HiScore_FormatRank(UBYTE rank, char *buffer);

#endif // HISCORE_H