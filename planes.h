#ifndef PLANES_H
#define PLANES_H

#include <exec/types.h>
#include "blitter.h"

#define PLANE_BPL1 "objects/fv_plane1.BPL"
#define PLANE_BPL2 "objects/fv_plane2.BPL"

#define NUM_PLANES          2
#define PLANE_BOB_WIDTH    16
#define PLANE_BOB_HEIGHT   16
#define PLANE_BOB_DEPTH     4

void Planes_Initialize(void);
void Planes_Start(void);
void Planes_Stop(void);
void Planes_Update(void);
void Planes_Draw(UBYTE *buffer);
void Planes_Restore(UBYTE *buffer);
void Planes_ResetDone(void);
BOOL Planes_IsActive(void);

#endif