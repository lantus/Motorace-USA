#ifndef RANKING_H
#define RANKING_H

#include <exec/types.h>

// Ranking tier structure
typedef struct {
    UBYTE rank_min;
    UBYTE rank_max;
    UWORD points;
} RankingTier;

// Ranking data for a checkpoint
typedef struct {
    UBYTE checkpoint_number;     // 1-6
    char city_name[16];          // "LAS VEGAS", "CHICAGO", etc.
    UBYTE player_rank;           // Player's achieved rank (1-99)
    UWORD player_points;         // Points earned
    UWORD average_speed;         // Average speed in km/h
    UBYTE todays_best_rank;      // Today's best ranking
    BOOL new_record;             // Did player set a new record?
} RankingData;

// Initialize ranking system
void Ranking_Initialize(void);

// Set ranking data for current checkpoint
void Ranking_SetData(UBYTE checkpoint, const char *city, UBYTE rank, UWORD points, 
                     UWORD avg_speed, UBYTE best_rank, BOOL record);

// Draw the ranking screen to buffer
void Ranking_Draw(UBYTE *buffer);

// Get current ranking data
RankingData* Ranking_GetData(void);

void Ranking_DrawCityBackdrop(WORD y_offset,UBYTE *buffer);

#endif // RANKING_H