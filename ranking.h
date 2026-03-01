#ifndef RANKING_H
#define RANKING_H

#include <exec/types.h>

typedef struct {
    char rank_text[10];  // "1", "7~10", "41~50", etc.
    UWORD points;
    UBYTE min_rank;
    UBYTE max_rank;
} RankingTier;

typedef enum {
    RANKING_STATE_SCROLLING,      // Scrolling to position
    RANKING_STATE_FLASHING,       // Flash red for 2 seconds
    RANKING_STATE_SOLID_RED,      // Line stays red, prepare bonus
    RANKING_STATE_BONUS_DEPLETING, // Transfer points to fuel/score
    RANKING_STATE_COMPLETE        // Show final stats
} RankingState;

typedef struct {
    UBYTE checkpoint_number;
    char city_name[16];
    UBYTE player_rank;
    UWORD player_points;
    UWORD average_speed;
    UBYTE todays_best_rank;
    BOOL new_record;
    UBYTE scroll_offset;       
    UBYTE target_scroll_offset; 
    UBYTE player_tier_index;  
    BOOL flash_state;         
    BOOL scrolling_complete; 
    UWORD bonus_remaining;
    RankingState rankingstate;   
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
void Ranking_Update(void); 
void Ranking_DrawCityBackdrop(WORD y_offset,UBYTE *buffer);

BOOL Ranking_IsComplete(void);

#endif // RANKING_H