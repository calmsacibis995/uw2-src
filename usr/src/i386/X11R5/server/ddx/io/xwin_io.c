/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:ddx/io/xwin_io.c	1.5.1.26"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

			All Rights Reserved
******************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#ifdef SVR4
#include <sys/stream.h>
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif /* SVR4 */
#include <signal.h>
#include <sys/param.h>
#include <sys/time.h>

#include "X.h"
#define	 NEED_EVENTS
#include "Xproto.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmap.h"
#include "input.h"
#include "inputstr.h"
#include "windowstr.h"
#include "regionstr.h"
#include "resource.h"
#include "keysym.h"

/* #include "att.h" */

#include "xwin_io.h"
#include "keynames.h"
#include "osdep.h"

#define EVC
#include <sys/mouse.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/ascii.h>
#include <sys/kd.h>
#include <sys/xque.h>
#include <setjmp.h>
#include <termio.h>

#ifdef XTESTEXT1
#define XTestSERVER_SIDE
#include "xtestqueue.h"
#include "xtestext1.h"
extern xqEvent		*XTestDeQueue();
extern int		on_steal_input;
extern Bool		XTestStealKeyData();
extern void		XTestStealMotionData();
static Bool		XTestKernelQueueFlag;

XTestQueue	xTestQ = { (XTestQueueNode *)0,
			   (XTestQueueNode *)0 };
#endif

#include "siconfig.h"

#define E0      0xe0
#define E1      0xe1
#define MOTION_BUFFER_SIZE	0	
/*
 *  Following four lines declare the corresponding functions as static
 *  to avoid -Xc warning message
 */
static int  	i386GetMotionEvents();
static void 	i386ChangePointerControl(),
		i386ChangeKeyboardControl(),
		i386Bell();

/*
 * LED_KANA is not defined in pre-esmp kd.h; it shouldn't harm if we define
 * it here; because we should never get this code in pre-esmp env's
 */
#ifdef NOT_ESMP
#define LED_KANA	0x08
#endif

unchar		ledsdown = 0;			/* LED keys that are down */
static unchar	ledsup =			/* Initially all up */
    LED_NUM|LED_CAP|LED_SCR|LED_KANA;

xqEventQueue *		queue;			/* global mouse & kbd queue */
unsigned long		lastEventTime = 0;
Bool			input_init_time = 1;
int			qLimit; 
static DevicePtr	i386Keyboard;
static DevicePtr	i386Pointer;

#define I386KeyboardState       (((DeviceIntPtr)i386Keyboard)->key->state)

/* XTESTEXT1 Macros **/
#ifdef XTESTEXT1
#define XTEST_STEALKEYDATA(Action) { \
	if (on_steal_input)				\
		XTestStealKeyData( xtest_map_key( key ), \
		Action, 0, mouse->x, mouse->y); \
}

#define XTEST_MOTIONDATA1() \
	if ( !on_steal_input || XTestStealKeyData( x.u.u.detail, \
		x.u.u.type, 0, mouse->x, mouse->y ))

#define XTEST_STEALMOTIONDATA() { \
	if ( on_steal_input ) \
	    XTestStealMotionData( mouse->x - lastmouse.x, \
		mouse->y - lastmouse.y, 0, lastmouse.x, lastmouse.y ); \
}
#else
#define XTEST_STEALKEYDATA(Action)
#define XTEST_MOTIONDATA1()
#define XTEST_STEALMOTIONDATA()
#endif

#define SETBUTTONINFO(button_bit, value) \
	if (changes&button_bit) { \
	    if (current_state&button_bit) \
		x.u.u.type = ButtonRelease; \
	    else \
		x.u.u.type = ButtonPress; \
					  \
	    x.u.u.detail = value; \
	    XTEST_MOTIONDATA1() \
/*	        (*i386Pointer->processInputProc)(&x, i386Pointer, 1);*/ \
	        mieqEnqueue(&x); \
	}
/* XTESTEXT1 Macros **/

xqCursorPtr	mouse = (xqCursorPtr)NULL;  /* current mouse position */
static ushort	length, tone;		/* For kd bell (initialized via DIX) */
char 		*blackValue = NULL,
		*whiteValue = NULL;

/*
 * This table is used to translate keyboard scan codes to ASCII character
 * sequences for the AT386 keyboard/display driver.  It is the default table,
 * and may be changed with system calls.
 */

/* Define key remappings using KEY to index into new mapping */

#define KEY(i) KeyMap.key[i].map[1]

keymap_t KeyMap;	/* This keymap will contain possible changed */
			/* keyboard mapping to be compared with */
			/* the default keyboard map (below). */
#include "keytable.h"

#include "../si/mipointer.h"

static Bool i386CursorOffScreen();
static void i386CrossScreen();
static void i386WarpCursor();

miPointerScreenFuncRec siPointerScreenFuncs = {
    i386CursorOffScreen,
    i386CrossScreen,
    i386WarpCursor,
};

unsigned char
GetKbdLeds()
{
    unchar	c;
    extern int	condev;			/* fd of the console */

    if(ioctl(condev, KDGETLED, &c) < 0){
	ErrorF("ioctl(KDGETLED) failed\n");
	return(0);
    }
    return(c);
}

void
SetKbdLeds(ledson)
unchar	ledson;
{
    extern int	condev;			/* fd of the console */

    if(ioctl(condev, KDSETLED, ledson) < 0)
	ErrorF("ioctl(KDSETLED) failed\n");
}

void
RefreshLeds()
{
	SetKbdLeds( ledsdown );
}

/* ARGSUSED */
int
i386MouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    BYTE map[4];

    i386Pointer = pDev;

    switch (onoff)
    {
	case DEVICE_INIT: 
	    /* Make sure there is only one pointer device at this time */
	    if (pDev != LookupPointerDevice()) {
		ErrorF ("Cannot open non-system mouse\n");
		return (!Success);
	    }

	    /*
	     * The only private data will be the screen ptr
	     * for now (use screens[0]?).
	     */
	    pDev->devicePrivate = (pointer) screenInfo.screens[0]; 
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pDev, map, 3, i386GetMotionEvents,
		i386ChangePointerControl, MOTION_BUFFER_SIZE);

	    SetInputCheck((long *)&queue->xq_head, (long *)&queue->xq_tail);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    /*
	     * We use shared memory for our mouse and keyboard devices,
	     * not selectable file descriptors...
	     * AddEnabledDevice(indev);
	     */
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

/*ARGSUSED*/
static Bool
i386CursorOffScreen(ppScreen, x, y)
  ScreenPtr	*ppScreen;
  int		*x, *y;
{
    int index;
    int ActiveZaphod=1;

    if (ActiveZaphod &&
	siNumScreens > 1 && (*x >= (*ppScreen)->width || *x < 0)) {
	index = (*ppScreen)->myNum;
	if (*x < 0) {
	    index = (index ? index : screenInfo.numScreens) - 1;
	    *ppScreen = screenInfo.screens[index];
	    *x += (*ppScreen)->width;
	} else {
	    *x -= (*ppScreen)->width;
	    index = (index + 1) % screenInfo.numScreens;
	    *ppScreen = screenInfo.screens[index];
	}
	return TRUE;
    }
    return FALSE;			/* do anything here ?? */
}

static void
i386CrossScreen(pScreen, entering)
  ScreenPtr pScreen;
  Bool      entering;
{
    int select;
    si_currentScreen();

    select = 1;
    if (entering)
      select = 0;

    if (xwin_verbose_level == 1)
   	ErrorF("i386CrossScreen(%d, %s)\n",
	   pScreen->myNum, entering ? "entering":"leaving");

    si_enterleavescreen(pScreen->myNum, select);
}

static void
i386WarpCursor (pScreen, x, y)
    ScreenPtr	pScreen;
    int		x, y;
{
    int oldenable;

    oldenable = queue->xq_sigenable;
    queue->xq_sigenable = 0;
    miPointerWarpCursor (pScreen, x, y);
    queue->xq_sigenable = oldenable;
}

/* ARGSUSED */
static void
i386ChangePointerControl(pDevice, ctrl)
    DevicePtr pDevice;
    PtrCtrl   *ctrl;
{
    /* We don't support changeable mouse parameters (i.e., acceleration) */
}

/* ARGSUSED */
static int
i386GetMotionEvents(buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    /* We don't support a motion buffer */
    return 0;
}

/**************************************************************************
 * catch_xsig- Signal catching function for xq signals (mouse & keyboard).
 * 
 * If we were in WaitForSomething(), select()ing for a non-zero amount of
 * time, then abort the sleeping by jumping back to the top of Dispatch().
 * Otherwise, delivery of this signal will wake up select with EINTR.
 *
 * Since we're making a longjmp from a singal handler, siglongjmp should be
 * used below instead of a longjmp.  We don't do this because:
 *   1	the longjmp may be taken from elsewhere other than a signal handler
 *	(this is true for BUILTIN, for example).
 *   2	we may not want the proc's signal mask saved in the setjmp to be
 *	restored (the proc's signal mask may have changed).
 */
#ifdef BUILTIN
#include <ucontext.h>
#include <siginfo.h>
#include "extensions/server/builtin/BIserver.h"

void
catch_xsig(int sig, siginfo_t *info, ucontext_t *uc)
#else
void
catch_xsig(int sig)
#endif
{
	extern jmp_buf startagain;
#ifdef DEBUG
    if (xwin_verbose_level == 1)
	    ErrorF("signal %d received\n", sig);
#endif
#ifdef BUILTIN
	if (!IN_NORMAL_SERVER_MODE())
	{
	    /* When server is running builtin client(s), can't take long jump
	     * while client code is on the stack.  Instead, wake up select by
	     * making data avail on dummy fd.
	     */
	    /* Check to see if the PC was in poll() when we were signalled */
	    BIGlobal.last_poll_addr = uc->uc_mcontext.gregs[R_EIP];
	    if ((BIGlobal.poll_addr == 0) ||
		(BIGlobal.poll_addr != BIGlobal.last_poll_addr))
		write(BIGlobal.dummy_pipe, &sig/*junk*/, 1);
	}
	else
#endif
	{
	    queue->xq_sigenable = 0;	/* since not returning to select */
	    /* setcontext(); */
	    longjmp(startagain, /*arbitrary*/ 1);
	}
}

Bool SpecialNumLock = FALSE;
Bool SpecialShiftLock = FALSE;
Bool AXKeyboard = FALSE;
Bool A01Keyboard = FALSE;

/* ARGSUSED */
int
i386KeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    extern int	condev;			/* fd of the console */
    extern int	xsig;			/* signal sent by xq driver */
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];
    char *env;
    struct sigaction st;


    switch (onoff)
    {
	case DEVICE_INIT: 
	    i386Keyboard = pDev;
	    GetI386Mappings( &keySyms, modMap);

	    /***** MR cs93-09609 ******/
	    /*
	     * If a env variable, NUMLOCK != NULL, then don't make NumLock a modifier.
	     * Also, see xwin_io.c file. 
	     * This change is made to solve the issues related to European keyboards.
	     * For a good description of the nature of the problem, see the description
	     * in the famous MR:  cs93-09609
	     */ 
	    if ( (env = (char *)getenv("NUMLOCK")) != NULL )
	    {
		SpecialNumLock = TRUE;
		printf ("NUMLOCK - Special Interpretation.\n");
	    }

	    /***** MR cs93-09609 ******/

	    if ( (env = (char *)getenv("XKEYBOARD")) != NULL )
	    {
		if(!strcmp(env, SKB_AX)) {	
			AXKeyboard = TRUE;
			printf ("AX KEYBOARD Installed.\n");
		}
		if(!strcmp(env, SKB_A01)) {	
			A01Keyboard = TRUE;
			printf ("A01 KEYBOARD Installed.\n");
		}
	    }
	    if ( (env = (char *)getenv("SHIFTDOWN")) != NULL )
	    {
		SpecialShiftLock = TRUE;
		printf ("SHIFTDOWN - Special Interpretation.\n");
	    }

	    InitKeyboardDeviceStruct(
		    i386Keyboard, &keySyms, modMap, i386Bell,
		    i386ChangeKeyboardControl);
	    xfree((pointer)keySyms.map);

	    /* Set sig handler for xq events/signals.  With builtin clients,
	     * additional info is needed to avoid making the longjmp in
	     * catch_xsig.  The code is #ifdef'ed here (rather than
	     * re-setting the signal disposition in the builtin extension) so
	     * that developers are not blind-sided.
	     */
	    st.sa_handler = catch_xsig;
	    sigemptyset(&st.sa_mask);
#ifdef BUILTIN
	    st.sa_flags = SA_SIGINFO|SA_NODEFER;
#else
	    st.sa_flags = SA_NODEFER;
#endif
	    sigaction(xsig, &st, NULL);
	    break;

	case DEVICE_ON: 
	    pDev->on = TRUE;
	    /*
	     * We use shared memory for our mouse and keyboard devices,
	     * not selectable file descriptors...
	     * AddEnabledDevice(condev);
	     */
	    break;
	case DEVICE_OFF: 
	    /* NOTE: This case doesn't seem to be used anywhere in the server*/
	    pDev->on = FALSE;
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

/* static void */
void
GetKDKeyMap (fd)
int fd;			/* fd for open ("console") */
{
    int i, j, k;

    KeyMap.n_keys = NUM_KEYS;
    if (ioctl (fd, GIO_KEYMAP, &KeyMap) == -1) {
	/* ioctl failed, use default map */
	for (i=0; i<KeyMap.n_keys; i++) {
	    KeyMap.key[i].map[0] = KDKeyMap.key[i].map[0];
	    /* Use second element of map to store key index value */
	    KeyMap.key[i].map[1] = i;
	}
	return;
    }
    /* Look to see if the map the kernal gives us is the same as */
    /* the default keyboard map.  For every entry that is the same */
    /* save the index to this location in the second map element. */
    /* If values are different search for the new value in the */
    /* map and store the index to this location in the new map. */

    for (i=0; i<KeyMap.n_keys; i++) {
	/* Check to see if key in new key map is same as */
	/* the one in the default map. */
	if (KeyMap.key[i].spcl == KDKeyMap.key[i].spcl) {
	    for (k=0; k<NUM_STATES; k++) {
		if (KeyMap.key[i].map[k] != KDKeyMap.key[i].map[k]) {
		    break;
		}
	    }
	    if (k == NUM_STATES) {
		/* Keys are the same */
	        KeyMap.key[i].map[1] = i;
		continue;
	    }
	}
	/* Key has been remapped */
	for (j=0; j<KeyMap.n_keys; j++) {
	    if (KeyMap.key[i].spcl == KDKeyMap.key[j].spcl) {
		for (k=0; k<NUM_STATES; k++) {
		    if (KeyMap.key[i].map[k] != KDKeyMap.key[j].map[k]) {
			break;
		    }
		}
		if (k == NUM_STATES) {
		    /*
 		    * this is to cover the strange situation where the new table
 		    * isn't moving the key, as the above test will find, but rather
 		    * adding in a key with the same mappings. In this case we don't
 		    * want to map to the key slot of the old but rather use the new.
 		    * this is for two KANA keys that at the driver level are defined
 		    * the same but at the X level must make different keysyms. 
 		    */

		    if (KeyMap.key[j].map[1] == j) {
			continue;
		    }
		    /* This key matches the new key. */
		    KeyMap.key[i].map[1] = j;
		    break;
		}
	    }
	}

	if (j == KeyMap.n_keys) {	/* No match, don't remap */
	    KeyMap.key[i].map[1] = i;
        }
    }
}

static int
TranslateE0Keycodes (key, x)
BYTE *key;
xEvent *x;
{

    switch (*key) {
	case K_Shift_L:
	case K_Shift_R: {
            /* Ignore E0 {left,right} shift */
            return FALSE;
	}
	case K_Enter: {
	    *key = x->u.u.detail = KEY(K_KP_Enter);
	    break;
	}
	case K_Control_L: {
	    *key = x->u.u.detail = KEY(K_Control_R);
	    break;
	}
	case K_Alt_L: {
	    *key = x->u.u.detail = KEY(K_Alt_R);
	    break;
	}
        case K_Scroll_Lock: {
            *key = x->u.u.detail = KEY(K_Pause);
            break;
	}
        case K_slash: {
            *key = x->u.u.detail = KEY(K_KP_Divide);
            break;
	}
        case K_Down: {
            *key = x->u.u.detail = KEY(K_ExDown);
            break;
	}
        case K_Right: {
            *key = x->u.u.detail = KEY(K_ExRight);
            break;
	}
        case K_Left: {
            *key = x->u.u.detail = KEY(K_ExLeft);
            break;
	}
        case K_Up: {
            *key = x->u.u.detail = KEY(K_ExUp);
            break;
	}
        case K_Insert: {
            *key = x->u.u.detail = KEY(K_Ex_Insert);
            break;
	}
        case K_Home: {
            *key = x->u.u.detail = KEY(K_Ex_Home);
            break;
	}
        case K_Prior: {
            *key = x->u.u.detail = KEY(K_Ex_Prior);
	    break;
        }
        case K_Delete: {
            *key = x->u.u.detail = KEY(K_Ex_Delete);
            break;
	}
        case K_End: {
            *key = x->u.u.detail = KEY(K_Ex_End);
            break;
	}
        case K_Next: {
            *key = x->u.u.detail = KEY(K_Ex_Next);
            break;
	}
        case K_KP_Multiply: {
            *key = x->u.u.detail = KEY(K_Ex_Print);
            break;
	}
    }
    return TRUE;
}

static int
TranslateKeycodes (key, x, pE)
BYTE *key;
xEvent *x;
xqEvent *pE;
{
    static BYTE lastKeycode = 0;	/* last key encountered */

    if (pE->xq_code == E0) {
	lastKeycode = E0;		/* Indicate E0 encountered */
	return FALSE;
    }
    else if (pE->xq_code == E1) {
	lastKeycode = E1;		/* Indicate E1 encountered */
	return FALSE;
    }
    switch (lastKeycode) {
	case E0: {
	    lastKeycode = 0;
	    if (TranslateE0Keycodes (key, x) == FALSE) {
		return FALSE;
	    }
	    return TRUE;
	}
	case E1: {		/* Process E1 */
	    lastKeycode = pE->xq_code&0177;
	    return FALSE;
	}
	case K_Control_L: {
	    lastKeycode = 0;
	    if (*key != K_Num_Lock) {
		return FALSE;
	    }
	    *key = x->u.u.detail = KEY(K_Pause);
	    return TRUE;
	}
	default: {
		if (A01Keyboard) {
                    switch (*key) {
                        case K_Caps_Lock:
                            if ((((DeviceIntPtr)i386Keyboard)->key->state & ShiftMask)  ||
				((x->u.u.type == KeyRelease) &&
				( (((ledsup & LED_CAP) == 0) && ((ledsdown & LED_CAP) == 0)) ||
				((ledsup & LED_CAP) && (ledsdown & LED_CAP))))) {
		
                                *key = x->u.u.detail = K_Caps_Lock;      
			    }
			    else {
                                *key = x->u.u.detail = K_A01_Eisu;      /* 93*/
			    }
                            break;
                        case K_ExRight:                 /*125*/
                            *key = x->u.u.detail = K_A01_yen;           /* 92*/
                            break;
                        case K_Control_R:               /*115*/
                            *key = x->u.u.detail = K_A01_backslash;     /* 86*/
                            break;
                        case K_Ex_Insert:               /*123*/
                            *key = x->u.u.detail = K_A01_Muhenkan;      /* 90*/
                            break;
                        case K_Ex_Delete:               /*121*/
                            *key = x->u.u.detail = K_A01_Henkan;        /* 91*/
                            break;
                    }
		}
	    	else {
	    		*key = x->u.u.detail = KEY(*key);
	    		lastKeycode = 0;
	    		return TRUE;
		}
            }
    }
}

/*
 *	Assuming all mouse buttons were up when we started 
 */

#define LEFT_BUTTON_BIT		4	
#define MIDDLE_BUTTON_BIT	2	
#define RIGHT_BUTTON_BIT	1

#define TVTOMILLI(tv)	(((tv).tv_usec/1000)+((tv).tv_sec*1000))

long
GetTimeInMillis()
{
    struct timeval now;

    if (queue && (queue->xq_head != queue->xq_tail))
	return(queue->xq_curtime);

    /* provide accurate time until queue is ready */
    gettimeofday(&now,(struct timezone *)0);
    return(TVTOMILLI(now));
}

extern int screenIsSaved;

#ifdef XTESTEXT1
static ushort	mouse_state = 07;
#endif

siSendKeybdEvents(key)
  unsigned int key;
{
    SIScreenRecP pSavedScreen;
    int ii, stealInput=0;
    static int secureKeyboard=-1;
    si_currentScreen();

    /*
     * if a DM needs(or can process key events) pass them on to the DM
     */
    /*
     * NOTE: By calling this function here every key stroke including
     * PRESS/RELEASE will be passed on to the DM - I am not sure if this
     * is what we want - may be we should pass the whole structure instead
     * of only a BYTE, so that the DM will know if it is a PRESS/RELEASE
     * OR should we pass only the PRESS key's ?
     *  - MUST RESOLVE THIS BEFORE APRIL 13, 1993
     */

    if (secureKeyboard == -1) {
	if (getenv("SECURE_KEYBOARD") != NULL)
	  secureKeyboard = 1;
	else
	  secureKeyboard = 0;
    } else if (secureKeyboard) {
	return(0);
    }

    pSavedScreen = global_pSIScreen;
    for (ii=0; ii < siNumScreens; ++ii) {
	global_pSIScreen = pSIScreen = &siScreens[ii];
	if (si_haskeybdevents) {
	    if (si_ProcKeybdEvent(key) == 1)
	      stealInput = 1;
	}
    }
    global_pSIScreen = pSavedScreen;

#if 0
    /*
     * XXX:md - we would like to be able to let the SDD divert
     * keyboard input for its own use (monitor setup, etc.)  However,
     * we will let it all pass-thru until we understand the security
     * ramifications.
     */
    return(stealInput);
#else
    return(0);
#endif
}			

Bool NumLockOn = FALSE;
Bool KanaLockOn = FALSE;

#define KeyPadKey(key)\
	((key==K_Home) || (key==K_Up) || (key==K_Prior)\
		|| (key==K_Left) || (key==K_Begin) || (key==K_Right)\
		|| (key==K_End) || (key==K_Down) || (key==K_Next)\
		|| (key==K_Insert) || (key==K_Delete) )

#define ModifierKey(key)\
	( (key==K_Shift_L) || (key==K_Shift_R)\
		|| (key==K_Num_Lock)\
		|| (key==K_Control_L) || (key==K_Control_R)\
		|| (key==K_Caps_Lock)\
		|| (key==K_Scroll_Lock)\
		|| (key==K_Kana_Lock)\
		|| (key==K_A01_Dummy_Kana_Lock)\
		|| (key==K_Alt_L) || (key==K_Alt_R) )

#define KeyPressed(k) (keyc->down[k >> 3] & (1 << (k & 7)))
#define ModifierDown(k) ((keyc->state & (k)) == (k))

void
i386ProcessInputEvents()
{
#ifndef XTESTEXT1
    static ushort	mouse_state = 07;
#endif

    register ushort	current_state, changes;
    xqEvent		*pE;
    xEvent		x;
    extern long		ClientsBlocked[];
    BYTE 		key;
    KeyClassRec *keyc = ((DeviceIntPtr)i386Keyboard)->key;
    Bool		fakedMask = FALSE;
    Bool		ModifierFlg = FALSE;
    unsigned int	save_state;

    for (;;)
    {
	xqCursorRec	lastmouse;	/* the previous mouse position */

/*
	if (ANYSET(ClientsBlocked))
		return;
*/
    {
#ifdef XTESTEXT1
	if ((queue->xq_head == queue->xq_tail) && XTestQueueEmpty( xTestQ ))
#else
	if ((queue->xq_head == queue->xq_tail))
#endif
	    break;
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);

#ifdef	XTESTEXT1
	if ( XTestQueueEmpty( xTestQ ) || (queue->xq_head != queue->xq_tail)
	    && queue->xq_events[queue->xq_head].xq_time
	    < xTestQ.head->xqevent.xq_time )
	{
	    XTestKernelQueueFlag = TRUE;
	    pE = &queue->xq_events[queue->xq_head];
	}
	else
	    pE = XTestDeQueue();
#else
	pE = &queue->xq_events[queue->xq_head];
#endif
    }
	if(queue->xq_curtime <= pE->xq_time)
		queue->xq_curtime = pE->xq_time+1;

	x.u.keyButtonPointer.time = lastEventTime = pE->xq_time;

	switch (pE->xq_type) {

	  case XQ_KEY:
	    {			/* better be a key */

		x.u.keyButtonPointer.rootX = mouse->x; /* necessary? */
		x.u.keyButtonPointer.rootY = mouse->y; /* necessary? */
		key = x.u.u.detail = pE->xq_code&0177;

		if (siSendKeybdEvents(pE->xq_code) == 1)
		  goto EndOfLoop;

		if (pE->xq_code&0200) {
		    x.u.u.type = KeyRelease;
                    if (TranslateKeycodes (&key, &x, pE) == FALSE) {
			XTEST_STEALKEYDATA(KeyRelease);
                        goto EndOfLoop;
                    }
		    if (key == K_Num_Lock) {
			if ((ledsup&LED_NUM) == 0) {
			    ledsup	 |= LED_NUM;
			    ledsdown &= ~LED_NUM;
			}
			else {
			    ledsup &= ~LED_NUM;
			    XTEST_STEALKEYDATA(KeyRelease);
			    goto EndOfLoop;
			}
		    }
		    else if(key == K_Caps_Lock){
			if((ledsup&LED_CAP) == 0){
			    ledsup	 |= LED_CAP;
			    ledsdown &= ~LED_CAP;
			}
			else {
			    ledsup &= ~LED_CAP;
			    XTEST_STEALKEYDATA(KeyRelease);
			    goto EndOfLoop;
			}
		    }
		      else if(key == K_Scroll_Lock){
		          if((ledsup&LED_SCR) == 0){
		              ledsup	 |= LED_SCR;
		              ledsdown &= ~LED_SCR;
		          }
			  else {
			      ledsup &= ~LED_SCR;
			      XTEST_STEALKEYDATA(KeyRelease);
			      goto EndOfLoop;
			  }
		      }
	
		    else if(AXKeyboard && (key == K_Kana_Lock)){ 
			if((((ledsup & LED_KANA) == 0)) && ((ledsdown & LED_KANA) == 0)){
			    ledsup	 |= LED_KANA;
			    ledsdown &= ~LED_KANA;
			}
			else {
			    /* 
			     * May get extraenous Kana_Lock 'release' it
			     * should be ignored and not change any state)
			     */
			    if(ledsdown&LED_KANA ) {	
			    	ledsup &= ~LED_KANA;
			    }
			    XTEST_STEALKEYDATA(KeyRelease);
			    goto EndOfLoop;
			}
		    }
		    else if(A01Keyboard && (key == K_A01_Hiragana)) {
			/*
			 * for the A01 can't really check mode bits here 
			 * because because the user may have released the
			 * alt or ctrl, shift before letting up on the
			 * hiragana key. In that case we use the fact that
			 * the Keypress portion already caught this and
			 * set down and up = 0. So when we get a hiragana
			 * release in this state assume it is to let up 
			 * the lock
			 */

			if((((ledsup & LED_KANA) == 0)) && ((ledsdown & LED_KANA) == 0)){
			    ledsup	 |= LED_KANA;
			    ledsdown &= ~LED_KANA;
                            key = x.u.u.detail = K_A01_Dummy_Kana_Lock;
			}
			else {
			    if(ledsdown&LED_KANA ) {	
			    	ledsup &= ~LED_KANA;
			    }
			    /* 
			     * only want to ignore the release if this was
			     * a Hiragana release for going into kanamode
			     * otherwise send up release as hiragana release
			     */
			    if((((ledsup & LED_KANA) == 1)) && ((ledsdown & LED_KANA) == 1)){
	
			    	XTEST_STEALKEYDATA(KeyRelease);
			    	goto EndOfLoop;
			    }
			}
		   }
#ifdef XTESTEXT1
		    if ( !on_steal_input
			|| XTestStealKeyData( xtest_map_key( key ),
					     KeyRelease, 0,
					     mouse->x, mouse->y ))
#endif
			fakedMask = FALSE;

			/* in KANA, K_Alt_R = Henkan_Mode and is not modifier*/
			if ( ModifierKey(key)) {
				if(AXKeyboard && (key == K_Alt_R)) 
					ModifierFlg = FALSE;
				else
					ModifierFlg = TRUE;
			}
			else
				ModifierFlg = FALSE;

			if (SpecialNumLock && NumLockOn && !ModifierFlg)
			{
			    if (KeyPadKey(key))
			    {
				if (keyc->state & ShiftMask)
				{
					save_state = keyc->state;
					keyc->state = keyc->state & ~(Mod2Mask|ShiftMask); 
					fakedMask = TRUE;
				}
			    }
			    else
			    {
				save_state = keyc->state;
				keyc->state = keyc->state & ~Mod2Mask; 
				fakedMask = TRUE;
			    }
			}


			if(SpecialShiftLock && !ModifierFlg &&
			    (keyc->lock_meaning == XK_Shift_Lock) &&
			    ((keyc->state & (ShiftMask|LockMask|Mod1Mask)) ==
				(ShiftMask|LockMask))) {
					if(!fakedMask) {
						save_state = keyc->state;
						fakedMask = TRUE;
					}
					keyc->state = keyc->state & ~(ShiftMask|LockMask); 
			}
					

			if (KanaLockOn && KeyPadKey(key)) {
				if(!fakedMask) {
					save_state = keyc->state;
					fakedMask = TRUE;
				}
				keyc->state = keyc->state & ~Mod1Mask; 
			}

			ProcessI386Input(&x, i386Keyboard);
			if (fakedMask)
				keyc->state = save_state;

			if(A01Keyboard)
                        	RefreshLeds();
		}
		else {
		    x.u.u.type = KeyPress;
                    if (TranslateKeycodes (&key, &x, pE) == FALSE) {
			XTEST_STEALKEYDATA(KeyRelease);
                        goto EndOfLoop;
                    }

		    if(key == K_Num_Lock){
			NumLockOn = !NumLockOn;	/* toggle the switch */
			if((ledsdown&LED_NUM) == 0)
			    ledsdown |= LED_NUM;
			else
			{
			    XTEST_STEALKEYDATA(KeyPress);
			    goto EndOfLoop;
			}
		    }
		    else if(key == K_Caps_Lock){
			if((ledsdown&LED_CAP) == 0)
			    ledsdown |= LED_CAP;
			else
			{
			    ledsdown &= ~LED_CAP;
			    XTEST_STEALKEYDATA(KeyPress);
			    goto EndOfLoop;
			}
		    }
		      else if(key == K_Scroll_Lock){
			  if((ledsdown&LED_SCR) == 0)
			      ledsdown |= LED_SCR;
			  else {
			      XTEST_STEALKEYDATA(KeyPress);
			      goto EndOfLoop;
			  }
		      }
		    else if((AXKeyboard && (key == K_Kana_Lock)) || 
			(A01Keyboard && (key == K_A01_Hiragana) &&
			(((I386KeyboardState & (ShiftMask|ControlMask|Mod3Mask)) ==
			(ShiftMask|ControlMask)) || 
			((I386KeyboardState & (ShiftMask|ControlMask|Mod3Mask)) ==
			Mod3Mask )))) {

			if(A01Keyboard)
                            key = x.u.u.detail = K_A01_Dummy_Kana_Lock;
			KanaLockOn = !KanaLockOn;	/* toggle the switch */
			if((ledsdown&LED_KANA) == 0)
			    ledsdown |= LED_KANA;
			else
			{
			    if(ledsdown&LED_KANA)
			   	ledsdown &= ~LED_KANA;
			    XTEST_STEALKEYDATA(KeyPress);
			    goto EndOfLoop;
			}
		    }
	
#ifdef XTESTEXT1
		    if ( !on_steal_input
			|| XTestStealKeyData( xtest_map_key( key ),
					     KeyPress, 0,
					     mouse->x, mouse->y ))
#endif

			fakedMask = FALSE;
			/* in KANA, K_Alt_R = Henkan_Mode and is not modifier*/
			if ( ModifierKey(key)) {
				if(AXKeyboard && (key == K_Alt_R)) 
					ModifierFlg = FALSE;
				else
					ModifierFlg = TRUE;
			}
			else
				ModifierFlg = FALSE;

			if (SpecialNumLock && NumLockOn && !ModifierFlg)
			{
			    if (KeyPadKey(key))
			    {
				if (keyc->state & ShiftMask)
				{
					save_state = keyc->state;
					keyc->state = keyc->state & ~(Mod2Mask|ShiftMask); 
					fakedMask = TRUE;
				}
			    }
			    else
			    {
				save_state = keyc->state;
				keyc->state = keyc->state & ~Mod2Mask; 
				fakedMask = TRUE;
			    }
			}

			if(SpecialShiftLock && !ModifierFlg &&
			    (keyc->lock_meaning == XK_Shift_Lock) &&
			    ((keyc->state & (ShiftMask|LockMask|Mod1Mask)) ==
				(ShiftMask|LockMask))) {
					if(!fakedMask) {
						save_state = keyc->state;
						fakedMask = TRUE;
					}
					keyc->state = keyc->state & ~(ShiftMask|LockMask); 
			}
					

			if (KanaLockOn && KeyPadKey(key)) {
				if(!fakedMask) {
					save_state = keyc->state;
					fakedMask = TRUE;
				}
				keyc->state = keyc->state & ~Mod1Mask; 
			}

			ProcessI386Input(&x, i386Keyboard);
			if (fakedMask)
				keyc->state = save_state;
		}
	    }
	    break;

	  case XQ_MOTION:
	    /*
             * rodent deltas (and maybe button state change)
             */
	    lastmouse = *mouse;
	    mouse->x += (short) pE->xq_x; /* update the mouse position */
	    mouse->y += (short) pE->xq_y;


	    if (mouse->x != lastmouse.x || mouse->y != lastmouse.y) {
		XTEST_STEALMOTIONDATA();
	    	miPointerDeltaCursor(mouse->x - lastmouse.x,
				     mouse->y - lastmouse.y,
				     lastEventTime);
	    } 

	    /* (fall through to check button state) */
	    /*FALLTHROUGH*/
	  case XQ_BUTTON:
	    /* button state change (no rodent deltas) */
	    /* Do we need to generate a button event ? */
	    current_state = pE->xq_code&07;
	    changes	  = current_state^mouse_state;
	    if (changes)
	    {
		x.u.keyButtonPointer.rootX = mouse->x; /* necessary? */
		x.u.keyButtonPointer.rootY = mouse->y; /* necessary? */
		SETBUTTONINFO(LEFT_BUTTON_BIT, 1);
		SETBUTTONINFO(MIDDLE_BUTTON_BIT, 2);
		SETBUTTONINFO(RIGHT_BUTTON_BIT, 3);
		mouse_state = current_state; 
	    }
	    break;

	  default:
	    ErrorF("Bad kbd/mouse queue event type: %d\n", pE->xq_type);
	    break;
	}
	    
      EndOfLoop:
#ifdef XTESTEXT1
	if ( XTestKernelQueueFlag )
#endif
	{
	    queue->xq_head++;
	    queue->xq_head %= qLimit;
	}
    }
}


/*****************
 * ProcessInputEvents:
 *    processes all the pending input events
 *****************/
void
ProcessInputEvents()
{
    i386ProcessInputEvents();
    mieqProcessInputEvents();
    miPointerUpdate();
}

#ifdef DEBUG
prdebug(x)
    xEvent	x;
{
    char	*what, *sense;
    char	temp[16];

    switch (x.u.u.type) {
	case KeyPress:
	    sense = "KeyPress";
	    break;
	case KeyRelease:
	    sense = "KeyRelease";
	    break;
	case ButtonPress:
	    sense = "ButtonPress";
	    break;
	case ButtonRelease:
	    sense = "ButtonRelease";
	    break;
	default:
	    sense = "MotionNotify";
	    break;
    }

    switch (x.u.u.detail) {
	case 3:
	    what = "RIGHT_BUTTON";
	    break;
	case 2:
	    what = "MIDDLE_BUTTON";
	    break;
	case 1:
	    what = "LEFT_BUTTON";
	    break;
	case 0:
	    what = "MOVEMENT";
	    break;
	default:
	    sprintf(temp, "KEY_%d", x.u.u.detail);
	    what = temp;
	    break;
    }

    ErrorF("%s: %s at (%d, %d)\n", what, sense,
	x.u.keyButtonPointer.rootX, x.u.keyButtonPointer.rootY);
}
#endif

void
SetTimeSinceLastInputEvent()
{
    lastEventTime = GetTimeInMillis();
}

/* ARGSUSED */
static void
i386Bell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{
    extern int	condev;			/* fd of the console */

    if (loud > 0) {
	/* Ring the kd bell - no software volume control;
	 * bell parameters: length 0x____0000, tone 0x0000____.
	 * Our old fixed setting was length: 150, tone: 500.
	 */

	ioctl (condev, KDMKTONE, (long)(length << 16 | tone));
    }
}

/* ARGSUSED */
static void
i386ChangeKeyboardControl(pDevice, ctrl)
    DevicePtr pDevice;
    KeybdCtrl *ctrl;
{
    /* We don't support changeable bell volume, keyclick, autorepeat, or LEDs */

    length = (ushort) ctrl->bell_duration;
    tone = (ushort) ctrl->bell_pitch;
}

#ifdef	XTESTEXT1

/*
** Very Hardware Dependent: assuming 8-bit, 2's comp signed chars
*/

#define MAX_SIGNED_CHAR		 127
#define MIN_SIGNED_CHAR		-128

static xqEvent *
XTestDeQueue()
{
	extern XTestQueueNode	*XTestQueueNodeFreeList;
	XTestQueueNode		*node;
	xqEvent			*event;

	XTestKernelQueueFlag = FALSE;
	node = xTestQ.head;
	event = &node->xqevent;

	if ( event->xq_type == XQ_MOTION )
	{
		Bool	skip_DeQ = FALSE;
		int	delta_x;
		int	delta_y;

		event->xq_code = mouse_state;

		delta_x = node->variant1 - mouse->x;
		delta_y = node->variant2 - mouse->y;

		if ( delta_x > MAX_SIGNED_CHAR )
		{
			event->xq_x = MAX_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else if ( delta_x < MIN_SIGNED_CHAR )
		{
			event->xq_x = MIN_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else
			event->xq_x = delta_x;

		if ( delta_y > MAX_SIGNED_CHAR )
		{
			event->xq_y = MAX_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else if ( delta_y < MIN_SIGNED_CHAR )
		{
			event->xq_y = MIN_SIGNED_CHAR;
			skip_DeQ = TRUE;
		}
		else
			event->xq_y = delta_y;

		if ( skip_DeQ )
			return event;
	}
	else if ( event->xq_type == XQ_BUTTON )
	{
		if ( node->variant1 == XTestKEY_UP )
			event->xq_code = mouse_state | node->variant2;
		else
			event->xq_code = mouse_state & ~node->variant2;
	}
	else	/* xq_type == XQ_KEY */
	{
		switch( event->xq_code )
		{
			case K_Num_Lock:

				if ( ledsdown & LED_NUM )
					SetKbdLeds( ledsdown & ~LED_NUM );
				else
					SetKbdLeds( ledsdown | LED_NUM );

				break;

			case K_Caps_Lock:

				if ( ledsdown & LED_CAP )
					SetKbdLeds( ledsdown & ~LED_CAP );
				else
					SetKbdLeds( ledsdown | LED_CAP );

				break;

			case K_Kana_Lock:
				if (!AXKeyboard)
					break;

				if ( ledsdown & LED_KANA )
					SetKbdLeds( ledsdown & ~LED_KANA );
				else
					SetKbdLeds( ledsdown | LED_KANA );

				break;

/*
			case K_Scroll_Lock:

				if ( ledsdown & LED_SCR )
					SetKbdLeds( ledsdown & ~LED_SCR );
				else
					SetKbdLeds( ledsdown | LED_SCR );

				break;
*/
			default:

				break;
		}
	}

	/*
	** Remove head of xTestQ, place on free list
	** and return pointer to xqEvent.
	*/

	xTestQ.head = node->next;
	node->next = XTestQueueNodeFreeList;
	XTestQueueNodeFreeList = node;
	return event;
}

static int
xtest_map_key( key )
{
#ifdef INGNORE_UNMAPPED_KEYS	/* Don't Blame me for the misspelling */

	/*
	** The following codes are not mapped:
	**
	**	70: ScrollLock
	**	84: Alt_SysRq
	**	85: ??
	**	86: ??
	*/

	if ( keycode == 70 || ( keycode >= 84 && keycode <= 86 ))
		return key;
#endif

	return key + TABLE_OFFSET;
}

#endif

void
constrainXY2Scr (px, py)
   short           *px;
   short           *py;
{
#ifdef XTESTEXT1
            if ( *px < 0 )
                 *px = 0;
            else if ( *px >= (short) screenInfo.screens[0]->width)
                *px = (short) screenInfo.screens[0]->width -1;

            if ( *py < 0 )
                 *py = 0;
            else if ( *py >= (short) screenInfo.screens[0]->height)
                 *py = (short) screenInfo.screens[0]->height -1;
#endif
}

