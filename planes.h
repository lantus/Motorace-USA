#ifndef PLANES_H
#define PLANES_H

#include <exec/types.h>
#include "blitter.h"

#define PLANE_BPL1           "objects/fv_plane1.BPL"
#define PLANE_BPL2           "objects/fv_plane2.BPL"
#define PLANE_REV_BPL1       "objects/fv_plane1_rev.BPL"
#define PLANE_REV_BPL2       "objects/fv_plane2_rev.BPL"
#define SMOKE_BPL            "objects/smoke.BPL"

#define NUM_PLANES          2
#define PLANE_BOB_WIDTH    16
#define PLANE_BOB_HEIGHT   16
#define PLANE_BOB_DEPTH     4

#define MAX_SMOKE_PUFFS     12
#define SMOKE_BOB_WIDTH     16
#define SMOKE_BOB_HEIGHT    16
#define SMOKE_BOB_DEPTH     4
#define SMOKE_START_X    48
#define SMOKE_END_X      128
#define SMOKE_Y          16

#define PLANE_MODE_HOUSTON  0   
#define PLANE_MODE_NYC      1   


void Planes_Initialize(void);
void Planes_Start(void);
void Planes_StartNYC(void);
void Planes_Stop(void);
void Planes_Update(void);
void Planes_Draw(UBYTE *buffer);
void Planes_Restore(UBYTE *buffer);
void Planes_ResetDone(void);
BOOL Planes_IsActive(void);
BOOL Planes_IsComplete(void);

#endif