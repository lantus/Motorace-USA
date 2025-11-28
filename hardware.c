#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <devices/input.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "hardware.h"

static UWORD old_dmacon;
static UWORD old_intena;
static UWORD old_adkcon;
static UWORD old_intreq;

static struct MsgPort *inputmp;
static struct IOStdReq *inputreq;
static struct Interrupt inputhandler;
static BOOL	inputdeviceok;

static struct View	*oldview;
static struct Window *old_processwinptr;
static struct Process *thisprocess;

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;
extern volatile struct Custom *custom;

static volatile ULONG *custom_vposr = (volatile ULONG *) 0xdff004;

// System state backup
struct View *saved_view;
UWORD saved_intena, saved_dma, saved_adkcon;
 
BOOL os_disabled = FALSE;

static LONG NullInputHandler(void)
{
	// kills all input
	return 0;
}

void KillSystem()
{
	thisprocess = (struct Process *)FindTask(0);

	// safe actual view and install null view

	oldview = GfxBase->ActiView;
	
	LoadView(0);
	WaitTOF();
	WaitTOF();
 

	// lock blitter

	OwnBlitter();
	WaitBlit();
	
	// no multitasking/interrupts

	Disable();

	// save important custom registers
	
	old_dmacon = custom->dmaconr | 0x8000;
	old_intena = custom->intenar | 0x8000;
	old_adkcon = custom->adkconr | 0x8000;
	old_intreq = custom->intreqr | 0x8000;

}

void ActivateSystem(void)
{
	// reset important custom registers
	
	custom->dmacon = 0x7FFF;
	custom->intena = 0x7FFF;
	custom->adkcon = 0x7FFF;
	custom->intreq = 0x7FFF;

	custom->dmacon = old_dmacon;
	custom->intena = old_intena;
	custom->adkcon = old_adkcon;
	custom->intreq = old_intreq;

	// enable multitasking/interrupts

	Enable();

	// unlock blitter

	WaitBlit();
	DisownBlitter();

	// enable requesters for our process
	
	thisprocess->pr_WindowPtr = old_processwinptr;

	// remove null inputhandler
	
	if (inputdeviceok)
	{
		inputreq->io_Command = IND_REMHANDLER;
		inputreq->io_Data = &inputhandler;
		DoIO((struct IORequest *)inputreq);
		
		CloseDevice((struct IORequest *)inputreq);
	}

	if (inputreq) DeleteIORequest(inputreq);
	if (inputmp) DeleteMsgPort(inputmp);

	// reset old view

	LoadView(oldview);
	WaitTOF();
	WaitTOF();	
}

void WaitVBL(void)
{
	while (((*custom_vposr) & 0x1ff00) != (262<<8)) ;
}

void WaitVBeam(ULONG line)
{
	while (1) {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}

}

void HardWaitBlit(void)
{
	if (custom->dmaconr & DMAF_BLTDONE)
	{
	}

	while (custom->dmaconr & DMAF_BLTDONE)
	{
	}
}

void HardWaitLMB(void)
{
	while (((*(UBYTE *)0xbfe001) & 64) != 0)
	{
	}

	while (((*(UBYTE *)0xbfe001) & 64) == 0)
	{
	}

}

BOOL JoyLeft(void)
{
	return (custom->joy1dat & 512) ? TRUE : FALSE;
}

BOOL JoyRight(void)
{
	return (custom->joy1dat & 2) ? TRUE : FALSE;
}

BOOL JoyUp(void)
{
	// ^ = xor

	WORD w;
	
	w = custom->joy1dat << 1;

	return ((w ^ custom->joy1dat) & 512) ? TRUE : FALSE;
}

BOOL JoyDown(void)
{
	// ^ = xor

	WORD w;
	
	w = custom->joy1dat << 1;

	return ((w ^ custom->joy1dat) & 2) ? TRUE : FALSE;
}

BOOL JoyFire(void)
{
	return ((*(UBYTE *)0xbfe001) & 128) ? FALSE : TRUE;
}

BOOL LMBDown(void)
{
	return ((*(UBYTE *)0xbfe001) & 64) ? FALSE : TRUE;
}

void System_DisableOS()
{
    if (os_disabled) return;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    
    // Save system state
    saved_view = GfxBase->ActiView;
    saved_intena = custom->intenar;
    saved_dma = custom->dmaconr;
    saved_adkcon = custom->adkconr;
    
    Forbid();
    
    // Remove OS display
    LoadView(NULL);
    WaitTOF();
    WaitTOF();
    
    // Disable interrupts and DMA
    custom->intena = 0x7FFF;
    custom->dmacon = 0x7FFF;
    
    os_disabled = TRUE;
}

void System_EnableOS()
{
    if (!os_disabled) return;
    
    // Stop our hardware usage
    custom->intena = 0x7FFF;
    custom->dmacon = 0x7FFF;
    
    // Restore OS state
    custom->intena = 0x8000 | saved_intena;
    custom->dmacon = 0x8000 | saved_dma;
    custom->adkcon = 0x8000 | saved_adkcon;
    
    // Restore OS display
    LoadView(saved_view);
    WaitTOF();
    WaitTOF();
    
    Permit();
    
    CloseLibrary((struct Library *)GfxBase);
    
    os_disabled = FALSE;
}