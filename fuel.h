#ifndef FUEL_H
#define FUEL_H

#include <exec/types.h>

#define FUEL_BLOCKS 13
#define FUEL_STATES 9  // 0-8 (0=full, 8=empty)

// Fuel gauge state
typedef struct {
    UBYTE current_block;     // 0-12 (which block is currently depleting)
    UBYTE current_state;     // 0-8 (EMPTY0 to EMPTY8)
    BOOL fuel_empty;         // TRUE when all fuel is gone
    BOOL needs_redraw;  
} FuelGauge;

// Initialize fuel system
void Fuel_Initialize(void);

// Update fuel (call every frame) - depletes over time
void Fuel_Update(void);

// Draw fuel gauge to HUD sprites
void Fuel_Draw(void);

// Check if fuel is empty
BOOL Fuel_IsEmpty(void);

// Reset fuel to full
void Fuel_Reset(void);

// Add fuel (for refueling checkpoints)
void Fuel_Add(UBYTE blocks);

void Fuel_DrawAll(void);  // Draw all blocks initially
void Fuel_Draw(void);     // Update current block only
void Fuel_Decrease(UBYTE blocks);  // Decrease fuel by N blocks (for collisions)
#endif // FUEL_H