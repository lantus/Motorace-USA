#ifndef BARRELTRUCK_H
#define BARRELTRUCK_H

#include <exec/types.h>
#include "blitter.h"
 
/* ---- Truck BOB ---- */
#define TRUCK_BOB_FILE     "objects/barreltruck.BPL"
#define TRUCK_BOB_WIDTH     32
#define TRUCK_BOB_HEIGHT    56
#define TRUCK_BOB_DEPTH     4

/* ---- barrel strip (14×20, overhangs truck — 2 anim frames) ---- */
#define LEFT_STRIP_FILE1    "objects/leftbarrel1.BPL"
#define LEFT_STRIP_FILE2    "objects/leftbarrel2.BPL"
#define LEFT_STRIP_WIDTH    16       /* 14 padded to 16 for word alignment */
#define LEFT_STRIP_HEIGHT   20
#define RIGHT_STRIP_WIDTH   16
#define RIGHT_STRIP_HEIGHT  20

/* ---- Dropped barrel (16×16) ---- */
#define BARREL_BOB_FILE1     "objects/barrel1.BPL"
#define BARREL_BOB_FILE2     "objects/barrel2.BPL"
#define BARREL_BOB_WIDTH    16
#define BARREL_BOB_HEIGHT   12
#define BARREL_BOB_DEPTH    4

/* ---- Strip placement on truck bed (offsets from truck_x/truck_y) ---- */
#define LEFT_STRIP_OFFSET_X     2
#define LEFT_STRIP_OFFSET_Y     30
#define RIGHT_STRIP_OFFSET_X    14
#define RIGHT_STRIP_OFFSET_Y    30

/* ---- Limits ---- */
#define MAX_BARRELS_PER_SIDE    2
#define MAX_BARRELS             4

/* ---- Sides ---- */
#define BARREL_SIDE_LEFT        0
#define BARREL_SIDE_RIGHT       1

extern WORD barrel_truck_speed;

typedef struct {
    WORD  x, y;                     /* world coordinates */
    WORD  old_x, old_y;
    WORD  prev_old_x, prev_old_y;
    WORD  accumulator;              /* 8.8 fixed for slow scroll */
    UBYTE side;                     /* BARREL_SIDE_LEFT or BARREL_SIDE_RIGHT */
    UBYTE needs_restore;
    BOOL  active;
    WORD  base_x;           
    UBYTE wobble_phase;     
    UBYTE wobble_counter;
} Barrel;

void BarrelTruck_Initialize(void);
void BarrelTruck_Spawn(WORD x, WORD y);
void BarrelTruck_Stop(void);
void BarrelTruck_Update(void);
void BarrelTruck_Draw(void);
void BarrelTruck_Reset(void);
BOOL BarrelTruck_IsActive(void);
BOOL BarrelTruck_CheckCollision(WORD bike_cx, WORD bike_top);
void BarrelTruck_RequestSpawn();
#endif