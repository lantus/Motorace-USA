#ifndef STAGEPROGRESS_H
#define STAGEPROGRESS_H

#include <exec/types.h>

#define PROGRESS_BLOCKS_PER_STAGE 3  // 2 for overhead, 1 for frontview
#define PROGRESS_STATES 9            // 0-7 (0=empty, 8=full)

typedef struct {
    UBYTE current_stage;     // 0-5 (LA, LV, HOU, STL, CHI, NY)
    UBYTE overhead_block;    // 0-1 (which overhead block)
    UBYTE overhead_state;    // 0-8 (progress within block)
    UBYTE frontview_state;   // 0-8 (progress in frontview block)
    LONG  current_map_pos;   // current position of the map
    LONG  mapsize;           // total size of the map
    BOOL needs_redraw;
} StageProgress;

extern StageProgress stage_progress;

void StageProgress_Initialize(void);
void StageProgress_SetStage(UBYTE stage);  // Set current stage (0-5)

// Update overhead progress based on map position
void StageProgress_UpdateOverhead(LONG mapposy);

// Update frontview progress based on cars passed
void StageProgress_UpdateFrontview(UBYTE cars_passed, UBYTE total_cars);

void StageProgress_DrawOverhead(void);
void StageProgress_DrawAll(void);
void StageProgress_FillAll(void);
void StageProgress_DrawFrontview(void);
void StageProgress_DrawAllEmpty(void);

#endif // STAGEPROGRESS_H