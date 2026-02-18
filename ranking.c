#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <string.h>
#include "ranking.h"
#include "font.h"
#include "blitter.h"
#include "hud.h"
#include "city_approach.h"
#include "game.h"

static RankingData current_ranking;

// Standard ranking tiers (same for all checkpoints)
static const RankingTier ranking_tiers[3] = {
    {41, 50, 1600},  // 41st-50th = 1600 points
    {51, 60, 1400},  // 51st-60th = 1400 points
    {61, 70, 0}      // 61st-70th = 0 points
};

void Ranking_Initialize(void)
{
    // Clear ranking data
    current_ranking.checkpoint_number = 0;
    current_ranking.city_name[0] = '\0';
    current_ranking.player_rank = 99;
    current_ranking.player_points = 0;
    current_ranking.average_speed = 0;
    current_ranking.todays_best_rank = 99;
    current_ranking.new_record = FALSE;

    // Clear screens
    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
}

void Ranking_SetData(UBYTE checkpoint, const char *city, UBYTE rank, UWORD points,
                     UWORD avg_speed, UBYTE best_rank, BOOL record)
{
    current_ranking.checkpoint_number = checkpoint;
    
    // Copy city name (max 15 chars + null)
    int i;
    for (i = 0; i < 15 && city[i] != '\0'; i++)
    {
        current_ranking.city_name[i] = city[i];
    }
    current_ranking.city_name[i] = '\0';
    
    current_ranking.player_rank = rank;
    current_ranking.player_points = points;
    current_ranking.average_speed = avg_speed;
    current_ranking.todays_best_rank = best_rank;
    current_ranking.new_record = record;
}

void Ranking_Draw(UBYTE *buffer)
{
    char line_buffer[32];
    UWORD y;
    
    // Draw checkpoint title
    y = 20;
    Font_DrawStringCentered(buffer, "CHECKPOINT #", y, 15);
    
    // Draw checkpoint number
    ULongToString(current_ranking.checkpoint_number, line_buffer, 2, ' ');
    Font_DrawString(buffer, line_buffer, 136, y, 15);
    
    // Draw city name
    y = 40;
    Font_DrawStringCentered(buffer, current_ranking.city_name, y, 12);
    
    // Draw headers
    y = 100;
    Font_DrawString(buffer, "RANK", 40, y, 12);
    Font_DrawString(buffer, "POINTS", 120, y, 12);
    
    // Draw ranking tiers
    y = 120;
    for (int i = 0; i < 3; i++)
    {
        const RankingTier *tier = &ranking_tiers[i];
        
        // Determine color based on player's rank
        UBYTE color = 15;  // Default white
        if (current_ranking.player_rank >= tier->rank_min && 
            current_ranking.player_rank <= tier->rank_max)
        {
            color = 12;  // Red if player is in this tier
        }
        
        // Format: "41 ~ 50------1600 PTS"
        ULongToString(tier->rank_min, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, 20, y, color);
        
        Font_DrawString(buffer, " ~ ", 36, y, color);
        
        ULongToString(tier->rank_max, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, 52, y, color);
        
        Font_DrawString(buffer, "------", 68, y, color);
        
        ULongToString(tier->points, line_buffer, 4, ' ');
        Font_DrawString(buffer, line_buffer, 110, y, color);
        
        Font_DrawString(buffer, " PTS", 142, y, color);
        
        y += 20;
    }
    
    // Draw average speed
    y = 180;
    Font_DrawString(buffer, "AVERAGE SPEED", 20, y, 15);
    
    ULongToString(current_ranking.average_speed, line_buffer, 5, ' ');
    Font_DrawString(buffer, line_buffer, 128, y, 15);
    
    Font_DrawString(buffer, "Km/h", 168, y, 15);
    
    // Draw today's best
    y = 200;
    Font_DrawString(buffer, "TODAY'S BEST", 20, y, 15);
    
    ULongToString(current_ranking.todays_best_rank, line_buffer, 3, ' ');
    Font_DrawString(buffer, line_buffer, 128, y, 15);
    
    Font_DrawString(buffer, "th", 152, y, 15);
    
    // Draw new record message if applicable
    if (current_ranking.new_record)
    {
        y = 220;
        Font_DrawStringCentered(buffer, "YOU SET A NEW RECORD", y, 12);
    }
}

RankingData* Ranking_GetData(void)
{
    return &current_ranking;
}


void Ranking_DrawCityBackdrop(WORD y_offset)
{
    WORD x = city_horizon->x;
 
    city_horizon->y = y_offset;

    City_BlitHorizon();
}