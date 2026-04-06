#ifndef BARRELTRUCK_H
#define BARRELTRUCK_H

#include <exec/types.h>
#include "blitter.h"

/* ---- Feature flag — set to TRUE when BOB assets are ready ---- */
#define BARRELTRUCK_ENABLED  FALSE

/* ---- Truck BOB assets ---- */
#define TRUCK_BOB_FILE1     "objects/truck1.BPL"
#define TRUCK_BOB_FILE2     "objects/truck2.BPL"

/* ---- Dimensions ---- */
#define TRUCK_BOB_WIDTH     32
#define TRUCK_BOB_HEIGHT    48
#define TRUCK_BOB_DEPTH     4

/* ---- Barrel BOB assets ---- */
#define BARREL_BOB_FILE     "objects/barrel.BPL"
#define BARREL_BOB_WIDTH    16
#define BARREL_BOB_HEIGHT   16
#define BARREL_BOB_DEPTH    4

/* ---- Limits ---- */
#define MAX_BARRELS         4

extern WORD barrel_truck_speed;

/* ---- Barrel instance ---- */
typedef struct {
    WORD  x, y;
    WORD  old_x, old_y;
    WORD  prev_old_x, prev_old_y;
    UBYTE needs_restore;
    BOOL  active;
} Barrel;

/* ---- API ---- */
void BarrelTruck_Initialize(void);
void BarrelTruck_Spawn(WORD x, WORD y);
void BarrelTruck_Stop(void);
void BarrelTruck_Update(void);
void BarrelTruck_Restore(void);
void BarrelTruck_Draw(void);
BOOL BarrelTruck_IsActive(void);
void BarrelTruck_Reset(void);

#endif
