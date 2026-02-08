
#ifndef CITY_APPROACH_H
#define CITY_APPROACH_H

#include <exec/types.h>
 
#include "timers.h"
#include "blitter.h"

#define NYC_SKYLINE "images/skyline_nyc.BPL"   // demo and nyc are the same
#define STL_SKYLINE "images/skyline_stl.BPL"
#define HOUSTON_SKYLINE "images/skyline_hou.BPL"
#define VEGAS_SKYLINE "images/skyline_lv.BPL"
#define VEGAS_SKYLINE_ANIM1 "images/lv_anim1.BPL"
#define VEGAS_SKYLINE_ANIM2 "images/lv_anim2.BPL"
#define LA_SKYLINE "images/skyline_la.BPL"

#define ONCOMING_CAR_1 "objects/frontview/fv1.BPL"
#define ONCOMING_CAR_2 "objects/frontview/fv2.BPL"
#define ONCOMING_CAR_3 "objects/frontview/fv3.BPL"
#define ONCOMING_CAR_4 "objects/frontview/fv4.BPL"
#define ONCOMING_CAR_5 "objects/frontview/fv5.BPL"
#define ONCOMING_CAR_6 "objects/frontview/fv6.BPL"
#define ONCOMING_CAR_7 "objects/frontview/fv7.BPL"
#define ONCOMING_CAR_8 "objects/frontview/fv8.BPL"
 

#define CITYSKYLINE_WIDTH   192
#define CITYSKYLINE_HEIGHT   56
 
// Oncoming car scales (8 frames from small to large)

#define ONCOMING_CAR_WIDTH 48
#define ONCOMING_CAR_HEIGHT 32
#define ONCOMING_CAR_WIDTH_WORDS 4            // (includes padding)

#define NUM_CAR_SCALES 8
#define TOTAL_CARS_TO_PASS 32

extern BlitterObject *city_horizon;
extern BlitterObject oncoming_car[8];
 
extern UBYTE *car_scale_data[NUM_CAR_SCALES];
extern WORD car_scale_widths[NUM_CAR_SCALES];
extern WORD car_scale_heights[NUM_CAR_SCALES];

void City_Initialize(void);
void City_OncomingCarsReset(void);
void City_SpawnOncomingCar(void);
void City_DrawOncomingCars(void);
void City_DrawOncomingCar(BlitterObject *car);
void City_RestoreOncomingCars(void);
void City_CopyPristineBackground(BlitterObject *car);
BOOL City_OncomingCarsIsComplete(void);

// City drawing functions
void City_BlitHorizon(void);
void City_DrawRoad(void);
void City_PreDrawRoad(void);
#endif