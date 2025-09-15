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
	ULONG vpos;

	line *= 0x100;

	do {
			vpos = *(ULONG *)0xdff004;
	} while ((vpos & 0x1FF00) != line);

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

