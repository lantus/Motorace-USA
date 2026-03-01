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
static GameTimer flash_timer;

#define TOTAL_RANKING_TIERS 18
#define VISIBLE_RANKINGS 7

static const RankingTier all_rankings[TOTAL_RANKING_TIERS] = {
    {"1", 20000, 1, 1},
    {"2", 14000, 2, 2},
    {"3", 12000, 3, 3},
    {"4", 10000, 4, 4},
    {"5", 8000, 5, 5},
    {"6", 6000, 6, 6},
    {"7\x4010", 3000, 7, 10},    
    {"11\x4015", 2400, 11, 15},
    {"16\x4020", 2200, 16, 20},
    {"21\x4030", 2000, 21, 30},
    {"31\x4040", 1800, 31, 40},
    {"41\x4050", 1600, 41, 50},
    {"51\x4060", 1400, 51, 60},
    {"61\x4070", 1200, 61, 70},
    {"71\x4080", 1000, 71, 80},
    {"81\x4090", 800, 81, 90},
    {"91\x4098", 600, 91, 98},
    {"99\x40", 0, 99, 99}
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
    current_ranking.scroll_offset = 0;
    current_ranking.player_tier_index = 0;
    current_ranking.flash_state = FALSE;
    
    Timer_Start(&flash_timer, 0);  // Flash every 0.5 seconds

    // Clear screens
    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

    // Calculate ranking data based on gameplay
    UBYTE player_rank = 68;  // Calculate based on time/performance
    UWORD points_earned = 0; // Points based on rank tier
    UWORD avg_speed = 205;   // Track during gameplay
    UBYTE best_rank = 68;    // Load from saved records
    BOOL new_record = TRUE;  // Check if beat previous best

    //  Set ranking data
    Ranking_SetData(
        1,              // Checkpoint number (1 = LA→LV)
        "LAS VEGAS",    // City name
        player_rank,    // Player's rank
        points_earned,  // Points earned
        avg_speed,      // Average speed
        best_rank,      // Today's best
        new_record      // New record flag
    );
      
}

void Ranking_SetData(UBYTE checkpoint, const char *city, UBYTE rank, UWORD points,
                     UWORD avg_speed, UBYTE best_rank, BOOL record)
{
    current_ranking.checkpoint_number = checkpoint;
    
    // Copy city name
    int i;
    for (i = 0; i < 15 && city[i] != '\0'; i++)
    {
        current_ranking.city_name[i] = city[i];
    }
    current_ranking.city_name[i] = '\0';
    
    current_ranking.player_rank = rank;
    current_ranking.average_speed = avg_speed;
    current_ranking.todays_best_rank = best_rank;
    current_ranking.new_record = record;
    
    // Find which tier player is in
    for (UBYTE i = 0; i < TOTAL_RANKING_TIERS; i++)
    {
        if (rank >= all_rankings[i].min_rank && rank <= all_rankings[i].max_rank)
        {
            current_ranking.player_tier_index = i;
            current_ranking.player_points = all_rankings[i].points;
            break;
        }
    }
    
    // Start scroll at top
    current_ranking.scroll_offset = 0;
    current_ranking.scrolling_complete = FALSE;
    
    // Calculate target scroll position (player as 3rd item)
    current_ranking.target_scroll_offset = 0;
    
    if (current_ranking.player_tier_index >= 2)
    {
        current_ranking.target_scroll_offset = current_ranking.player_tier_index - 2;
        
        // Don't scroll past the end
        if (current_ranking.target_scroll_offset + VISIBLE_RANKINGS > TOTAL_RANKING_TIERS)
        {
            current_ranking.target_scroll_offset = TOTAL_RANKING_TIERS - VISIBLE_RANKINGS;
        }
    }
    
    Timer_Start(&flash_timer, 0);
}

void Ranking_Update(void)
{
    static UWORD scroll_counter = 0;
    
    if (!current_ranking.scrolling_complete)
    {
        if (current_ranking.scroll_offset < current_ranking.target_scroll_offset)
        {
            current_ranking.scroll_offset++;
        }
        else
        {
            current_ranking.scrolling_complete = TRUE;
            KPrintF("Ranking scroll complete at offset %d\n", current_ranking.scroll_offset);
        }
    }
    
    // Handle flashing (only after scroll completes)
    if (current_ranking.scrolling_complete)
    {
        static UWORD flash_counter = 0;
        flash_counter++;
        
        if (flash_counter >= 30)  // Flash at 2Hz
        {
            flash_counter = 0;
            current_ranking.flash_state = !current_ranking.flash_state;
        }
    }
}

void Ranking_Draw(UBYTE *buffer)
{
    char line_buffer[32];
    UWORD y;

    // Draw checkpoint title
    y = 8;
    Font_DrawStringCentered(buffer, "CHECKPOINT #", y, 15);

    ULongToString(current_ranking.checkpoint_number, line_buffer, 2, ' ');
    Font_DrawString(buffer, line_buffer, 136, y, 15);

    // Draw city name
    y = 20;
    Font_DrawStringCentered(buffer, current_ranking.city_name, y, 12);

    // Draw headers
    y = 80;
    Font_DrawString(buffer, "RANK", 20, y, 12);
    Font_DrawString(buffer, "POINTS", 120, y, 12);
 
    // Draw 7 visible ranking tiers
    y = 100;
    for (UBYTE i = 0; i < VISIBLE_RANKINGS; i++)
    {
        UBYTE tier_index = current_ranking.scroll_offset + i;
        if (tier_index >= TOTAL_RANKING_TIERS) break;
        
        const RankingTier *tier = &all_rankings[tier_index];
        
        // Determine color - flash red if this is player's tier
        UBYTE color = 15;  // White
        if (tier_index == current_ranking.player_tier_index && current_ranking.flash_state)
        {
            color = 12;  // Red (flash)
        }
        
        // Format line: "RANK-------POINTS PTS" (19 chars total)
        // Example: "7 ~ 10------3000 PTS"
        char formatted_line[20];
        UBYTE pos = 0;

        // Build points string
        char points_str[10];
        ULongToString(tier->points, points_str, 4, ' ');

        // Calculate dash count
        UBYTE rank_len = strlen(tier->rank_text);
        UBYTE points_len = strlen(points_str) + 4;  // " PTS"
        UBYTE dash_count = 19 - rank_len - points_len;
 
        // Copy rank text
        for (UBYTE r = 0; r < rank_len; r++)
        {
            formatted_line[pos++] = tier->rank_text[r];
        }

        // Add dashes
        for (UBYTE d = 0; d < dash_count; d++)
        {
            formatted_line[pos++] = DASH_PIECE[0];  // First char of DASH_PIECE
        }

        // Add points
        for (UBYTE p = 0; points_str[p] != '\0'; p++)
        {
            formatted_line[pos++] = points_str[p];
        }

        // Add " PTS"
        formatted_line[pos++] = ' ';
        formatted_line[pos++] = 'P';
        formatted_line[pos++] = 'T';
        formatted_line[pos++] = 'S';
        formatted_line[pos] = '\0';

        //Font_RestoreFromPristine(buffer, 20, y, 192, 8);
        Font_DrawString(buffer, formatted_line, 20, y, color);

        y += 16;
    }

    /* 
    // Draw average speed
    y = 200;
    Font_DrawString(buffer, "AVERAGE SPEED", 20, y, 15);
    ULongToString(current_ranking.average_speed, line_buffer, 5, ' ');
    Font_DrawString(buffer, line_buffer, 128, y, 15);
    Font_DrawString(buffer, "Km/h", 168, y, 15);

    // Draw today's best
    y = 220;
    Font_DrawString(buffer, "TODAY'S BEST", 20, y, 15);
    ULongToString(current_ranking.todays_best_rank, line_buffer, 3, ' ');
    Font_DrawString(buffer, line_buffer, 128, y, 15);
    Font_DrawString(buffer, "th", 152, y, 15);

    if (current_ranking.new_record)
    {
        y = 240;
        Font_DrawStringCentered(buffer, "YOU SET A NEW RECORD", y, 12);
    } */
}

RankingData* Ranking_GetData(void)
{
    return &current_ranking;
}


void Ranking_DrawCityBackdrop(WORD y_offset,UBYTE *buffer)
{
    city_horizon->y = y_offset;

    WORD x = city_horizon->x;
    WORD y = y_offset;

    UWORD source_mod = 0; 
    UWORD dest_mod = (SCREENWIDTH - CITYSKYLINE_WIDTH) / 8;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
 
    UWORD bltsize = ((CITYSKYLINE_HEIGHT << 2) << 6) | CITYSKYLINE_WIDTH / 16;
    
    UBYTE *source = (UBYTE*)&city_horizon->data[0];
   
    //   Only blit to the specified buffer (not all 3 buffers)
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, buffer);
}