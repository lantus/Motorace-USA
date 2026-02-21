#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include "fuel.h"
#include "timers.h"
#include "hud.h"
#include "game.h"

static FuelGauge fuel_gauge;
static GameTimer fuel_timer;

static const char* fuel_pieces[FUEL_STATES] = {
    "\x20\x2b",  // EMPTY0 - Full
    "\x20\x0f",  // EMPTY1
    "\x20\x0e",  // EMPTY2
    "\x20\x0d",  // EMPTY3
    "\x20\x0c",  // EMPTY4
    "\x20\x0b",  // EMPTY5
    "\x20\x0a",  // EMPTY6
    "\x20\x09",  // EMPTY7
    "\x20\x08"   // EMPTY8 - Empty
};

void Fuel_Initialize(void)
{
    fuel_gauge.current_block = 0;
    fuel_gauge.current_state = 0;
    fuel_gauge.fuel_empty = FALSE;
    fuel_gauge.needs_redraw = TRUE;   
    
    Timer_Start(&fuel_timer, 3);
    
    KPrintF("Fuel system initialized\n");
}

void Fuel_Update(void)
{
    if (fuel_gauge.fuel_empty) return;
    
    if (Timer_HasElapsed(&fuel_timer))
    {
        Timer_Reset(&fuel_timer);
        
        fuel_gauge.current_state++;
        
        if (fuel_gauge.current_state >= FUEL_STATES)
        {
            fuel_gauge.current_state = 0;
            fuel_gauge.current_block++;
            
            KPrintF("Fuel block %d depleted\n", fuel_gauge.current_block - 1);
            
            if (fuel_gauge.current_block >= FUEL_BLOCKS)
            {
                fuel_gauge.fuel_empty = TRUE;
                fuel_gauge.current_block = FUEL_BLOCKS - 1;
                fuel_gauge.current_state = FUEL_STATES - 1;
                
                KPrintF("=== FUEL EMPTY - GAME OVER ===\n");
            }
        }
        
    
        fuel_gauge.needs_redraw = TRUE;
    }
}

void Fuel_Decrease(UBYTE blocks)
{
    // Don't decrease if already empty
    if (fuel_gauge.fuel_empty) return;
    
    // Jump forward N blocks
    fuel_gauge.current_block += blocks;
    
    // Reset current state to empty for that block
    fuel_gauge.current_state = FUEL_STATES - 1;  // Set to EMPTY8
    
    // Check if fuel is now empty
    if (fuel_gauge.current_block >= FUEL_BLOCKS)
    {
        fuel_gauge.fuel_empty = TRUE;
        fuel_gauge.current_block = FUEL_BLOCKS - 1;
        fuel_gauge.current_state = FUEL_STATES - 1;
        
        KPrintF("=== FUEL EMPTY after collision ===\n");
    }
    
    fuel_gauge.needs_redraw = TRUE;
    
    KPrintF("Fuel decreased by %d blocks (collision penalty)\n", blocks);
}

void Fuel_Draw(void)
{
    
    if (!fuel_gauge.needs_redraw) return;
    
    fuel_gauge.needs_redraw = FALSE;
    
 
    WORD status_y = 40;
    WORD y_offset = status_y + 8 + (fuel_gauge.current_block * 8);
    
    const char *piece = fuel_pieces[fuel_gauge.current_state];
    HUD_DrawString((char*)piece, 0, y_offset);
}

void Fuel_DrawAll(void)
{
     WORD status_y = 40;
    
    for (UBYTE block = 0; block < FUEL_BLOCKS; block++)
    {
        const char *piece;
        
        if (block < fuel_gauge.current_block)
        {
            
            piece = fuel_pieces[FUEL_STATES - 1];
        }
        else if (block == fuel_gauge.current_block)
        {
          
            piece = fuel_pieces[fuel_gauge.current_state];
        }
        else
        {
        
            piece = fuel_pieces[0];
        }
        
        WORD y_offset = status_y + 8 + (block * 8);
        HUD_DrawString((char*)piece, 0, y_offset);
    }
}

void Fuel_Reset(void)
{
    fuel_gauge.current_block = 0;
    fuel_gauge.current_state = 0;
    fuel_gauge.fuel_empty = FALSE;
    fuel_gauge.needs_redraw = TRUE;
    
    Timer_Start(&fuel_timer, 4);
    
    KPrintF("Fuel reset to full\n");
}

void Fuel_Add(UBYTE blocks)
{
    if (fuel_gauge.current_block >= blocks)
    {
        fuel_gauge.current_block -= blocks;
    }
    else
    {
        fuel_gauge.current_block = 0;
    }
    
    fuel_gauge.current_state = 0;
    fuel_gauge.fuel_empty = FALSE;
    fuel_gauge.needs_redraw = TRUE;   
    
    KPrintF("Added %d fuel blocks\n", blocks);
}

BOOL Fuel_IsEmpty(void)
{
    return fuel_gauge.fuel_empty;
}
