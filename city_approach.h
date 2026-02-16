
#ifndef CITY_APPROACH_H
#define CITY_APPROACH_H

#include <exec/types.h>
 
#include "timers.h"
#include "blitter.h"

#define NYC_SKYLINE "images/skyline_nyc.BPL"   // demo and nyc are the same
#define STL_SKYLINE "images/skyline_stl.BPL"
#define HOUSTON_SKYLINE "images/skyline_hou.BPL"
#define VEGAS_SKYLINE "images/skyline_lv.BPL"
#define VEGAS_SKYLINE2 "images/skyline_lv2.BPL"
 
#define LA_SKYLINE "images/skyline_la.BPL"

#define ONCOMING_CAR_1 "objects/frontview/fv1.BPL"
#define ONCOMING_CAR_2 "objects/frontview/fv2.BPL"
#define ONCOMING_CAR_3 "objects/frontview/fv3.BPL"
#define ONCOMING_CAR_4 "objects/frontview/fv4.BPL"
#define ONCOMING_CAR_5 "objects/frontview/fv5.BPL"
#define ONCOMING_CAR_6 "objects/frontview/fv6.BPL"
#define ONCOMING_CAR_7 "objects/frontview/fv7.BPL"
#define ONCOMING_CAR_8 "objects/frontview/fv8.BPL"
#define ONCOMING_CAR_5_RIGHT "objects/frontview/fv5_r.BPL"
#define ONCOMING_CAR_6_RIGHT "objects/frontview/fv6_r.BPL"
#define ONCOMING_CAR_7_RIGHT "objects/frontview/fv7_r.BPL"
#define ONCOMING_CAR_8_RIGHT "objects/frontview/fv8_r.BPL"


// Horizon vanishing point (center of road at top)
#define HORIZON_VANISHING_X 72   // Center of 192-pixel screen
#define HORIZON_Y 67             // Where cars spawn (scale_start_y[0])

#define CITYSKYLINE_WIDTH   192
#define CITYSKYLINE_HEIGHT   56
 
// Oncoming car scales (8 frames from small to large)

#define ONCOMING_CAR_WIDTH 48
#define ONCOMING_CAR_HEIGHT 32
#define ONCOMING_CAR_WIDTH_WORDS 4           

#define NUM_CAR_SCALES 8
#define TOTAL_CARS_TO_PASS 8
#define CAR_SPAWN_TIMER 1 // 1 sec

// City approach states
typedef enum {
    CITY_STATE_WAITING_NAME,       // Showing city name
    CITY_STATE_ACTIVE,             // Cars spawning/passing
    CITY_STATE_INTO_HORIZON,       // All cars passed, bike moving into horizon
    CITY_STATE_COMPLETE            // Horizon transition done
} CityApproachState;

extern BlitterObject *city_horizon;
extern BlitterObject oncoming_car[8];
 
extern UBYTE *car_scale_data[NUM_CAR_SCALES];
extern WORD car_scale_widths[NUM_CAR_SCALES];
extern WORD car_scale_heights[NUM_CAR_SCALES];

extern BOOL frontview_bike_crashed;
extern BOOL use_alt_frame; // skyline alternative anim

// City/frontview functions
void City_Initialize(void);
void City_OncomingCarsReset(void);
void City_SpawnOncomingCar(void);
void City_DrawOncomingCars(void);
void City_DrawOncomingCar(BlitterObject *car);
void City_RestoreOncomingCars(void);
void City_CopyPristineBackground(BlitterObject *car);
BOOL City_OncomingCarsIsComplete(void);
BOOL City_IsCityNameComplete(void);
void City_ShowCityName(const char *city_name);
void City_BlitHorizon(void);
void City_DrawRoad(void);
void City_PreDrawRoad(void);
void City_UpdateHorizonTransition(WORD *bike_y, WORD *bike_speed, UWORD frame_count);
WORD City_CalculatePerspectiveX(WORD scale_index, WORD target_x);
WORD City_CalculateBikePerspectiveX(WORD bike_y, WORD starting_x);
WORD City_GetBikeHorizonStartX(void);
BOOL City_CheckCarCollision(BlitterObject *car);
void City_UpdateBikeCrashAnimation(void);
CityApproachState City_GetApproachState(void);
#endif