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
 



const UBYTE* planes[BLOCKSDEPTH];

struct BitMapEx *BlocksBitmap,*ScreenBitmap;
struct RawMap *Map;
UBYTE	 *frontbuffer,*blocksbuffer;


WORD	bitmapheight;

UWORD 	*mapdata;
LONG MapSize = 0;
UWORD	colors[BLOCKSCOLORS];
BOOL	option_ntsc,option_how,option_speed;
WORD	option_fetchmode;
UBYTE *hud_buffer = NULL;
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

void InitHUD(void);
 
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
l = (BITMAPBYTESPERROW * BLOCKSDEPTH) - (26  );

	CopBPLMODA[VALUE] = l;
	CopBPLMODB[VALUE] = l;
	
	// display window start/stop
	
	CopDIWSTART[VALUE] = DIWSTARTVAL;
	CopDIWSTOP[VALUE] = DIWSTOPVAL;
	
	// display data fetch start/stop
	
	CopDDFSTART[VALUE] = 0x38;
    CopDDFSTOP[VALUE] = 0x98;   // Changed from 0xD0 to stop at 192 pixels
	
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

struct BitMapEx *HUDBitmap = NULL;


void InitHUD(void)
{
    HUDBitmap = BitMapEx_Create(BLOCKSDEPTH, 144, SCREENHEIGHT);
    if (!HUDBitmap) {
        Cleanup("Failed to create HUD bitmap");
    }
    
    hud_buffer = HUDBitmap->planes[0];
    
    // Fill with bright cyan (color 6) for debugging
    // Set bits in planes 1 and 2 only: 0110 binary = 6
    const USHORT hud_lineSize = 144/8;  // 18 bytes
    
    memset(hud_buffer, 0, hud_lineSize * SCREENHEIGHT * BLOCKSDEPTH);  // Clear all
    
    // Set plane 1 (offset: hud_lineSize * 1)
    memset(hud_buffer + hud_lineSize * 1, 0xFF, hud_lineSize * SCREENHEIGHT);
    
    // Set plane 2 (offset: hud_lineSize * 2)  
    memset(hud_buffer + hud_lineSize * 2, 0xFF, hud_lineSize * SCREENHEIGHT);
    
    KPrintF("HUD initialized: 144 x 256 pixels (cyan for debug)\n");
}

 
static void OpenMap(void)
{
 
	
	if (!(MapHandle = Open("maps/level1.dat",MODE_OLDFILE)))
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
	mapheight = Map->mapheight;  
}
 
static void OpenBlocks(void)
{
	 
	LONG l = 0;

	BlocksBitmap = BitMapEx_Create(BLOCKSDEPTH, BLOCKSWIDTH, BLOCKSHEIGHT);

	if (!(BlocksBitmap))
	{	 
		Cleanup("Can't alloc blocks bitmap!");
	}
	
	if (!(MapHandle = Open("tiles/lv1_tiles.BPL",MODE_OLDFILE)))
	{
		Cleanup(s);
	}
	
	l = BLOCKSWIDTH * BLOCKSHEIGHT * BLOCKSDEPTH / 8;
	
	if (Read(MapHandle,BlocksBitmap->planes[0],l) != l)
	{
	 
		Cleanup(s);
	}
	
	Close(MapHandle);MapHandle = 0;


	if (!(MapHandle = Open("tiles/lv1_tiles.PAL",MODE_OLDFILE)))
	{
		Cleanup(s);
	}

	if (Read(MapHandle,colors,PALSIZE) != PALSIZE)
	{
		Cleanup(s);
	}

	Close(MapHandle);MapHandle = 0;
	
	blocksbuffer = BlocksBitmap->planes[0];

	debug_register_bitmap(blocksbuffer, "blocksbuffer", 320, 256, 4, debug_resource_bitmap_interleaved);
	debug_register_palette(colors,"palette",16,0); 

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
 

__attribute__((always_inline)) inline static void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy)
{
	 
	x = (x / 8) & 0xFFFE;
	y = y * BITMAPBYTESPERROW;
	
	UWORD block = mapdata[mapy * mapwidth + mapx];

	mapx = (block % BLOCKSPERROW) * (BLOCKWIDTH / 8);
	mapy = (block / BLOCKSPERROW) * (BLOCKPLANELINES * BLOCKSBYTESPERROW);
 
	
	HardWaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = BLOCKSBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= frontbuffer + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH / 16);
}
 
static void ScrollUp(void)
{
	 WORD mapx, mapy, x, y;

    if (mapposy < 1) return;  // Stop at top of map

    mapposy--;
    videoposy = mapposy % HALFBITMAPHEIGHT;

	mapx = mapposy & (NUMSTEPS - 1);
	mapy = mapposy / BLOCKHEIGHT;
	
	y = ROUND2BLOCKHEIGHT(videoposy) * BLOCKSDEPTH;

   
   	// blit only one block per half bitmap
   	
   	x = mapx * BLOCKWIDTH;
   	
   	DrawBlock(x,y,mapx,mapy);
   	DrawBlock(x,y + HALFBITMAPHEIGHT * BLOCKSDEPTH,mapx,mapy);
   	
   
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
	WORD a, b, x, y;
	WORD start_tile_y = mapposy / BLOCKHEIGHT;

	for (b = 0; b < HALFBITMAPBLOCKSPERCOL; b++)
	{
		for (a = 0; a < 12; a++)  // 12 tiles for 192 pixel width
		{
			x = a * BLOCKWIDTH;
			y = b * BLOCKPLANELINES;
			DrawBlock(x, y, a, start_tile_y + b);
			DrawBlock(x, y + HALFBITMAPHEIGHT * BLOCKSDEPTH, a, start_tile_y + b);
		}
	} 
 
}
 

// Add these to your global variables
WORD bike_sspeed = 42;        // Current speed in mph (starts at idle)
WORD bike_acceleration = 0;  // Current acceleration
#define MIN_SPEED 42
#define MAX_SPEED 150
#define ACCEL_RATE 2         // Speed increase per frame when accelerating
#define DECEL_RATE 1         // Speed decrease per frame when coasting

WORD scroll_accumulator = 0;  // Fractional scroll position (fixed point)



// Convert speed to scroll calls
WORD GetScrollAmount(WORD speed)
{
    // Map 42-150 mph to 2-6 scroll calls
    // Linear interpolation: scrolls = 2 + ((speed - 42) / (150 - 42)) * 4
    if (speed <= MIN_SPEED) return 2;
    if (speed >= MAX_SPEED) return 6;
    
    // Calculate proportional scroll amount
    WORD range = speed - MIN_SPEED;
    WORD max_range = MAX_SPEED - MIN_SPEED;
    return 2 + ((range * 4) / max_range);
}

static void SmoothScroll(void)
{
    // Calculate scroll speed in fixed-point (8.8 format: 8 bits integer, 8 bits fraction)
    // 2 scrolls = 512 (2 << 8), 6 scrolls = 1536 (6 << 8)
    WORD scroll_speed = GetScrollAmount(bike_speed) << 8;
    
    // Add to accumulator
    scroll_accumulator += scroll_speed;
    
    // Extract whole pixels and scroll
    while (scroll_accumulator >= 256) {
        ScrollUp();
        scroll_accumulator -= 256;
    }
}

static void CheckJoyScroll(void)
{
    if (JoyFire()) {
        bike_state = BIKE_STATE_ACCELERATING;
        bike_speed += ACCEL_RATE;
        if (bike_speed > MAX_SPEED) bike_speed = MAX_SPEED;
    } else {
		 if (bike_speed > MIN_SPEED)
        	bike_speed -= DECEL_RATE;
        if (bike_speed < MIN_SPEED) bike_speed = MIN_SPEED;
    }
    
    SmoothScroll();  // Use smooth scrolling instead of loop
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

	OpenMap();
	OpenBlocks();
	OpenDisplay();

	//TakeSystem();
 
	KillSystem();	

		InitHUD();
	InitCopperlist();

	Copper_SetPalette(0, 0x003);

	NewGame(0);
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
 
	
		CheckJoyScroll();
		UpdateCopperlist();
 
	}

    ActivateSystem();

	Cleanup("");

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);

    return 0;
}