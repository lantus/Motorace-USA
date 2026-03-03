#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <string.h>
#include "ranking.h"
#include "font.h"
#include "blitter.h"
#include "hud.h"
#include "city_approach.h"
#include "fuel.h"
#include "game.h"

static RankingData current_ranking;
static GameTimer flash_timer;
static GameTimer bonus_timer;
static GameTimer scroll_timer;
static GameTimer horizon_timer;
static GameTimer horizon_anim_timer;

#define TOTAL_RANKING_TIERS 18
#define VISIBLE_RANKINGS 7
#define PADDING_TIERS 4

static const RankingTier all_rankings[TOTAL_RANKING_TIERS+PADDING_TIERS] = {
    {"1", 20000, 1, 1},
    {"2", 14000, 2, 2},
    {"3", 12000, 3, 3},
    {"4", 10000, 4, 4},
    {"5", 8000, 5, 5},
    {"6", 6000, 6, 6},
    {"7" TILDE_PIECE "10", 3000, 7, 10},    
    {"11" TILDE_PIECE "15", 2400, 11, 15},
    {"16" TILDE_PIECE "20", 2200, 16, 20},
    {"21" TILDE_PIECE "30", 2000, 21, 30},
    {"31" TILDE_PIECE "40", 1800, 31, 40},
    {"41" TILDE_PIECE "50", 1600, 41, 50},
    {"51" TILDE_PIECE "60", 1400, 51, 60},
    {"61" TILDE_PIECE "70", 1200, 61, 70},
    {"71" TILDE_PIECE "80", 1000, 71, 80},
    {"81" TILDE_PIECE "90", 800, 81, 90},
    {"91" TILDE_PIECE "98", 600, 91, 98},
    {"99" TILDE_PIECE "", 0, 99, 99}, 
    // Padding
    {"", 0, 100, 100},
    {"", 0, 101, 101},
    {"", 0, 102, 102},
    {"", 0, 103, 103}
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
    UBYTE player_rank = 81;  // Calculate based on time/performance
    UWORD points_earned = 0; // Points based on rank tier
    UWORD avg_speed = 205;   // Track during gameplay
    UBYTE best_rank = 2;    // Load from saved records
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
    
    current_ranking.bonus_remaining = current_ranking.player_points;
    current_ranking.backdrop_drawn_both = FALSE; 
    current_ranking.rankingstate = RANKING_STATE_DRAWHORIZON;

    Timer_StartMs(&scroll_timer, 250);
    Timer_StartMs(&horizon_timer, 120);
    Timer_StartMs(&horizon_anim_timer, 400);

    // Start scroll at top
    current_ranking.scroll_offset = 0;
    current_ranking.scrolling_complete = FALSE;
    
     if (current_ranking.player_tier_index < 2)
    {
        current_ranking.target_scroll_offset = 0;
        current_ranking.scrolling_complete = TRUE;  // No scroll needed
    }
    //  Rank 3 and everything below: scroll to appear as 3rd line
    else
    {
        // Position player as 3rd visible item (index 2)
        current_ranking.target_scroll_offset = current_ranking.player_tier_index - 2;
        
        // Cap at max scroll (includes padding for rank 99)
        UBYTE max_offset = (TOTAL_RANKING_TIERS + PADDING_TIERS) - VISIBLE_RANKINGS;
        if (current_ranking.target_scroll_offset > max_offset)
        {
            current_ranking.target_scroll_offset = max_offset;
        }
    }

    Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.bitplanes);
    Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.offscreen_bitplanes);
    
    Timer_Start(&flash_timer, 0);
}

void Ranking_Update(void)
{
    switch (current_ranking.rankingstate)
    {
        case RANKING_STATE_DRAWHORIZON:
        {
            if (Timer_HasElapsed(&horizon_timer))
            {
                Timer_Reset(&horizon_timer);
                if (current_ranking.backdrop_y < 30)
                {
                    // Draw backdrop to BOTH buffers
                    if (!current_ranking.backdrop_drawn_both)
                    {
                        Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.bitplanes);
                        Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.offscreen_bitplanes);
                    }
                    
                    current_ranking.backdrop_y += 2;
                }
                else
                {
                    current_ranking.backdrop_y = 30;

                    Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.bitplanes);
                    Ranking_DrawCityBackdrop(current_ranking.backdrop_y, screen.offscreen_bitplanes);

                    current_ranking.rankingstate = RANKING_STATE_SCROLLING;
                    current_ranking.backdrop_drawn_both = TRUE;

                    Timer_StartMs(&scroll_timer, 250);
                }
            }

            break;
        }
        case RANKING_STATE_SCROLLING:
        {
            if (Timer_HasElapsed(&scroll_timer))
            {
                Timer_Reset(&scroll_timer);
                
                if (current_ranking.scroll_offset < current_ranking.target_scroll_offset)
                {
                    current_ranking.scroll_offset++;
                }
                else
                {
                    // Scroll complete - start flashing
                    Timer_Stop(&scroll_timer);
                    current_ranking.rankingstate = RANKING_STATE_FLASHING;
                    Timer_Start(&flash_timer, 2); 
                    KPrintF("=== Ranking scroll complete - flashing for 2s ===\n");
                }
            }
            
            break;
        }
        
        case RANKING_STATE_FLASHING:
        {
            // Flash the line
            static UWORD flash_counter = 0;
            flash_counter++;
            
            if (flash_counter >= 1)
            {
                flash_counter = 0;
                current_ranking.flash_state = !current_ranking.flash_state;
            }
            
            // After 2 seconds, stop flashing
            if (Timer_HasElapsed(&flash_timer))
            {
                Timer_Stop(&flash_timer);
                current_ranking.flash_state = TRUE;  // Keep it red
                current_ranking.rankingstate = RANKING_STATE_SOLID_RED;
                Timer_Start(&bonus_timer, 0);  // Start bonus depletion immediately
              
            }
            break;
        }
        
        case RANKING_STATE_SOLID_RED:
        {
            // Line stays solid red (no flashing)
            current_ranking.flash_state = TRUE;
            
          
            current_ranking.rankingstate = RANKING_STATE_BONUS_DEPLETING;
            break;
        }
        
        case RANKING_STATE_BONUS_DEPLETING:
        {
            static UWORD bonus_counter = 0;
            bonus_counter++;
            
            // ✅ Deplete 100 points every 2 frames (30 times per second)
            if (bonus_counter >= 8 && current_ranking.bonus_remaining > 0)
            {
                bonus_counter = 0;
                UWORD amount = (current_ranking.bonus_remaining >= 100) ? 100 : current_ranking.bonus_remaining;
                current_ranking.bonus_remaining -= amount;
                game_score += amount;
            }
            
            // When bonus depleted, show final stats
            if (current_ranking.bonus_remaining == 0)
            {
                current_ranking.rankingstate = RANKING_STATE_COMPLETE;
                KPrintF("=== Bonus complete - showing final stats ===\n");
            }
            break;
        }
        
        case RANKING_STATE_COMPLETE:
            // Nothing to update - just display final stats
            break;
    }
}

void Ranking_Draw(UBYTE *buffer)
{
    char line_buffer[32];
    UWORD y;

    if (current_ranking.rankingstate == RANKING_STATE_DRAWHORIZON && 
        !current_ranking.backdrop_drawn_both)
    {
        Ranking_DrawCityBackdrop(current_ranking.backdrop_y, buffer);
        return;
    }

    if (current_ranking.rankingstate < RANKING_STATE_BONUS_DEPLETING)
    {
        // Draw checkpoint title
        y = 8;
        Font_DrawStringCentered(buffer, "CHECKPOINT #", y, 13);

        ULongToString(current_ranking.checkpoint_number, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, 136, y, 13);

        // Draw city name
        y = 16;
        Font_DrawStringCentered(buffer, current_ranking.city_name, y, 12);

        // Draw headers
        y = 96;
        Font_DrawString(buffer, "RANK", 20, y, 12);
        Font_DrawString(buffer, "POINTS", 130, y, 12);
    
        // Draw 7 visible ranking tiers
        y = 110;
        for (UBYTE i = 0; i < VISIBLE_RANKINGS; i++)
        {
            UBYTE tier_index = current_ranking.scroll_offset + i;
            if (tier_index >= TOTAL_RANKING_TIERS + PADDING_TIERS) break;
            
            const RankingTier *tier = &all_rankings[tier_index];
            
            if (tier_index >= TOTAL_RANKING_TIERS)
            {
                break;  // Hit padding, stop drawing
            }

            // Determine color - flash red if this is player's tier
            UBYTE color = 13;  // White
            if (tier_index == current_ranking.player_tier_index && current_ranking.flash_state)
            {
                color = 12;  // Red (flash)
            }
            
            char formatted_line[20];
            UBYTE pos = 0;

            // Build points string
            char points_str[10];
            ULongToString(tier->points, points_str, 5, ' ');

            // Calculate dash count
            UBYTE rank_len = strlen(tier->rank_text);
            UBYTE points_len = strlen(points_str);
    
            WORD points_x = 120;  // Fixed X position for points (15 chars * 8 = 120)
            WORD rank_end_x = 20 + (rank_len * 8);
            UBYTE dash_count = (points_x - rank_end_x) / 8;

            if (tier_index > 3)   
            {
                dash_count += 1;
            }
            if (tier_index > 14)  
            {
                dash_count += 1;
            }
            if (tier_index == 17)  
            {
                dash_count += 2;
            }

            WORD x = 20;

            // Draw rank text
            Font_DrawString(buffer, (char*)tier->rank_text, x, y, color);
            x += rank_len * 8;

            // Draw dashes up to fixed points position
            for (UBYTE d = 0; d < dash_count; d++)
            {
                Font_DrawString(buffer, DASH_PIECE, x, y, color);
                x += 8;
            }
    
            Font_DrawString(buffer, points_str, points_x, y, color);
            Font_DrawString(buffer, " PTS", points_x + (points_len * 8), y, color);

            y += 15;
        }
    }
    else if (current_ranking.rankingstate == RANKING_STATE_BONUS_DEPLETING)
    {
        if (current_ranking.bonus_remaining  > 0)
        {
            UWORD amount = (current_ranking.bonus_remaining >= 100) ? 100 : current_ranking.bonus_remaining;
            current_ranking.bonus_remaining -= amount;

            if (!Fuel_IsFull())
            {
                Fuel_AddPoints(amount);   
                Fuel_DrawAll();
            }
            else
            {
                HUD_UpdateScore(game_score); 
            }
        }

    }
    else if (current_ranking.rankingstate == RANKING_STATE_COMPLETE)
    {
        // Draw average speed
        y = 160;
        Font_DrawString(buffer, "AVERAGE SPEED", 24, y, 15);
       // ULongToString(current_ranking.average_speed, line_buffer, 5, ' ');
       // Font_DrawString(buffer, line_buffer, 128, y, 15);
        //Font_DrawString(buffer, "", 168, y, 15);

        // Draw today's best
        y = 180;
        Font_DrawString(buffer, "TODAYS BEST", 20, y, 15);
        ULongToString(current_ranking.todays_best_rank, line_buffer, 3, ' ');
        Font_DrawString(buffer, line_buffer, 128, y, 15);
        Font_DrawString(buffer, "th", 152, y, 15);

        if (current_ranking.new_record)
        {
            y = 200;
            Font_DrawStringCentered(buffer, "YOU SET A NEW RECORD", y, 12);
        }  
    }
 
}

RankingData* Ranking_GetData(void)
{
    return &current_ranking;
}

BOOL Ranking_IsComplete(void)
{
    return current_ranking.rankingstate == RANKING_STATE_COMPLETE;
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
    
    if (game_map == MAP_FRONTVIEW_LASVEGAS)
    {
        if (Timer_HasElapsed(&horizon_anim_timer))
        {
            use_alt_frame = !use_alt_frame;
            Timer_Reset(&horizon_anim_timer); 
        }
    }   

    UBYTE *source;

    if (use_alt_frame == TRUE)
    {
        source = (UBYTE*)&city_horizon->data_frame2[0];
    }
    else
    {
        source = (UBYTE*)&city_horizon->data[0];
    }
 
    //   Only blit to the specified buffer (not all 3 buffers)
    BlitBobSimple(SCREENWIDTH_WORDS, x, y, admod, bltsize, source, buffer);
}

RankingState Ranking_GetState(void)
{
    return current_ranking.rankingstate;
}