#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include "game.h"
#include "map.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
 
volatile struct Custom *custom;

enum
{
	REG = 0,
	VALUE = 1
};

// Global variables
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
 
struct GfxBase *GfxBase = NULL;
struct ViewPort *viewPort = NULL;
struct RasInfo rasInfo;
 
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
static volatile APTR VBR = 0;
static APTR SystemIrq;
struct View *ActiView;
 
/* write definitions for dmaconw */
#define DMAF_SETCLR  0x8000
#define DMAF_AUDIO   0x000F   /* 4 bit mask */
#define DMAF_AUD0    0x0001
#define DMAF_AUD1    0x0002
#define DMAF_AUD2    0x0004
#define DMAF_AUD3    0x0008
#define DMAF_DISK    0x0010
#define DMAF_SPRITE  0x0020
#define DMAF_BLITTER 0x0040
#define DMAF_COPPER  0x0080
#define DMAF_RASTER  0x0100
#define DMAF_MASTER  0x0200
#define DMAF_BLITHOG 0x0400
#define DMAF_ALL     0x01FF   /* all dma channels */

#define BPLCON3_BRDNBLNK (1<<5)
 
#define SCREENWIDTH  320
#define SCREENHEIGHT 256
#define EXTRAHEIGHT 32
#define SCREENBYTESPERROW (SCREENWIDTH / 8)

#define BITMAPWIDTH SCREENWIDTH
#define BITMAPBYTESPERROW (BITMAPWIDTH / 8)
#define BITMAPHEIGHT ((SCREENHEIGHT + EXTRAHEIGHT) * 2)
#define HALFBITMAPHEIGHT (BITMAPHEIGHT / 2)

#define BLOCKSWIDTH 64
#define BLOCKSHEIGHT 104
#define BLOCKSDEPTH 5
#define BLOCKSCOLORS (1L << BLOCKSDEPTH)
#define BLOCKWIDTH 8
#define BLOCKHEIGHT 8
#define BLOCKSBYTESPERROW (BLOCKSWIDTH / 8)
#define BLOCKSPERROW (BLOCKSWIDTH / BLOCKWIDTH)

#define NUMSTEPS BLOCKHEIGHT

#define BITMAPBLOCKSPERROW (BITMAPWIDTH / BLOCKWIDTH)
#define BITMAPBLOCKSPERCOL (BITMAPHEIGHT / BLOCKHEIGHT)
#define HALFBITMAPBLOCKSPERCOL (BITMAPBLOCKSPERCOL / 2)

#define VISIBLEBLOCKSX (SCREENWIDTH / BLOCKWIDTH)
#define VISIBLEBLOCKSY (SCREENHEIGHT / BLOCKHEIGHT)

#define BITMAPPLANELINES (BITMAPHEIGHT * BLOCKSDEPTH)
#define BLOCKPLANELINES  (BLOCKHEIGHT * BLOCKSDEPTH)
 
#define PALSIZE (BLOCKSCOLORS * 2)
#define BLOCKSFILESIZE (BLOCKSWIDTH * BLOCKSHEIGHT * BLOCKSPLANES / 8 + PALSIZE)

#define TWOBLOCKS (BITMAPBLOCKSPERROW - NUMSTEPS)
#define TWOBLOCKSTEP (NUMSTEPS - TWOBLOCKS)

const UBYTE* planes[BLOCKSDEPTH];

struct BitMapEx *BlocksBitmap,*ScreenBitmap;
struct RawMap *Map;
UBYTE	 *frontbuffer,*blocksbuffer;

WORD	mapposy,videoposy;
WORD	bitmapheight;

LONG	mapwidth,mapheight;
UBYTE 	*mapdata;
LONG MapSize = 0;
UWORD	colors[BLOCKSCOLORS];
BOOL	option_ntsc,option_how,option_speed;
WORD	option_fetchmode;

BPTR	MapHandle = 0;
char	s[256];
struct RawMap *Map;
struct FetchInfo
{
	WORD	start;
	WORD	stop;
	WORD	modulooffset;
} fetchinfo [] =
{
	{0x38,0xD0,0},	/* normal         */
	{0x38,0xC8,0}, 	/* BPL32          */
	{0x38,0xC8,0}, 	/* BPAGEM         */
	{0x38,0xB8,0}	/* BPL32 + BPAGEM */
};

/********************* MACROS ***********************/

#define ROUND2BLOCKWIDTH(x)  ((x) & ~(BLOCKWIDTH - 1))
#define ROUND2BLOCKHEIGHT(x) ((x) & ~(BLOCKHEIGHT - 1))

#define ADJUSTDESTCOORDS(x,y) 		\
	if (x >= BITMAPWIDTH)				\
	{											\
		x -= BITMAPWIDTH;					\
		y++;									\
		if (y >= BITMAPPLANELINES)		\
		{										\
			y -= BITMAPPLANELINES;		\
		}										\
	}



// Global variables
 
struct Interrupt vblankInt;
BOOL running = TRUE;
 
__attribute__((always_inline)) inline short MouseLeft() { return !((*(volatile UBYTE*)0xbfe001) & 64); }
__attribute__((always_inline)) inline short MouseRight() { return !((*(volatile UWORD*)0xdff016) & (1 << 10)); }
 

static void InitCopperlist(void)
{
	WORD	*wp;
	LONG l;

	WaitVBL();

	custom->dmacon = 0x7FFF;
	custom->beamcon0 = 0;

	// plane pointers
 
	for(int i=0; i < 16; i++)
	{
		Copper_SetPalette(i, ((USHORT*)colors)[i]);
	}

	CopFETCHMODE[VALUE] = 0;

	CopBPLCON0[VALUE] = 0x4200;				// 32 Colors
	CopBPLCON1[VALUE] = 0;
	CopBPLCON3[VALUE] = (1<<6);

	// bitplane modulos

	l = BITMAPBYTESPERROW * BLOCKSDEPTH -
		 SCREENBYTESPERROW - fetchinfo[0].modulooffset;

	CopBPLMODA[VALUE] = l;
	CopBPLMODB[VALUE] = l;
	
	// display window start/stop
	
	CopDIWSTART[VALUE] = DIWSTARTVAL;
	CopDIWSTOP[VALUE] = DIWSTOPVAL;
	
	// display data fetch start/stop
	
	CopDDFSTART[VALUE]= fetchinfo[0].start;
	CopDDFSTOP[VALUE] = fetchinfo[0].stop;
	
	// plane pointers

	const USHORT lineSize=320/8;
	 
	for(int a=0;a<BLOCKSDEPTH;a++)
		planes[a]=(UBYTE*)frontbuffer + lineSize * a;

	Copper_SetBitplanePointer(BLOCKSDEPTH,planes , 0);
 
	custom->intena = 0x7FFF;
	custom->dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_COPPER | DMAF_RASTER | DMAF_MASTER | DMAF_SPRITE;
	custom->cop2lc = (ULONG)CopperList;	
};

 
void Cleanup(char *msg)
{
	WORD rc;
	
	if (msg)
	{
		KPrintF("Error: %s\n",msg);
		rc = RETURN_WARN;
	} 
	else 
	{
		rc = RETURN_OK;
	}
	 
	if (ScreenBitmap)
	{
		WaitBlit();
		BitMapEx_Destroy(ScreenBitmap);
	}

	if (BlocksBitmap)
	{
		WaitBlit();
		BitMapEx_Destroy(BlocksBitmap);
	}

	if (Map) FreeMem(Map,MapSize);
	if (MapHandle) Close(MapHandle);

	
	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);
	 
	Exit(rc);
}

static void OpenDisplay(void)
{
	bitmapheight = BITMAPHEIGHT + 3;

	ScreenBitmap = BitMapEx_Create(BLOCKSDEPTH,BITMAPWIDTH,bitmapheight);
	frontbuffer = ScreenBitmap->planes[0];
}

void WaitLine(USHORT line) 
{
	while (1) 
    {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}
}

static APTR GetVBR(void) 
{
    APTR vbr = 0;
    UWORD getvbr[] = { 0x4e7a, 0x0801, 0x4e73 }; // MOVEC.L VBR,D0 RTE
    if (SysBase->AttnFlags & AFF_68010)
        vbr = (APTR)Supervisor((ULONG (*)())getvbr);
    return vbr;
}

void SetInterruptHandler(APTR interrupt) 
{
    *(volatile APTR*)(((UBYTE*)VBR) + 0x6c) = interrupt;
}

APTR GetInterruptHandler() {
    return *(volatile APTR*)(((UBYTE*)VBR) + 0x6c);
}

void WaitVbl() 
{
    while (1) {
        volatile ULONG vpos = *(volatile ULONG*)0xdff004;
        vpos &= 0x1ff00;
        if (vpos != (311 << 8)) break;
    }
    while (1) {
        volatile ULONG vpos = *(volatile ULONG*)0xdff004;
        vpos &= 0x1ff00;
        if (vpos == (311 << 8)) break;
    }
}

static void OpenMap(void)
{
	/* 
	
	if (!(MapHandle = Open(MAPNAME,MODE_OLDFILE)))
	{
		Cleanup("Find Not Found");
	}
	
	Seek(MapHandle,0,OFFSET_END);
	MapSize = Seek(MapHandle,0,OFFSET_BEGINNING);

	if (!(Map = AllocMem(MapSize,MEMF_PUBLIC)))
	{
		Cleanup("Out of memory!");
	}
	
	if (Read(MapHandle,Map,MapSize) != MapSize)
	{
	 
		Cleanup(s);
	}
	
	Close(MapHandle);MapHandle = 0;
	
	mapdata = Map->data;
	mapwidth = Map->mapwidth;
	mapheight = Map->mapheight; */
}
 
static void OpenBlocks(void)
{
	/* 
	LONG l = 0;

	BlocksBitmap = BitMapEx_Create(BLOCKSDEPTH, BLOCKSWIDTH, BLOCKSHEIGHT);

	if (!(BlocksBitmap))
	{	 
		Cleanup("Can't alloc blocks bitmap!");
	}
	
	if (!(MapHandle = Open(BLOCKSNAME,MODE_OLDFILE)))
	{
		Cleanup(s);
	}
	
	l = BLOCKSWIDTH * BLOCKSHEIGHT * BLOCKSDEPTH / 8;
	
	if (Read(MapHandle,BlocksBitmap->planes[0],l) != l)
	{
	 
		Cleanup(s);
	}
	
	Close(MapHandle);MapHandle = 0;


	if (!(MapHandle = Open(PALETTENAME,MODE_OLDFILE)))
	{
		Cleanup(s);
	}

	if (Read(MapHandle,colors,PALSIZE) != PALSIZE)
	{
		Cleanup(s);
	}

	Close(MapHandle);MapHandle = 0;
	
	blocksbuffer = BlocksBitmap->planes[0];

	debug_register_bitmap(blocksbuffer, "blocksbuffer", 64, 104, 4, debug_resource_bitmap_interleaved);
	debug_register_palette(colors,"palette",16,0); */

}


 
void TakeSystem() 
{
	Forbid();
	//Save current interrupts and DMA settings so we can restore them upon exit. 
	SystemADKCON=custom->adkconr;
	SystemInts=custom->intenar;
	SystemDMA=custom->dmaconr;
	ActiView=GfxBase->ActiView; //store current view

	LoadView(0);
	WaitTOF();
	WaitTOF();

	WaitVbl();
	WaitVbl();

	OwnBlitter();
	WaitBlit();	
	Disable();
	
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	
	custom->dmacon=0x7fff;//Clear all DMA channels

	//set all colors black
	for(int a=0;a<32;a++)
		custom->color[a]=0;

	WaitVbl();
	WaitVbl();

	VBR=GetVBR();
	SystemIrq=GetInterruptHandler(); //store interrupt register
}

void FreeSystem()
{ 
	WaitVbl();
	WaitBlit();
    
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	custom->dmacon=0x7fff;//Clear all DMA channels

	//restore interrupts
	SetInterruptHandler(SystemIrq);

	/*Restore system copper list(s). */
	custom->cop1lc=(ULONG)GfxBase->copinit;
	custom->cop2lc=(ULONG)GfxBase->LOFlist;
	custom->copjmp1=0x7fff; //start coppper

	/*Restore all interrupts and DMA settings. */
	custom->intena=SystemInts|0x8000;
	custom->dmacon=SystemDMA|0x8000;
	custom->adkcon=SystemADKCON|0x8000;

	WaitBlit();	
	DisownBlitter();
	Enable();

	LoadView(ActiView);
	WaitTOF();
	WaitTOF();

	Permit();
} 
 
static __attribute__((interrupt)) void InterruptHandler() 
{
	custom->intreq=(1<<INTB_VERTB);  
}

// Add these constants at the top with your other defines
#define GAMEAREA_WIDTH 64          // 24 blocks × 8 pixels
#define GAMEAREA_BLOCKS 28          // blocks across the game area
#define SCREEN_OFFSET_X ((SCREENWIDTH - GAMEAREA_WIDTH) / 2)  // Center the game area

__attribute__((always_inline)) inline static void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy)
{
    UBYTE block;
    ULONG srcptr, dstptr;
 
    block = mapdata[mapy * mapwidth + mapx];

    WORD block_x = (block % BLOCKSPERROW);  // Block column in tileset (0-7)
    WORD block_y = (block / BLOCKSPERROW);  // Block row in tileset

    // INTERLEAVED format: each line has (width/8) * depth bytes
    // For 64x104, 4 bitplanes: each line = 8 * 4 = 32 bytes
    // Each 8x8 block = 1 byte per bitplane = 4 consecutive bytes
    
    LONG src_line_bytes = (BLOCKSWIDTH / 8) * BLOCKSDEPTH;  // 32 bytes per line
    LONG src_x_offset = block_x * BLOCKSDEPTH;  // 4 bytes per block horizontally
    LONG src_y_offset = block_y * BLOCKHEIGHT * src_line_bytes;  // Block row * 8 lines * 32 bytes
    
    // Destination: 320 width, 4 bitplanes interleaved
    LONG dst_line_bytes = (BITMAPWIDTH / 8) * BLOCKSDEPTH;  // 160 bytes per line
    LONG dst_x_offset = (x / 8) * BLOCKSDEPTH;  // 4 bytes per 8-pixel position
    LONG dst_y_offset = (y / BLOCKSDEPTH) * dst_line_bytes;  // Line number * 160 bytes

    srcptr = (ULONG)(blocksbuffer + src_y_offset + src_x_offset);
    dstptr = (ULONG)(frontbuffer + dst_y_offset + dst_x_offset);

    HardWaitBlit();
    
    custom->bltcon0 = 0x09F0;  // Simple copy: D = A
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    
    // Modulo calculations
    custom->bltamod = src_line_bytes - BLOCKSDEPTH;  // 32 - 4 = 28
    custom->bltdmod = dst_line_bytes - BLOCKSDEPTH;  // 160 - 4 = 156
    
    custom->bltapt = (APTR)srcptr;
    custom->bltdpt = (APTR)dstptr;
    
    // Blit size: 8 lines, 4 bytes per line = 2 words per line
    custom->bltsize = (BLOCKHEIGHT << 6) | 2;
 
}
 
static void ScrollUp(void)
{
	WORD mapx,mapy,x,y;

	if (mapposy < 1) return;

	mapposy--;
	videoposy = mapposy % HALFBITMAPHEIGHT;

	mapx = mapposy & (NUMSTEPS - 1);
	mapy = mapposy / BLOCKHEIGHT;
	
	y = ROUND2BLOCKHEIGHT(videoposy) * BLOCKSDEPTH;

   if (mapx < TWOBLOCKSTEP)
   {
   	// blit only one block per half bitmap
   	
   	x = mapx * BLOCKWIDTH;
   	
   	DrawBlock(x,y,mapx,mapy);
   	DrawBlock(x,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx,mapy);
   	
   } else {
   	// blit two blocks per half bitmap
   	
   	mapx = TWOBLOCKSTEP + (mapx - TWOBLOCKSTEP) * 2;
   	x = mapx * BLOCKWIDTH;
   	
   	DrawBlock(x,y,mapx,mapy);
   	DrawBlock(x,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx,mapy);

   	DrawBlock(x + BLOCKWIDTH,y,mapx + 1,mapy);
   	DrawBlock(x + BLOCKWIDTH,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx + 1,mapy);
   	
   }
}

static void ScrollDown(void)
{
	WORD mapx,mapy,x,y;

	if (mapposy >= (mapheight * BLOCKHEIGHT - SCREENHEIGHT - BLOCKHEIGHT)) return;
	
	mapx = mapposy & (NUMSTEPS - 1);
	mapy = HALFBITMAPBLOCKSPERCOL + mapposy / BLOCKHEIGHT;
	
	y = ROUND2BLOCKHEIGHT(videoposy) * BLOCKSDEPTH;

   if (mapx < TWOBLOCKSTEP)
   {
   	// blit only one block per half bitmap
   	
   	x = mapx * BLOCKWIDTH;
   	
   	DrawBlock(x,y,mapx,mapy);
   	DrawBlock(x,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx,mapy);
   	
   } else {
   	// blit two blocks per half bitmap
   	
   	mapx = TWOBLOCKSTEP + (mapx - TWOBLOCKSTEP) * 2;
   	x = mapx * BLOCKWIDTH;
   	
   	DrawBlock(x,y,mapx,mapy);
   	DrawBlock(x,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx,mapy);

   	DrawBlock(x + BLOCKWIDTH,y,mapx + 1,mapy);
   	DrawBlock(x + BLOCKWIDTH,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx + 1,mapy);
   }

	mapposy++;
	videoposy = mapposy % HALFBITMAPHEIGHT;
	
}

static void FillScreen(void)
{
	/* WORD a, b, x, y;
    
   
    for (b = 0; b < HALFBITMAPBLOCKSPERCOL; b++)
    {
        for (a = 0; a < GAMEAREA_BLOCKS; a++)  
        {
            x = a * BLOCKWIDTH;  // This will go from 0 to 184 pixels
            y = b * BLOCKPLANELINES;
            DrawBlock(x, y, a, b);
            DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, b);
        }
    }*/
}
 
 
static void CheckJoyScroll(void)
{
	WORD i,count;
	
	if (JoyFire()) count = 4; else count = 1;

	if (JoyUp())
	{
		ScrollUp();
		ScrollUp();
		ScrollUp();
	 
	}
	
	if (JoyDown())
	{
		ScrollDown();
		ScrollDown();
		ScrollDown();
 
	}
}

__attribute__((always_inline)) inline static void UpdateCopperlist(void)
{
	LONG planeadd;
	
	planeadd = ((LONG)(videoposy + BLOCKHEIGHT)) * BITMAPBYTESPERROW * BLOCKSDEPTH;
	
	// set plane pointers
	 
	Copper_SetBitplanePointer(BLOCKSDEPTH, planes, planeadd);
 
}

 
int main(void)
{
	LONG x = 0;
    SysBase = *((struct ExecBase**)4UL);
	custom = (struct Custom*)0xdff000;
 
	// We will use the graphics library only to locate and restore the system copper list once we are through.

	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library",0);
	if (!GfxBase)
		Exit(0);

    // used for printing
	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase)
		Exit(0);
 
	option_how = FALSE;
   	option_speed = TRUE;
 	option_fetchmode = 0;
	
	WaitVbl();

    Delay(10);
	
	InitializeGame();

	//OpenMap();
	//OpenBlocks();
	OpenDisplay();

	//TakeSystem();
 
	KillSystem();	

	InitCopperlist();

	Copper_SetPalette(0, 0x003);

	FillScreen();

	HardWaitBlit();
	WaitVBL();

 
	custom->copjmp2 = 0;
 
	NewGame(0);
	while(!MouseLeft()) 
    {		 
		WaitBlit();
		
		WaitLine(200);

 		//WaitLine(1);
// 

		bike_state = BIKE_STATE_MOVING;

		if (JoyFire())
		{
			bike_state = BIKE_STATE_ACCELERATING;
		}

		if (JoyLeft())
		{
			bike_position_x-=2;
			bike_state = BIKE_STATE_LEFT;
		}

		if (JoyRight())
		{
			bike_position_x+=2;
			bike_state = BIKE_STATE_RIGHT;
		}

		UpdateMotorBikePosition(bike_position_x,bike_position_y,bike_state);
 
	
		//CheckJoyScroll();
		//UpdateCopperlist();
	}

    ActivateSystem();

	Cleanup("");

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);

    return 0;
}