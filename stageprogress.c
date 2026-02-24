#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include "stageprogress.h"
#include "hud.h"
#include "game.h"

StageProgress stage_progress;

static const char* progress_pieces[PROGRESS_STATES] = 
{
    "\x20\x10",  // PIECE_0 - Empty
    "\x20\x90",  // PIECE_1
    "\x20\x91",  // PIECE_2
    "\x20\x92",  // PIECE_3
    "\x20\x93",  // PIECE_4
    "\x20\x94",  // PIECE_5
    "\x20\x95",  // PIECE_6
    "\x20\x96",  // PIECE_7  
    "\x20\x97"   // PIECE_8 - Full
};

void StageProgress_Initialize(void)
{
    stage_progress.current_stage = 0;  // Start at LA
    stage_progress.overhead_block = 0;
    stage_progress.overhead_state = 0;
    stage_progress.frontview_state = 0;
    stage_progress.needs_redraw = TRUE;
    
    KPrintF("Stage progress initialized - Stage %d\n", stage_progress.current_stage);
}

void StageProgress_SetStage(UBYTE stage)
{
    stage_progress.current_stage = stage;
    stage_progress.overhead_block = 0;
    stage_progress.overhead_state = 0;
    stage_progress.frontview_state = 0;
    stage_progress.needs_redraw = TRUE;

    HUD_ClearSpriteBlock(1, 140);
    
    KPrintF("Stage progress set to stage %d\n", stage);
}

void StageProgress_UpdateOverhead(LONG mapposy)
{
    // Calculate progress through overhead stage
    // mapposy starts high and decreases to 0 (end of stage)
    LONG distance_traveled = stage_progress.mapsize - mapposy;
    LONG total_distance = stage_progress.mapsize;
    stage_progress.current_map_pos = distance_traveled;

    if (total_distance <= 0) return;
    
    // Progress from 0.0 to 1.0
    LONG progress = (distance_traveled * 100) / total_distance;  // 0-100%
    
    // Map to 2 blocks (0-1), each with 8 states
    // Block 0: 0-50%, Block 1: 50-100%
    if (progress < 50)
    {
        stage_progress.overhead_block = 0;
        // Map 0-50% to states 0-7
        stage_progress.overhead_state = (progress * PROGRESS_STATES) / 50;
    }
    else
    {
        stage_progress.overhead_block = 1;
        // Map 50-100% to states 0-7
        stage_progress.overhead_state = ((progress - 50) * PROGRESS_STATES) / 50;
    }
    
    if (stage_progress.overhead_state >= PROGRESS_STATES)
    {
        stage_progress.overhead_state = PROGRESS_STATES - 1;
    }
    
    stage_progress.needs_redraw = TRUE;
}

void StageProgress_UpdateFrontview(UBYTE cars_passed, UBYTE total_cars)
{
    if (total_cars == 0) return;
    
    stage_progress.frontview_state = (cars_passed * 5) / total_cars;
    
    if (stage_progress.frontview_state > 5)
    {
        stage_progress.frontview_state = 5;
    }
    
    stage_progress.needs_redraw = TRUE;
}

void StageProgress_DrawOverhead(void)
{
    static UBYTE last_overhead_state = 0xFF;  // Track previous state  
    static UBYTE last_overhead_block = 0xFF;

    if (!stage_progress.needs_redraw) return;

     if (stage_progress.overhead_state == last_overhead_state && 
        stage_progress.overhead_block == last_overhead_block)
    {
        return;  // No change, skip drawing
    }
    
    // State changed - update and draw
    last_overhead_state = stage_progress.overhead_state;
    last_overhead_block = stage_progress.overhead_block;
    
    stage_progress.needs_redraw = FALSE;
    
    // Calculate base block for current stage
    UBYTE stage_base_block = 0;
    
    WORD base_y = 140;  // LA marker position

    UBYTE block = stage_base_block + stage_progress.overhead_block;
    
    const char *piece = progress_pieces[stage_progress.overhead_state];
    
    WORD y_offset = base_y - (block * 8);
  
    HUD_DrawString((char*)piece, 1, y_offset);
 
}

void StageProgress_DrawFrontview(void)
{
    static UBYTE last_frontview_state = 0xFF;   
    
    if (!stage_progress.needs_redraw) return;
    
    // Only draw if state actually changed
    if (stage_progress.frontview_state == last_frontview_state)
    {
        return;  // No change, skip drawing
    }
    
    // State changed - update and draw
    last_frontview_state = stage_progress.frontview_state;
    
    stage_progress.needs_redraw = FALSE;
    
    // Calculate base block for current stage
    UBYTE stage_base_block = stage_progress.current_stage * PROGRESS_BLOCKS_PER_STAGE;
    
    WORD base_y = 140;  // LA marker position

    // Frontview is always the 3rd block (index 2)
    UBYTE block = stage_base_block + 2;
    
    const char *piece = progress_pieces[stage_progress.frontview_state];
    
    WORD y_offset = base_y - (block * 8);
  
    HUD_DrawString((char*)piece, 1, y_offset);

    KPrintF("Frontview progress - Block %d, frontview_state=%d\n", 
            block, stage_progress.frontview_state);
}

void StageProgress_DrawAllEmpty(void)
{
    //  Force all blocks to show as empty (PIECE_0)
    WORD base_y = 140;
    
    for (UBYTE block = 0; block < 12; block++)
    {
        WORD y_offset = base_y - (block * 8);
        HUD_ClearSpriteBlock(1, y_offset);
        HUD_DrawString((char*)progress_pieces[0], 1, y_offset);  
    }
    
    KPrintF("Stage progress - all blocks drawn as EMPTY\n");
}

void StageProgress_DrawAll(void)
{
    WORD base_y = 140;
 
    for (UBYTE block = 0; block < 12; block++)
    {
        UBYTE block_stage = block / PROGRESS_BLOCKS_PER_STAGE;
        UBYTE block_within_stage = block % PROGRESS_BLOCKS_PER_STAGE;
        
        const char *piece;
        
        if (block_stage < stage_progress.current_stage)
        {
            // Previous stages - all full
            piece = progress_pieces[PROGRESS_STATES - 1];
        }
        else if (block_stage == stage_progress.current_stage)
        {
            // Current stage
            if (block_within_stage < 2)
            {
                // Overhead blocks
                if (block_within_stage < stage_progress.overhead_block)
                {
                    piece = progress_pieces[PROGRESS_STATES - 1];
                }
                else if (block_within_stage == stage_progress.overhead_block)
                {
                    piece = progress_pieces[stage_progress.overhead_state];
                }
                else
                {
                    piece = progress_pieces[0];
                }
            }
            else
            {
                // Frontview block
                piece = progress_pieces[stage_progress.frontview_state];
            }
        }
        else
        {
            // Future stages - empty
            piece = progress_pieces[0];
        }
        
        WORD y_offset = base_y - (block * 8);
        HUD_DrawString((char*)piece, 1, y_offset);
    }
}

void StageProgress_FillAll(void)
{
    stage_progress.current_stage = 4;   
    stage_progress.overhead_block = 1;
    stage_progress.overhead_state = PROGRESS_STATES - 1;
    stage_progress.frontview_state = PROGRESS_STATES - 1;
    stage_progress.needs_redraw = TRUE;
    
    KPrintF("Stage progress filled - attract mode\n");
}