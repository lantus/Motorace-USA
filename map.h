#ifndef _MAP_
#define _MAP_

typedef struct RawMap
{
	WORD	mapwidth;
	WORD	mapheight;
	WORD	maplayers;
	WORD	blockwidth;
	WORD	blockheight;
	BYTE	bytesperblock;
	BYTE	transparentblock;
	UBYTE	data[1];
} RawMap;

void MapPool_Initialize(void);
void MapPool_Load(UBYTE stage);

#endif

