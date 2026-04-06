#ifndef _TILESHEET_
#define _TILESHEET_

#include <exec/types.h>


#define TILEPOOL_CITY_ATTRACT   0
#define TILEPOOL_LEVEL1         1
#define TILEPOOL_LEVEL2         2
#define TILEPOOL_LEVEL3         3
#define TILEPOOL_LEVEL4         4
#define TILEPOOL_LEVEL5         5
#define TILEPOOL_COUNT          6

void TilesheetPool_Initialize(void);
void TilesheetPool_Load(UBYTE sheet_id);

#endif