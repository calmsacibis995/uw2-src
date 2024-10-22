/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:dix/devices.c	1.6"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/


/* $XConsortium: devices.c,v 5.22 91/12/10 11:19:22 keith Exp $ */

#include "X.h"
#include "misc.h"
#include "resource.h"
#define NEED_EVENTS
#define NEED_REPLIES
#include "Xproto.h"
#include "windowstr.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "site.h"

extern InputInfo inputInfo;
extern int (* InitialVector[3]) ();
extern void (* ReplySwapVector[256]) ();
extern void CopySwap32Write(), SwapTimeCoordWrite();
extern void ActivatePointerGrab(), DeactivatePointerGrab();
extern void ActivateKeyboardGrab(), DeactivateKeyboardGrab();
extern Mask EventMaskForClient();
extern void EnqueueEvent();

DevicePtr
AddInputDevice(deviceProc, autoStart)
    DeviceProc deviceProc;
    Bool autoStart;
{
    register DeviceIntPtr dev;

    if (inputInfo.numDevices >= MAX_DEVICES)
	return (DevicePtr)NULL;
    dev = (DeviceIntPtr) xalloc(sizeof(DeviceIntRec));
    if (!dev)
	return (DevicePtr)NULL;
    dev->name = (char *)NULL;
    dev->type = 0;
    dev->id = inputInfo.numDevices;
    inputInfo.numDevices++;
    dev->public.on = FALSE;
    dev->public.processInputProc = NoopDDA;
    dev->public.realInputProc = NoopDDA;
    dev->public.enqueueInputProc = EnqueueEvent;
    dev->deviceProc = deviceProc;
    dev->startup = autoStart;
    dev->sync.frozen = FALSE;
    dev->sync.other = NullGrab;
    dev->sync.state = NOT_GRABBED;
    dev->sync.event = (xEvent *) NULL;
    dev->grab = NullGrab;
    dev->grabTime = currentTime;
    dev->fromPassiveGrab = FALSE;
    dev->key = (KeyClassPtr)NULL;
    dev->valuator = (ValuatorClassPtr)NULL;
    dev->button = (ButtonClassPtr)NULL;
    dev->focus = (FocusClassPtr)NULL;
    dev->proximity = (ProximityClassPtr)NULL;
    dev->kbdfeed = (KbdFeedbackPtr)NULL;
    dev->ptrfeed = (PtrFeedbackPtr)NULL;
    dev->intfeed = (IntegerFeedbackPtr)NULL;
    dev->stringfeed = (StringFeedbackPtr)NULL;
    dev->bell = (BellFeedbackPtr)NULL;
    dev->leds = (LedFeedbackPtr)NULL;
    dev->next = inputInfo.off_devices;
    inputInfo.off_devices = dev;
    return &dev->public;
}

Bool
EnableDevice(dev)
    register DeviceIntPtr dev;
{
    register DeviceIntPtr *prev;

    for (prev = &inputInfo.off_devices;
	 *prev && (*prev != dev);
	 prev = &(*prev)->next)
	;
    if ((*prev != dev) || !dev->inited ||
	((*dev->deviceProc)(dev, DEVICE_ON) != Success))
	return FALSE;
    *prev = dev->next;
    dev->next = inputInfo.devices;
    inputInfo.devices = dev;
    return TRUE;
}

Bool
DisableDevice(dev)
    register DeviceIntPtr dev;
{
    register DeviceIntPtr *prev;

    for (prev = &inputInfo.devices;
	 *prev && (*prev != dev);
	 prev = &(*prev)->next)
	;
    if (*prev != dev)
	return FALSE;
    (void)(*dev->deviceProc)(dev, DEVICE_OFF);
    *prev = dev->next;
    dev->next = inputInfo.off_devices;
    inputInfo.off_devices = dev;
    return TRUE;
}

int
InitAndStartDevices()
{
    register DeviceIntPtr dev, next;

    for (dev = inputInfo.off_devices; dev; dev = dev->next)
	dev->inited = ((*dev->deviceProc)(dev, DEVICE_INIT) == Success);
    for (dev = inputInfo.off_devices; dev; dev = next)
    {
	next = dev->next;
	if (dev->inited && dev->startup)
	    (void)EnableDevice(dev);
    }
    for (dev = inputInfo.devices;
	 dev && (dev != inputInfo.keyboard);
	 dev = dev->next)
	;
    if (!dev || (dev != inputInfo.keyboard))
	return BadImplementation;
    for (dev = inputInfo.devices;
	 dev && (dev != inputInfo.pointer);
	 dev = dev->next)
	;
    if (!dev || (dev != inputInfo.pointer))
	return BadImplementation;
    return Success;
}

static void
CloseDevice(dev)
    register DeviceIntPtr dev;
{
    KbdFeedbackPtr k, knext;
    PtrFeedbackPtr p, pnext;
    IntegerFeedbackPtr i, inext;
    StringFeedbackPtr s, snext;
    BellFeedbackPtr b, bnext;
    LedFeedbackPtr l, lnext;

    if (dev->inited)
	(void)(*dev->deviceProc)(dev, DEVICE_CLOSE);
    xfree(dev->name);
    if (dev->key)
    {
	xfree(dev->key->curKeySyms.map);
	xfree(dev->key->modifierKeyMap);
	xfree(dev->key);
    }
    xfree(dev->valuator);
    xfree(dev->button);
    if (dev->focus)
    {
	xfree(dev->focus->trace);
	xfree(dev->focus);
    }
    xfree(dev->proximity);
    for (k=dev->kbdfeed; k; k=knext)
    {
	knext = k->next;
	xfree(k);
    }
    for (p=dev->ptrfeed; p; p=pnext)
    {
	pnext = p->next;
	xfree(p);
    }
    for (i=dev->intfeed; i; i=inext)
    {
	inext = i->next;
	xfree(i);
    }
    for (s=dev->stringfeed; s; s=snext)
    {
	snext = s->next;
	xfree(s->ctrl.symbols_supported);
	xfree(s->ctrl.symbols_displayed);
	xfree(s);
    }
    for (b=dev->bell; b; b=bnext)
    {
	bnext = b->next;
	xfree(b);
    }
    for (l=dev->leds; l; l=lnext)
    {
	lnext = l->next;
	xfree(l);
    }
    xfree(dev);
}

void
CloseDownDevices()
{
    register DeviceIntPtr dev, next;

    for (dev = inputInfo.devices; dev; dev = next)
    {
	next = dev->next;
	CloseDevice(dev);
    }
    for (dev = inputInfo.off_devices; dev; dev = next)
    {
	next = dev->next;
	CloseDevice(dev);
    }
}

int
NumMotionEvents()
{
    return inputInfo.pointer->valuator->numMotionEvents;
}

void
RegisterPointerDevice(device)
    DevicePtr device;
{
    inputInfo.pointer = (DeviceIntPtr)device;
    device->processInputProc = ProcessPointerEvent;
    device->realInputProc = ProcessPointerEvent;
    ((DeviceIntPtr)device)->ActivateGrab = ActivatePointerGrab;
    ((DeviceIntPtr)device)->DeactivateGrab = DeactivatePointerGrab;
}

void
RegisterKeyboardDevice(device)
    DevicePtr device;
{
    inputInfo.keyboard = (DeviceIntPtr)device;
    device->processInputProc = ProcessKeyboardEvent;
    device->realInputProc = ProcessKeyboardEvent;
    ((DeviceIntPtr)device)->ActivateGrab = ActivateKeyboardGrab;
    ((DeviceIntPtr)device)->DeactivateGrab = DeactivateKeyboardGrab;
}

DevicePtr
LookupKeyboardDevice()
{
    return &inputInfo.keyboard->public;
}

DevicePtr
LookupPointerDevice()
{
    return &inputInfo.pointer->public;
}

void
QueryMinMaxKeyCodes(minCode, maxCode)
    KeyCode *minCode, *maxCode;
{
    *minCode = inputInfo.keyboard->key->curKeySyms.minKeyCode;
    *maxCode = inputInfo.keyboard->key->curKeySyms.maxKeyCode;
}

Bool
SetKeySymsMap(dst, src)
    register KeySymsPtr dst, src;
{
    int i, j;
    int rowDif = src->minKeyCode - dst->minKeyCode;
           /* if keysym map size changes, grow map first */

    if (src->mapWidth < dst->mapWidth)
    {
        for (i = src->minKeyCode; i <= (int)src->maxKeyCode; i++)
	{
#define SI(r, c) (((r-src->minKeyCode)*src->mapWidth) + (c))
#define DI(r, c) (((r - dst->minKeyCode)*dst->mapWidth) + (c))
	    for (j = 0; j < src->mapWidth; j++)
		dst->map[DI(i, j)] = src->map[SI(i, j)];
	    for (j = src->mapWidth; j < dst->mapWidth; j++)
		dst->map[DI(i, j)] = NoSymbol;
#undef SI
#undef DI
	}
	return TRUE;
    }
    else if (src->mapWidth > dst->mapWidth)
    {
        KeySym *map;
	int bytes = sizeof(KeySym) * src->mapWidth *
		    (dst->maxKeyCode - dst->minKeyCode + 1);
        map = (KeySym *)xalloc(bytes);
	if (!map)
	    return FALSE;
	bzero((char *)map, bytes);
        if (dst->map)
	{
            for (i = 0; i <= (int)(dst->maxKeyCode-dst->minKeyCode); i++)
		bcopy((char *)&dst->map[i*dst->mapWidth],
		      (char *)&map[i*src->mapWidth],
		      dst->mapWidth * sizeof(KeySym));
	    xfree(dst->map);
	}
	dst->mapWidth = src->mapWidth;
	dst->map = map;
    }
    bcopy((char *)src->map,
	  (char *)&dst->map[rowDif * dst->mapWidth],
	  (int)(src->maxKeyCode - src->minKeyCode + 1) *
	  dst->mapWidth * sizeof(KeySym));
    return TRUE;
}

static Bool
InitModMap(keyc)
    register KeyClassPtr keyc;
{
    int i, j;
    CARD8 keysPerModifier[8];
    CARD8 mask;

    keyc->maxKeysPerModifier = 0;
    for (i = 0; i < 8; i++)
	keysPerModifier[i] = 0;
    for (i = 8; i < MAP_LENGTH; i++)
    {
	for (j = 0, mask = 1; j < 8; j++, mask <<= 1)
	{
	    if (mask & keyc->modifierMap[i])
	    {
		if (++keysPerModifier[j] > keyc->maxKeysPerModifier)
		    keyc->maxKeysPerModifier = keysPerModifier[j];
	    }
	}
    }
    keyc->modifierKeyMap = (KeyCode *)xalloc(8*keyc->maxKeysPerModifier);
    if (!keyc->modifierKeyMap && keyc->maxKeysPerModifier)
	return (FALSE);
    bzero((char *)keyc->modifierKeyMap, 8*(int)keyc->maxKeysPerModifier);
    for (i = 0; i < 8; i++)
	keysPerModifier[i] = 0;
    for (i = 8; i < MAP_LENGTH; i++)
    {
	for (j = 0, mask = 1; j < 8; j++, mask <<= 1)
	{
	    if (mask & keyc->modifierMap[i])
	    {
		keyc->modifierKeyMap[(j*keyc->maxKeysPerModifier) +
				     keysPerModifier[j]] = i;
		keysPerModifier[j]++;
	    }
	}
    }
    return TRUE;
}

Bool
InitKeyClassDeviceStruct(dev, pKeySyms, pModifiers)
    DeviceIntPtr dev;
    KeySymsPtr pKeySyms;
    CARD8 pModifiers[];
{
    int i;
    register KeyClassPtr keyc;

    keyc = (KeyClassPtr)xalloc(sizeof(KeyClassRec));
    if (!keyc)
	return FALSE;
    keyc->curKeySyms.map = (KeySym *)NULL;
    keyc->curKeySyms.mapWidth = 0;
    keyc->curKeySyms.minKeyCode = pKeySyms->minKeyCode;
    keyc->curKeySyms.maxKeyCode = pKeySyms->maxKeyCode;
    keyc->modifierKeyMap = (KeyCode *)NULL;
    keyc->state = 0;
    if (pModifiers)
	bcopy((char *)pModifiers, (char *)keyc->modifierMap, MAP_LENGTH);
    else
	bzero((char *)keyc->modifierMap, MAP_LENGTH);
    bzero((char *)keyc->down, DOWN_LENGTH);
    for (i = 0; i < 8; i++)
	keyc->modifierKeyCount[i] = 0;
    if (!SetKeySymsMap(&keyc->curKeySyms, pKeySyms) || !InitModMap(keyc))
    {
	xfree(keyc->curKeySyms.map);
	xfree(keyc->modifierKeyMap);
	xfree(keyc);
	return FALSE;
    }

    SetLockMeaning(keyc);

    dev->key = keyc;
    return TRUE;
}

Bool
InitButtonClassDeviceStruct(dev, numButtons, map)
    register DeviceIntPtr dev;
    int numButtons;
    CARD8 *map;
{
    register ButtonClassPtr butc;
    int i;

    butc = (ButtonClassPtr)xalloc(sizeof(ButtonClassRec));
    if (!butc)
	return FALSE;
    butc->numButtons = numButtons;
    for (i = 1; i <= numButtons; i++)
	butc->map[i] = map[i];
    butc->buttonsDown = 0;
    butc->state = 0;
    butc->motionMask = 0;
    bzero((char *)butc->down, DOWN_LENGTH);
    dev->button = butc;
    return TRUE;
}

Bool
InitValuatorClassDeviceStruct(dev, numAxes, motionProc, numMotionEvents, mode)
    DeviceIntPtr dev;
    int (*motionProc)();
    int numAxes;
    int numMotionEvents;
    int mode;
{
    int i;
    register ValuatorClassPtr valc;

    valc = (ValuatorClassPtr)xalloc(sizeof(ValuatorClassRec) +
				    numAxes * sizeof(AxisInfo) +
				    numAxes * sizeof(unsigned int));
    if (!valc)
	return FALSE;
    valc->GetMotionProc = motionProc;
    valc->numMotionEvents = numMotionEvents;
    valc->motionHintWindow = NullWindow;
    valc->numAxes = numAxes;
    valc->mode = mode;
    valc->axes = (AxisInfoPtr)(valc + 1);
    valc->axisVal = (int *)(valc->axes + numAxes);
    for (i=0; i<numAxes; i++)
	valc->axisVal[i]=0;
    dev->valuator = valc;
    return TRUE;
}

Bool
InitFocusClassDeviceStruct(dev)
    DeviceIntPtr dev;
{
    register FocusClassPtr focc;

    focc = (FocusClassPtr)xalloc(sizeof(FocusClassRec));
    if (!focc)
	return FALSE;
    focc->win = PointerRootWin;
    focc->revert = None;
    focc->time = currentTime;
    focc->trace = (WindowPtr *)NULL;
    focc->traceSize = 0;
    focc->traceGood = 0;
    dev->focus = focc;
    return TRUE;
}

Bool
InitKbdFeedbackClassDeviceStruct(dev, bellProc, controlProc)
    DeviceIntPtr dev;
    void (*bellProc)();
    void (*controlProc)();
{
    register KbdFeedbackPtr feedc;

    feedc = (KbdFeedbackPtr)xalloc(sizeof(KbdFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->BellProc = bellProc;
    feedc->CtrlProc = controlProc;
    feedc->ctrl = defaultKeyboardControl;
    feedc->ctrl.id = 0;
    feedc->next = dev->kbdfeed;
    if (feedc->next)
	feedc->ctrl.id = dev->kbdfeed->ctrl.id + 1;
    dev->kbdfeed = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}

Bool
InitPtrFeedbackClassDeviceStruct(dev, controlProc)
    DeviceIntPtr dev;
    void (*controlProc)();
{
    register PtrFeedbackPtr feedc;

    feedc = (PtrFeedbackPtr)xalloc(sizeof(PtrFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->CtrlProc = controlProc;
    feedc->ctrl = defaultPointerControl;
    feedc->ctrl.id = 0;
    feedc->next = dev->ptrfeed;
    if (feedc->next)
        feedc->ctrl.id = dev->ptrfeed->ctrl.id + 1;
    dev->ptrfeed = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}


LedCtrl defaultLedControl = {
	DEFAULT_LEDS, DEFAULT_LEDS_MASK, 0};

BellCtrl defaultBellControl = {
	DEFAULT_BELL,
	DEFAULT_BELL_PITCH,
	DEFAULT_BELL_DURATION,
	0};

IntegerCtrl defaultIntegerControl = {
	DEFAULT_INT_RESOLUTION,
	DEFAULT_INT_MIN_VALUE,
	DEFAULT_INT_MAX_VALUE,
	DEFAULT_INT_DISPLAYED,
	0};

Bool
InitStringFeedbackClassDeviceStruct (dev, controlProc, max_symbols,
				     num_symbols_supported, symbols)
    DeviceIntPtr dev;
    void (*controlProc)();
    int max_symbols;
    int num_symbols_supported;
    KeySym *symbols;
{
    int i;
    register StringFeedbackPtr feedc;

    feedc = (StringFeedbackPtr)xalloc(sizeof(StringFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->CtrlProc = controlProc;
    feedc->ctrl.num_symbols_supported = num_symbols_supported;
    feedc->ctrl.num_symbols_displayed = 0;
    feedc->ctrl.max_symbols = max_symbols;
    feedc->ctrl.symbols_supported = (KeySym *) 
	xalloc (sizeof (KeySym) * num_symbols_supported);
    feedc->ctrl.symbols_displayed = (KeySym *) 
	xalloc (sizeof (KeySym) * max_symbols);
    if (!feedc->ctrl.symbols_supported || !feedc->ctrl.symbols_displayed)
    {
	xfree(feedc->ctrl.symbols_supported);
	xfree(feedc->ctrl.symbols_displayed);
	xfree(feedc);
	return FALSE;
    }
    for (i=0; i<num_symbols_supported; i++)
	*(feedc->ctrl.symbols_supported+i) = *symbols++;
    for (i=0; i<max_symbols; i++)
	*(feedc->ctrl.symbols_displayed+i) = (KeySym) NULL;
    feedc->ctrl.id = 0;
    feedc->next = dev->stringfeed;
    if (feedc->next)
	feedc->ctrl.id = dev->stringfeed->ctrl.id + 1;
    dev->stringfeed = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}

Bool
InitBellFeedbackClassDeviceStruct (dev, bellProc, controlProc)
    DeviceIntPtr dev;
    void (*bellProc)();
    void (*controlProc)();
{
    register BellFeedbackPtr feedc;

    feedc = (BellFeedbackPtr)xalloc(sizeof(BellFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->CtrlProc = controlProc;
    feedc->BellProc = bellProc;
    feedc->ctrl = defaultBellControl;
    feedc->ctrl.id = 0;
    feedc->next = dev->bell;
    if (feedc->next)
	feedc->ctrl.id = dev->bell->ctrl.id + 1;
    dev->bell = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}

Bool
InitLedFeedbackClassDeviceStruct (dev, controlProc)
    DeviceIntPtr dev;
    void (*controlProc)();
{
    register LedFeedbackPtr feedc;

    feedc = (LedFeedbackPtr)xalloc(sizeof(LedFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->CtrlProc = controlProc;
    feedc->ctrl = defaultLedControl;
    feedc->ctrl.id = 0;
    feedc->next = dev->leds;
    if (feedc->next)
	feedc->ctrl.id = dev->leds->ctrl.id + 1;
    dev->leds = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}

Bool
InitIntegerFeedbackClassDeviceStruct (dev, controlProc)
    DeviceIntPtr dev;
    void (*controlProc)();
{
    register IntegerFeedbackPtr feedc;

    feedc = (IntegerFeedbackPtr)xalloc(sizeof(IntegerFeedbackClassRec));
    if (!feedc)
	return FALSE;
    feedc->CtrlProc = controlProc;
    feedc->ctrl = defaultIntegerControl;
    feedc->ctrl.id = 0;
    feedc->next = dev->intfeed;
    if (feedc->next)
	feedc->ctrl.id = dev->intfeed->ctrl.id + 1;
    dev->intfeed = feedc;
    (*controlProc)(dev, &feedc->ctrl);
    return TRUE;
}

Bool
InitPointerDeviceStruct(device, map, numButtons, motionProc, controlProc,
			numMotionEvents)
    DevicePtr device;
    CARD8 *map;
    int numButtons;
    void (*controlProc)();
    int (*motionProc)();
    int numMotionEvents;
{
    DeviceIntPtr dev = (DeviceIntPtr)device;

    return(InitButtonClassDeviceStruct(dev, numButtons, map) &&
	   InitValuatorClassDeviceStruct(dev, 2, motionProc,
					 numMotionEvents, 0) &&
	   InitPtrFeedbackClassDeviceStruct(dev, controlProc));
}

Bool
InitKeyboardDeviceStruct(device, pKeySyms, pModifiers, bellProc, controlProc)
    DevicePtr device;
    KeySymsPtr pKeySyms;
    CARD8 pModifiers[];
    void (*bellProc)();
    void (*controlProc)();
{
    DeviceIntPtr dev = (DeviceIntPtr)device;

    return(InitKeyClassDeviceStruct(dev, pKeySyms, pModifiers) &&
	   InitFocusClassDeviceStruct(dev) &&
	   InitKbdFeedbackClassDeviceStruct(dev, bellProc, controlProc));
}

void
SendMappingNotify(request, firstKeyCode, count)
    CARD8 request, count;
    KeyCode firstKeyCode;
{
    int i;
    xEvent event;

    event.u.u.type = MappingNotify;
    event.u.mappingNotify.request = request;
    if (request == MappingKeyboard)
    {
        event.u.mappingNotify.firstKeyCode = firstKeyCode;
        event.u.mappingNotify.count = count;
    }
    /* 0 is the server client */
    for (i=1; i<currentMaxClients; i++)
        if (clients[i] && ! clients[i]->clientGone &&
	    (clients[i]->requestVector != InitialVector))
	{
	    event.u.u.sequenceNumber = clients[i]->sequence;
            WriteEventsToClient(clients[i], 1, &event);
	}
}

/*
 * n-squared algorithm. n < 255 and don't want to copy the whole thing and
 * sort it to do the checking. How often is it called? Just being lazy?
 */
Bool
BadDeviceMap(buff, length, low, high, errval)
    register BYTE *buff;
    int length;
    unsigned low, high;
    XID *errval;
{
    register int     i, j;

    for (i = 0; i < length; i++)
	if (buff[i])		       /* only check non-zero elements */
	{
	    if ((low > buff[i]) || (high < buff[i]))
	    {
		*errval = buff[i];
		return TRUE;
	    }
	    for (j = i + 1; j < length; j++)
		if (buff[i] == buff[j])
		{
		    *errval = buff[i];
		    return TRUE;
		}
	}
    return FALSE;
}

Bool
AllModifierKeysAreUp(dev, map1, per1, map2, per2)
    register DeviceIntPtr dev;
    register CARD8 *map1, *map2;
    int per1, per2;
{
    register int i, j, k;
    register CARD8 *down = dev->key->down;

    for (i = 8; --i >= 0; map2 += per2)
    {
	for (j = per1; --j >= 0; map1++)
	{
	    if (*map1 && BitIsOn(down, *map1))
	    {
		for (k = per2; (--k >= 0) && (*map1 != map2[k]);)
		  ;
		if (k < 0)
		    return FALSE;
	    }
	}
    }
    return TRUE;
}

int 
ProcSetModifierMapping(client)
    ClientPtr client;
{
    xSetModifierMappingReply rep;
    REQUEST(xSetModifierMappingReq);
    KeyCode *inputMap;
    int inputMapLen;
    register int i;
    DeviceIntPtr keybd = inputInfo.keyboard;
    register KeyClassPtr keyc = keybd->key;

    REQUEST_AT_LEAST_SIZE(xSetModifierMappingReq);

    if (stuff->length != ((stuff->numKeyPerModifier<<1) +
			  (sizeof (xSetModifierMappingReq)>>2)))
	return BadLength;

    inputMapLen = 8*stuff->numKeyPerModifier;
    inputMap = (KeyCode *)&stuff[1];

    /*
     *	Now enforce the restriction that "all of the non-zero keycodes must be
     *	in the range specified by min-keycode and max-keycode in the
     *	connection setup (else a Value error)"
     */
    i = inputMapLen;
    while (i--)
    {
	if (inputMap[i]
	    && (inputMap[i] < keyc->curKeySyms.minKeyCode
		|| inputMap[i] > keyc->curKeySyms.maxKeyCode))
	{
	    client->errorValue = inputMap[i];
	    return BadValue;
	}
    }
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.success = MappingSuccess;

    /*
     *	Now enforce the restriction that none of the old or new
     *	modifier keys may be down while we change the mapping,  and
     *	that the DDX layer likes the choice.
     */
    if (!AllModifierKeysAreUp(keybd, keyc->modifierKeyMap,
			      (int)keyc->maxKeysPerModifier,
			      inputMap, (int)stuff->numKeyPerModifier)
	    ||
	!AllModifierKeysAreUp(keybd, inputMap, (int)stuff->numKeyPerModifier,
			      keyc->modifierKeyMap,
			      (int)keyc->maxKeysPerModifier))
    {
	rep.success = MappingBusy;
    }
    else
    {
	for (i = 0; i < inputMapLen; i++)
	{
	    if (inputMap[i] && !LegalModifier(inputMap[i], keybd))
	    {
		rep.success = MappingFailed;
		break;
	    }
	}
    }

    if (rep.success == MappingSuccess)
    {
	KeyCode *map;
	/*
	 *	Now build the keyboard's modifier bitmap from the
	 *	list of keycodes.
	 */
	map = (KeyCode *)xalloc(inputMapLen);
	if (!map)
	    return BadAlloc;
	if (keyc->modifierKeyMap)
	    xfree(keyc->modifierKeyMap);
	keyc->modifierKeyMap = map;
	bcopy((char *)inputMap, (char *)map, inputMapLen);

	keyc->maxKeysPerModifier = stuff->numKeyPerModifier;
	for (i = 0; i < MAP_LENGTH; i++)
	    keyc->modifierMap[i] = 0;
	for (i = 0; i < inputMapLen; i++)
	{
	    if (inputMap[i])
		keyc->modifierMap[inputMap[i]] |=
		    (1<<(i/(unsigned) keyc->maxKeysPerModifier));
	}
    }

    SetLockMeaning(keyc);

    if (rep.success == MappingSuccess)
        SendMappingNotify(MappingModifier, 0, 0);

    WriteReplyToClient(client, sizeof(xSetModifierMappingReply), &rep);

    return(client->noClientException);
}

int
ProcGetModifierMapping(client)
    ClientPtr client;
{
    xGetModifierMappingReply rep;
    REQUEST(xReq);
    register KeyClassPtr keyc = inputInfo.keyboard->key;

    REQUEST_SIZE_MATCH(xReq);
    rep.type = X_Reply;
    rep.numKeyPerModifier = keyc->maxKeysPerModifier;
    rep.sequenceNumber = client->sequence;
    /* length counts 4 byte quantities - there are 8 modifiers 1 byte big */
    rep.length = keyc->maxKeysPerModifier << 1;

    WriteReplyToClient(client, sizeof(xGetModifierMappingReply), &rep);

    /* Use the (modified by DDX) map that SetModifierMapping passed in */
    (void)WriteToClient(client, (int)(keyc->maxKeysPerModifier << 3),
			(char *)keyc->modifierKeyMap);
    return client->noClientException;
}

int
ProcChangeKeyboardMapping(client)
    ClientPtr client;
{
    REQUEST(xChangeKeyboardMappingReq);
    unsigned len;
    KeySymsRec keysyms;
    register KeySymsPtr curKeySyms = &inputInfo.keyboard->key->curKeySyms;
    REQUEST_AT_LEAST_SIZE(xChangeKeyboardMappingReq);

    len = stuff->length - (sizeof(xChangeKeyboardMappingReq) >> 2);  
    if (len != (stuff->keyCodes * stuff->keySymsPerKeyCode))
            return BadLength;
    if ((stuff->firstKeyCode < curKeySyms->minKeyCode) ||
	(stuff->firstKeyCode > curKeySyms->maxKeyCode))
    {
	    client->errorValue = stuff->firstKeyCode;
	    return BadValue;
    }
    if (((unsigned) stuff->firstKeyCode + stuff->keyCodes - 1 > curKeySyms->maxKeyCode) ||
	(stuff->keySymsPerKeyCode == 0))
    {
	    client->errorValue = stuff->keySymsPerKeyCode;
	    return BadValue;
    }
    keysyms.minKeyCode = stuff->firstKeyCode;
    keysyms.maxKeyCode = stuff->firstKeyCode + stuff->keyCodes - 1;
    keysyms.mapWidth = stuff->keySymsPerKeyCode;
    keysyms.map = (KeySym *)&stuff[1];
    if (!SetKeySymsMap(curKeySyms, &keysyms))
	return BadAlloc;

    SetLockMeaning(inputInfo.keyboard->key);

    SendMappingNotify(MappingKeyboard, stuff->firstKeyCode, stuff->keyCodes);
    return client->noClientException;

}

int
ProcSetPointerMapping(client)
    ClientPtr client;
{
    REQUEST(xSetPointerMappingReq);
    BYTE *map;
    xSetPointerMappingReply rep;
    register int i;
    DeviceIntPtr mouse = inputInfo.pointer;

    REQUEST_AT_LEAST_SIZE(xSetPointerMappingReq);
    if (stuff->length != (sizeof(xSetPointerMappingReq) + stuff->nElts + 3)>>2)
	return BadLength;
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.success = MappingSuccess;
    map = (BYTE *)&stuff[1];
    if (stuff->nElts != mouse->button->numButtons)
    {
	client->errorValue = stuff->nElts;
	return BadValue;
    }
    if (BadDeviceMap(&map[0], (int)stuff->nElts, 1, 255, &client->errorValue))
	return BadValue;
    for (i=0; i < (int)stuff->nElts; i++)
	if ((mouse->button->map[i + 1] != map[i]) &&
	    BitIsOn(mouse->button->down, i + 1))
	{
    	    rep.success = MappingBusy;
	    WriteReplyToClient(client, sizeof(xSetPointerMappingReply), &rep);
            return Success;
	}
    for (i = 0; i < (int)stuff->nElts; i++)
	mouse->button->map[i + 1] = map[i];
    SendMappingNotify(MappingPointer, 0, 0);
    WriteReplyToClient(client, sizeof(xSetPointerMappingReply), &rep);
    return Success;
}

int
ProcGetKeyboardMapping(client)
    ClientPtr client;
{
    xGetKeyboardMappingReply rep;
    REQUEST(xGetKeyboardMappingReq);
    KeySymsPtr curKeySyms = &inputInfo.keyboard->key->curKeySyms;

    REQUEST_SIZE_MATCH(xGetKeyboardMappingReq);

    if ((stuff->firstKeyCode < curKeySyms->minKeyCode) ||
        (stuff->firstKeyCode > curKeySyms->maxKeyCode))
    {
	client->errorValue = stuff->firstKeyCode;
	return BadValue;
    }
    if ((unsigned) stuff->firstKeyCode + stuff->count > curKeySyms->maxKeyCode + 1)
    {
	client->errorValue = stuff->count;
        return BadValue;
    }

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.keySymsPerKeyCode = curKeySyms->mapWidth;
    /* length is a count of 4 byte quantities and KeySyms are 4 bytes */
    rep.length = (curKeySyms->mapWidth * stuff->count);
    WriteReplyToClient(client, sizeof(xGetKeyboardMappingReply), &rep);
    client->pSwapReplyFunc = CopySwap32Write;
    WriteSwappedDataToClient(
	client,
	curKeySyms->mapWidth * stuff->count * sizeof(KeySym),
	&curKeySyms->map[(stuff->firstKeyCode - curKeySyms->minKeyCode) *
			 curKeySyms->mapWidth]);

    return client->noClientException;
}

int
ProcGetPointerMapping(client)
    ClientPtr client;
{
    xGetPointerMappingReply rep;
    REQUEST(xReq);
    ButtonClassPtr butc = inputInfo.pointer->button;

    REQUEST_SIZE_MATCH(xReq);
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.nElts = butc->numButtons;
    rep.length = ((unsigned) rep.nElts + (4-1))/4;
    WriteReplyToClient(client, sizeof(xGetPointerMappingReply), &rep);
    (void)WriteToClient(client, (int)rep.nElts, (char *)&butc->map[1]);
    return Success;    
}

void
NoteLedState(keybd, led, on)
    DeviceIntPtr keybd;
    int		led;
    Bool	on;
{
    KeybdCtrl *ctrl = &keybd->kbdfeed->ctrl;
    if (on)
	ctrl->leds |= ((Leds)1 << (led - 1));
    else
	ctrl->leds &= ~((Leds)1 << (led - 1));
}

int
Ones(mask)                /* HACKMEM 169 */
    Mask mask;
{
    register Mask y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >>1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}

int
ProcChangeKeyboardControl (client)
    ClientPtr client;
{
#define DO_ALL    (-1)
    KeybdCtrl ctrl;
    DeviceIntPtr keybd = inputInfo.keyboard;
    XID *vlist;
    int t;
    int led = DO_ALL;
    int key = DO_ALL;
    BITS32 vmask, index;
    int mask, i;
    REQUEST(xChangeKeyboardControlReq);

    REQUEST_AT_LEAST_SIZE(xChangeKeyboardControlReq);
    vmask = stuff->mask;
    if (stuff->length !=(sizeof(xChangeKeyboardControlReq)>>2) + Ones(vmask))
	return BadLength;
    vlist = (XID *)&stuff[1];		/* first word of values */
    ctrl = keybd->kbdfeed->ctrl;
    while (vmask)
    {
	index = (BITS32) lowbit (vmask);
	vmask &= ~index;
	switch (index)
	{
	case KBKeyClickPercent: 
	    t = (INT8)*vlist;
	    vlist++;
	    if (t == -1)
		t = defaultKeyboardControl.click;
	    else if (t < 0 || t > 100)
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    ctrl.click = t;
	    break;
	case KBBellPercent:
	    t = (INT8)*vlist;
	    vlist++;
	    if (t == -1)
		t = defaultKeyboardControl.bell;
	    else if (t < 0 || t > 100)
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    ctrl.bell = t;
	    break;
	case KBBellPitch:
	    t = (INT16)*vlist;
	    vlist++;
	    if (t == -1)
		t = defaultKeyboardControl.bell_pitch;
	    else if (t < 0)
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    ctrl.bell_pitch = t;
	    break;
	case KBBellDuration:
	    t = (INT16)*vlist;
	    vlist++;
	    if (t == -1)
		t = defaultKeyboardControl.bell_duration;
	    else if (t < 0)
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    ctrl.bell_duration = t;
	    break;
	case KBLed:
	    led = (CARD8)*vlist;
	    vlist++;
	    if (led < 1 || led > 32)
	    {
		client->errorValue = led;
		return BadValue;
	    }
	    if (!(stuff->mask & KBLedMode))
		return BadMatch;
	    break;
	case KBLedMode:
	    t = (CARD8)*vlist;
	    vlist++;
	    if (t == LedModeOff)
	    {
		if (led == DO_ALL)
		    ctrl.leds = 0x0;
		else
		    ctrl.leds &= ~(((Leds)(1)) << (led - 1));
	    }
	    else if (t == LedModeOn)
	    {
		if (led == DO_ALL)
		    ctrl.leds = ~0L;
		else
		    ctrl.leds |= (((Leds)(1)) << (led - 1));
	    }
	    else
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    break;
	case KBKey:
	    key = (KeyCode)*vlist;
	    vlist++;
	    if (key < (int)(inputInfo.keyboard->key->curKeySyms.minKeyCode) ||
		key > (int)(inputInfo.keyboard->key->curKeySyms.maxKeyCode))
	    {
		client->errorValue = key;
		return BadValue;
	    }
	    if (!(stuff->mask & KBAutoRepeatMode))
		return BadMatch;
	    break;
	case KBAutoRepeatMode:
	    i = (key >> 3);
	    mask = (1 << (key & 7));
	    t = (CARD8)*vlist;
	    vlist++;
	    if (t == AutoRepeatModeOff)
	    {
		if (key == DO_ALL)
		    ctrl.autoRepeat = FALSE;
		else
		    ctrl.autoRepeats[i] &= ~mask;
	    }
	    else if (t == AutoRepeatModeOn)
	    {
		if (key == DO_ALL)
		    ctrl.autoRepeat = TRUE;
		else
		    ctrl.autoRepeats[i] |= mask;
	    }
	    else if (t == AutoRepeatModeDefault)
	    {
		if (key == DO_ALL)
		    ctrl.autoRepeat = defaultKeyboardControl.autoRepeat;
		else
		    ctrl.autoRepeats[i] =
			    (ctrl.autoRepeats[i] & ~mask) |
			    (defaultKeyboardControl.autoRepeats[i] & mask);
	    }
	    else
	    {
		client->errorValue = t;
		return BadValue;
	    }
	    break;
	default:
	    client->errorValue = stuff->mask;
	    return BadValue;
	}
    }
    keybd->kbdfeed->ctrl = ctrl;
    (*keybd->kbdfeed->CtrlProc)(keybd, &keybd->kbdfeed->ctrl);
    return Success;
#undef DO_ALL
} 

int
ProcGetKeyboardControl (client)
    ClientPtr client;
{
    int i;
    register KeybdCtrl *ctrl = &inputInfo.keyboard->kbdfeed->ctrl;
    xGetKeyboardControlReply rep;
    REQUEST(xReq);

    REQUEST_SIZE_MATCH(xReq);
    rep.type = X_Reply;
    rep.length = 5;
    rep.sequenceNumber = client->sequence;
    rep.globalAutoRepeat = ctrl->autoRepeat;
    rep.keyClickPercent = ctrl->click;
    rep.bellPercent = ctrl->bell;
    rep.bellPitch = ctrl->bell_pitch;
    rep.bellDuration = ctrl->bell_duration;
    rep.ledMask = ctrl->leds;
    for (i = 0; i < 32; i++)
	rep.map[i] = ctrl->autoRepeats[i];
    WriteReplyToClient(client, sizeof(xGetKeyboardControlReply), &rep);
    return Success;
} 

int
ProcBell(client)
    ClientPtr client;
{
    register DeviceIntPtr keybd = inputInfo.keyboard;
    int base = keybd->kbdfeed->ctrl.bell;
    int newpercent;
    REQUEST(xBellReq);
    REQUEST_SIZE_MATCH(xBellReq);
    if (stuff->percent < -100 || stuff->percent > 100)
    {
	client->errorValue = stuff->percent;
	return BadValue;
    }
    newpercent = (base * stuff->percent) / 100;
    if (stuff->percent < 0)
        newpercent = base + newpercent;
    else
    	newpercent = base - newpercent + stuff->percent;
    (*keybd->kbdfeed->BellProc)(newpercent, keybd);
    return Success;
} 

int
ProcChangePointerControl(client)
    ClientPtr client;
{
    DeviceIntPtr mouse = inputInfo.pointer;
    PtrCtrl ctrl;		/* might get BadValue part way through */
    REQUEST(xChangePointerControlReq);

    REQUEST_SIZE_MATCH(xChangePointerControlReq);
    ctrl = mouse->ptrfeed->ctrl;
    if ((stuff->doAccel != xTrue) && (stuff->doAccel != xFalse))
    {
	client->errorValue = stuff->doAccel;
	return(BadValue);
    }
    if ((stuff->doThresh != xTrue) && (stuff->doThresh != xFalse))
    {
	client->errorValue = stuff->doThresh;
	return(BadValue);
    }
    if (stuff->doAccel)
    {
	if (stuff->accelNum == -1)
	    ctrl.num = defaultPointerControl.num;
	else if (stuff->accelNum < 0)
	{
	    client->errorValue = stuff->accelNum;
	    return BadValue;
	}
	else ctrl.num = stuff->accelNum;
	if (stuff->accelDenum == -1)
	    ctrl.den = defaultPointerControl.den;
	else if (stuff->accelDenum <= 0)
	{
	    client->errorValue = stuff->accelDenum;
	    return BadValue;
	}
	else ctrl.den = stuff->accelDenum;
    }
    if (stuff->doThresh)
    {
	if (stuff->threshold == -1)
	    ctrl.threshold = defaultPointerControl.threshold;
	else if (stuff->threshold < 0)
	{
	    client->errorValue = stuff->threshold;
	    return BadValue;
	}
	else ctrl.threshold = stuff->threshold;
    }
    mouse->ptrfeed->ctrl = ctrl;
    (*mouse->ptrfeed->CtrlProc)(mouse, &mouse->ptrfeed->ctrl);
    return Success;
} 

int
ProcGetPointerControl(client)
    ClientPtr client;
{
    register PtrCtrl *ctrl = &inputInfo.pointer->ptrfeed->ctrl;
    REQUEST(xReq);
    xGetPointerControlReply rep;

    REQUEST_SIZE_MATCH(xReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.threshold = ctrl->threshold;
    rep.accelNumerator = ctrl->num;
    rep.accelDenominator = ctrl->den;
    WriteReplyToClient(client, sizeof(xGenericReply), &rep);
    return Success;
}

void
MaybeStopHint(dev, client)
    register DeviceIntPtr dev;
    ClientPtr client;
{
    GrabPtr grab = dev->grab;

    if ((grab && SameClient(grab, client) &&
	 ((grab->eventMask & PointerMotionHintMask) ||
	  (grab->ownerEvents &&
	   (EventMaskForClient(dev->valuator->motionHintWindow, client) &
	    PointerMotionHintMask)))) ||
	(!grab &&
	 (EventMaskForClient(dev->valuator->motionHintWindow, client) &
	  PointerMotionHintMask)))
	dev->valuator->motionHintWindow = NullWindow;
}

int
ProcGetMotionEvents(client)
    ClientPtr client;
{
    WindowPtr pWin;
    xTimecoord * coords = (xTimecoord *) NULL;
    xGetMotionEventsReply rep;
    int     i, count, xmin, xmax, ymin, ymax;
    unsigned long nEvents;
    DeviceIntPtr mouse = inputInfo.pointer;
    TimeStamp start, stop;
    REQUEST(xGetMotionEventsReq);

    REQUEST_SIZE_MATCH(xGetMotionEventsReq);
    pWin = LookupWindow(stuff->window, client);
    if (!pWin)
	return BadWindow;
    if (mouse->valuator->motionHintWindow)
	MaybeStopHint(mouse, client);
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    nEvents = 0;
    start = ClientTimeToServerTime(stuff->start);
    stop = ClientTimeToServerTime(stuff->stop);
    if ((CompareTimeStamps(start, stop) != LATER) &&
	(CompareTimeStamps(start, currentTime) != LATER) &&
	mouse->valuator->numMotionEvents)
    {
	if (CompareTimeStamps(stop, currentTime) == LATER)
	    stop = currentTime;
	coords = (xTimecoord *)ALLOCATE_LOCAL(mouse->valuator->numMotionEvents
					      * sizeof(xTimecoord));
	if (!coords)
	    return BadAlloc;
	count = (*mouse->valuator->GetMotionProc) (mouse, coords,
						   start.milliseconds,
						   stop.milliseconds,
						   pWin->drawable.pScreen);
	xmin = pWin->drawable.x - wBorderWidth (pWin);
	xmax = pWin->drawable.x + (int)pWin->drawable.width +
		wBorderWidth (pWin);
	ymin = pWin->drawable.y - wBorderWidth (pWin);
	ymax = pWin->drawable.y + (int)pWin->drawable.height +
		wBorderWidth (pWin);
	for (i = 0; i < count; i++)
	    if ((xmin <= coords[i].x) && (coords[i].x < xmax) &&
		    (ymin <= coords[i].y) && (coords[i].y < ymax))
	    {
		coords[nEvents].time = coords[i].time;
		coords[nEvents].x = coords[i].x - pWin->drawable.x;
		coords[nEvents].y = coords[i].y - pWin->drawable.y;
		nEvents++;
	    }
    }
    rep.length = nEvents * (sizeof(xTimecoord) >> 2);
    rep.nEvents = nEvents;
    WriteReplyToClient(client, sizeof(xGetMotionEventsReply), &rep);
    if (nEvents)
    {
	client->pSwapReplyFunc = SwapTimeCoordWrite;
	WriteSwappedDataToClient(client, nEvents * sizeof(xTimecoord),
				 (char *)coords);
    }
    if (coords)
	DEALLOCATE_LOCAL(coords);
    return Success;
}

int
ProcQueryKeymap(client)
    ClientPtr client;
{
    xQueryKeymapReply rep;
    int i;
    CARD8 *down = inputInfo.keyboard->key->down;
    REQUEST(xReq); /* added */

    REQUEST_SIZE_MATCH(xReq); /* added */
    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 2;
    for (i = 0; i<32; i++)
	rep.map[i] = down[i];
    WriteReplyToClient(client, sizeof(xQueryKeymapReply), &rep);
    return Success;
}

