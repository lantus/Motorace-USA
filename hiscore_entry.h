#ifndef HISCORE_ENTRY_H
#define HISCORE_ENTRY_H

#include <exec/types.h>

// State for name entry screen
typedef struct {
    char name[4];      // 3 chars + null
    UBYTE cursor_pos;  // 0-2
    BOOL active;
    UBYTE table_position;  // Where this will be inserted
} NameEntryState;

// Initialize name entry screen
void NameEntry_Init(UBYTE position);

// Update name entry (call each frame)
void NameEntry_Update(void);

// Draw name entry screen
void NameEntry_Draw(UBYTE *buffer);

// Handle input (joystick or keyboard)
void NameEntry_HandleInput(UBYTE joystick, BOOL fire);

// Check if entry is complete
BOOL NameEntry_IsComplete(void);

// Get entered name
const char* NameEntry_GetName(void);

#endif // HISCORE_ENTRY_H