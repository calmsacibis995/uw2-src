/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:ddx/io/init.c	1.6.1.17"

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
/* $Header: init.c,v 1.27 87/09/09 17:09:06 rws Exp $ */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#ifdef SVR4
#include <sys/stream.h>
#include <sys/fcntl.h>
#else
#include <fcntl.h>
#endif

#include <signal.h>
#include <sys/param.h>

#define EVC
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/xque.h>
#include <termio.h>
#include <sys/stat.h>

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "cursor.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "gc.h"
#include "gcstruct.h"
#include "site.h"

#include "xwin_io.h"
#include "siconfig.h"

#include "../si/mipointer.h"

#ifdef XTESTEXT1
#include "keynames.h"
extern int xtest_command_key;
#endif /* XTESTEXT1 */

#define strdup	xstrdup

int	condev = 0;		/* fd of the console */
int	indev;			/* fd of the mouse   */
unchar  orgleds;                /* LED's that were down
                                 * when we started or after
                                 * we switched back VT */
static int	vtnum = 0;
int		siScreenIndex=-1;
static unsigned long siScreenGeneration=0;
int		keyboard = KBD_US;

#define VENDOR_STRING_BASE      "USL's XWIN with Screen Interface Technology."
#define VENDOR_STRING_MAXLENGTH 400

static FILE *config_fp = (FILE *)0; /* open file pointer */
char	*config_file	= NULL;		/* configuration file to use */
char    *vendor_string; 	/* it's a global dynamic vendor string */
Bool 	defaultVendorString = TRUE; /* default vendor string flag */
Bool	SunRiverStation = FALSE;

SIScreenRec	siScreens[MAX_NUMSCREENS];
int		siNumScreens;

SIScreenRecP	global_pSIScreen = &siScreens[0];

/* Redefining XSIG, so that we can handle signals properly in gdb
 * REMOVE THE NEXT LINE AFTER YOU ARE DONE WITH DEBUGGING
#define XSIG    SIGWINCH
 */
#define XSIG    SIGTRAP
int		xsig = XSIG;
extern xqEventQueue *queue;
extern unchar 	ledsdown;	/* defined in xwin_io.c */
extern int	qLimit;
extern void	GetKDKeyMap ();
extern void	NoopDDA ();
extern char     *getenv();
extern unsigned char GetKbdLeds();

#ifdef FLUSH_IN_BH
extern void	xwinBlockHandler ();
extern void	xwinWakeupHandler ();
#endif

static Bool	ioDisplayDeviceInit();
static Bool	(*savedCloseScreen)(); /* CloseScreen Wrapper */
static Bool	ioCloseScreen ();
static Bool	ioSaveScreen ();
extern int	io_readcmapdata();

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	256

/*
 * Frame-buffer-private info.
 *	fd  	  	file opened to the frame buffer device.
 *	info	  	description of the frame buffer -- type, height, depth,
 *	    	  	width, etc.
 *	pGC 	  	A GC for realizing cursors.
 *	GetImage  	Original GetImage function for this screen.
 *	CreateGC  	Original CreateGC function
 *	CreateWindow	Original CreateWindow function
 *	ChangeWindowAttributes	Original function
 *	GetSpans  	GC function which needs to be here b/c GetSpans isn't
 *	    	  	called with the GC as an argument...
 *	fbPriv	  	Data private to the frame buffer type.
 */
typedef struct {
    GCPtr   	  	pGC;	    /* GC for realizing cursors */

    void    	  	(*GetImage)();
    Bool	      	(*CreateGC)();/* GC Creation function previously in the
				       * Screen structure */
    Bool	      	(*CreateWindow)();
    Bool		(*ChangeWindowAttributes)();
    unsigned int  	*(*GetSpans)();
    void		(*EnterLeave)();
    int			fd; 	    /* Descriptor open to frame buffer */
    int			fbtype;	    /* Frame buffer type */
    pointer		fbPriv;	    /* Frame-buffer-dependent data */
    pointer		fb_pbits;   /* pointer to screen bits (or -1) */
    int			fb_width;   /* pixel width of screen buffer */
    int			fb_xsize, fb_ysize;
    int			fb_dpix, fb_dpiy;
} fbFd;

extern fbFd 	  ioFbs[];
extern Bool	  screenSaved;		/* True is screen is being saved */
extern void	  ErrorF();

extern Bool i386ScreenInit();		/* i386 specific code */
extern int i386MouseProc();
extern int i386KeybdProc();

#define MOTION_BUFFER_SIZE 0

int displayType;

/*
 * openvt is called from connection.c, before we read the config file
 * use this global for now - look at this later
 */
char *ttyname = NULL;

/* SI: start */
extern  void    CloseVirtTerm ();
Bool          screenSaved = xFalse;

fbFd	ioFbs[MAX_NUMSCREENS];  /* descriptors of open frame buffers */

static PixmapFormatRec	formats[] = {
    1, 1, BITMAP_SCANLINE_PAD,	/* 1-bit deep, 1-bpp */
    2, 2, BITMAP_SCANLINE_PAD,	/* 2-bit deep, 2-bpp */
    4, 4, BITMAP_SCANLINE_PAD,	/* 4-bit deep, 4-bpp */
    8, 8, BITMAP_SCANLINE_PAD,	/* 8-bit deep, 8-bpp */
    15, 16, BITMAP_SCANLINE_PAD, /* 15-bit deep, 16-bpp */
    16, 16, BITMAP_SCANLINE_PAD, /* 16-bit deep, 16-bpp */
    24, 32, BITMAP_SCANLINE_PAD, /* 24-bit deep, 32-bpp */
    32, 32, BITMAP_SCANLINE_PAD, /* 32-bit deep, 32-bpp */
};
#define NUMFORMATS	(sizeof formats)/(sizeof formats[0])

extern char	*display;		 /* in server/os/sysV/connection.c */

Bool screen_inited = 0; /* if 1, display has been initialized */

/* SI: end */

/* ARGSUSED */
Bool
InitInput(argc, argv)
    int argc;
    char *argv[];
{
    DevicePtr p, k;
    extern Bool mieqInit();
    
#ifdef XTESTEXT1
    xtest_command_key = K_F12 + TABLE_OFFSET;
#endif /* XTESTEXT1 */

    p = AddInputDevice(i386MouseProc, TRUE);
    k = AddInputDevice(i386KeybdProc, TRUE);
    if (!p || !k)
	FatalError("\tFailed to create input devices in InitInput.\n");

    SetTimeSinceLastInputEvent();

    RegisterPointerDevice(p);
    RegisterKeyboardDevice(k);
    miRegisterPointerDevice(screenInfo.screens[0], p);
    if (mieqInit (k, p) == FALSE)
        return FALSE;
    return TRUE;
}

ScreenInitFailure()
{
    FatalError("Cannot initialize the screen(s).\n\
Check if all the shared libraries are installed properly.\n");
}

/*-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all framebuffers configured for the
 *	current display.
 * Results:
 *	screenInfo init proc field set
 * Side Effects:
 *	None
 *----------------------------------------------------------------------
 *
 * NUMSCREENS is the number of supported frame buffers (i.e. the number of
 * structures in odFbData which have an actual probeProc).
 *---------------------------------------------------------------------*/

Bool
InitOutput(pScreenInfo, argc, argv)
  ScreenInfo	*pScreenInfo;
  int     	argc;
  char		**argv;
{
    int		i, index;
    char	*tfp;
    char 	*cp = NULL;
    char	path[MAX_LINESIZE];
    static void xwinPrintConfig();
    extern int defaultColorVisualClass;
    si_currentScreen();

    index = 0;

    /*
     *	Due to the inner workings of vts, need to find and open an
     *	available vt before actually opening the output device.
     *	This requires that we find the correct keyboard device here,
     *	instead of dealing with it in InitInput().  (Note that only
     *	the first keyboard for this display will actually be used.)
     */

    if (siScreenGeneration != serverGeneration) {
	siScreenIndex = AllocateScreenPrivateIndex();
	if (siScreenIndex < 0)
	  return(FALSE);
	siScreenGeneration = serverGeneration;
    }

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    /* SI: */
    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
      pScreenInfo->formats[i] = formats[i];

    /*
     * This function is called everytime the last client disconnects from
     * the server
     * why read the config file, everytime this function is called?
     */
    if (!screen_inited) {
	/*
	 * get the name of the Xwinconfig file with full path
	 */
	if ( (tfp = getenv("XWINCONFIG")) != NULL )
	  config_file = tfp;

	/* file not open yet */
	if (config_fp == (FILE *)0) 
	{ 
    	    struct stat stat_buf;

	    /*
	     * if configuration file name is given on command line,
	     * don't bother figuring it out.
	     */
	    if (!config_file) {
		cp = (char *) GetXWINHome ("defaults");
		if (cp != (char *) 0) {
		    strcpy (path, cp);
		    strcat (path, "/");
		    strcat (path, CONFIG_FILE);
		    config_file = path;
		}
		else
		  config_file = "/usr/X/defaults/Xwinconfig";
	    }


	    if ((config_fp = fopen (config_file, "r")) == (FILE *)0) {
		FatalError("\tCannot open config file: %s.\n", config_file);
	    }
	    else {
		/* if the user defined config file is not valid, bail out */
		if ( (fstat(fileno(config_fp), &stat_buf) == -1) &&
		   (!S_ISREG(stat_buf.st_mode)) )
			FatalError("\tInvalid config file: %s\n", config_file);

		/*
		 * if the config_file is not user defined, set it to NULL,
		 * or else, when the server restarts (when last client 
		 * disconnects) config_file points to junk....
		 */
		if (cp) 
		  config_file = NULL;
	    }
	} else
	  rewind (config_fp);	/* file already open, just rewind */

	/*
	 *	Read the configuration file.
	 */
	/*
	 * First, try for a new config file format; if it doesn't 
	 * succeed,  then try for older format
	 */
	if ( !(siNumScreens = r_configfile(config_fp)) ) 
	{
	      printf ("Using old config file format, file: %s\n",config_file);
	      rewind (config_fp);

	      global_pSIScreen = pSIScreen = &siScreens[0];

	      /* pre v1.1, we support only one screen */
	      siNumScreens = r_oldconfigfile(config_fp);
	}
	fclose(config_fp);

	if (siNumScreens < 1)
		ScreenInitFailure();
    }  /* screen_inited */

    for (i=0; i<siNumScreens; i++) {
	/*
	 * If no screen was specified, then "screen" will be -1.
	 * It needs to be changed to 0.  This may not be correct
	 * behaviour when support for multiple screens is added.
	 */
	global_pSIScreen = pSIScreen = &siScreens[i];

	if (io_readcmapdata() < 0)
	    siColormap->sz = 0;
	else
	{
	    if (xwin_verbose_level == 1)
	      ErrorF("Colormap initialized with data from Xwincmaps file.\n");
	}

	defaultColorVisualClass = siColormap->visual;

	if (siConfig->screen == -1)
	  siConfig->screen = 0;

	/* ErrorF("** attempt init screen %d [%s]\n",i,siConfig->class); */

	/* this entry interests us. Try initializing the device */
	if (ioDisplayDeviceInit (index, argc, argv) == TRUE) {
	    /* This display initialized properly */
	    index++;
	}
	else {
		ErrorF("Initialization of screen %d failed.\n");
		ScreenInitFailure();
	}
    }

    if (xwin_verbose_level == 1)
    	ErrorF("All screens initialized\n");

    if (index == 0) {
	ScreenInitFailure();
    } else {
	/* remember that we need to de-init screen later */
	screen_inited++;

	if (defaultVendorString) {
	    char tmpbuf[128];

	    sprintf(vendor_string, "%s ", VENDOR_STRING_BASE);
	    /*
	     * VPiX wierdity: VPiX depends on a pattern "(using" in 
	     * vendor_string; so if you do not have this the 'zoom' 
	     * feature in VPiX will not work
	     */
	    for (i=0; i < index; ++i) {
		sprintf(tmpbuf," %s.%d=%s",
			display,i,siScreens[i].cfgPtr->class);
		strcat(vendor_string,tmpbuf);
	    }
	    sprintf(tmpbuf, " [(using vt%02d)]", vtnum);
	    strcat(vendor_string, tmpbuf);
	}
    }

    pScreenInfo->numScreens = index;
#if 0
    if (screen_inited < 2)
#endif
      xwinPrintConfig ();	/* print only at start up time */

    return(TRUE);
}

/*-----------------------------------------------------------------------
 * ioDisplayDeviceInit --
 *	Attempt to find and initialize a particular framebuffer.
 *
 * Results:
 *	Returns TRUE if successful, FALSE if not.
 *
 * Side Effects:
 *	The device driver init routine is called, passed the open VT file
 *	descriptor, a pointer to the configuration information, and the
 *	hardware information pointer.
 *
 *----------------------------------------------------------------------*/
static Bool
ioDisplayDeviceInit(index, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         i;
    int		ret;
    int (*initFunc)();
    extern int siTrace;
    si_currentScreen();

    if (!screen_inited) {
	/*
	 * NOTE: The SIFunctions structure used to be declared by a DM;
	 * But, now (from SI spec v1.1), this structure is allocated by the
	 * CORE server and let the DM fill up the function pointers. This
	 * needed to avoid dynamic library conflicts in a multi-screen env.
	 * get the ptr to the the DM's "initialization" function.
	 * This is done by looking (dlsym) for DM_InitFunction after the
	 * shared library is opened.
	 */
	initFunc = 0;
	if( !LoadDisplayLibrary(siConfig->vendor_lib, &initFunc) )
		return FALSE;

	/*
	 * to be safe and also to make sure we won't have any problems
	 * with older DisplayModules, initialize some of the newer
	 * fields to some default values here. On return from
	 * ioDisplayDeviceInit, these fields will be initialized
	 * to appropriate values by the DisplayModules
	 */
	siConfig->IdentString = NULL;

	/*
	 * initialize the server's SI version
	 */
	siFlags->SIserver_version = X_SI_VERSION1_1;
	siFlags->SIdm_version = 0;

	siFlags->SIfb_pbits = SI_FB_NULL;
	siFlags->SIfb_width = 0;
    	/*
	 * call the device-specific initialization routine 
	 */
	if (initFunc) {
	   ret = (*initFunc) (condev, pSIScreen);
	} else {
            ret = si_Init (condev, siScreens[0].cfgPtr,
			   siScreens[0].flagsPtr,
			   &siScreens[0].funcsPtr);
            /* turn OFF tracing for older versions */
            siTrace = 0;
	}

	if ( !ret ) {
	   printf ("Error detected at display initialization time.\n");
	   if (siFlags->SIdm_version > 0x0100) {
		printf("Screen %ld :\n",siConfig->screen);
		printf("    Display Class: %s\n",
		       siConfig->class ? siConfig->class:"UNKNOWN");
		printf("    Vendor Library: %s\n",
		       siConfig->vendor_lib ? siConfig->vendor_lib:"UNKNOWN");
		printf("    Vendor String: %s\n",
		       siConfig->IdentString ? siConfig->IdentString:"UNKNOWN");
	   	FatalError("\tDisplay Module SI version: %04x\n",
			   siFlags->SIdm_version);
		/* DO WE really want to print out all this info?
		 * Is there a guarantee, that some of these fields
		 * are initialized properly if si_Init fails ?
		 * LOOK INTO THIS LATER 3/11/93
		 */
	   }
	   else {
	   	FatalError("\tDisplay Class: %s\nSIdm version: %x",
			    siConfig->class ? siConfig->class:"UNKNOWN",
			    siFlags->SIdm_version);
	   }
    	}
    }

    /*
     * if siTrace != 0, turn on the tracing of the function calls
     * There are 3 levels of tracing.
     * 	level 1: most often used calls (ex: screen->screen, scr->mem etc
     *  level 2: includes level-1 and the 3 mandatory scanline functions
     *  level 3: trace all SI functions
     */ 
    if (siTrace > 0)
      sitrace (pSIScreen, siTrace);

    ioFbs[index].fd = condev;
    ioFbs[index].fb_xsize = si_getscanlinelen;
    ioFbs[index].fb_ysize = si_getscanlinecnt;
    ioFbs[index].fb_dpix = siFlags->SIxppin ? siFlags->SIxppin : 90;
    ioFbs[index].fb_dpiy = siFlags->SIyppin ? siFlags->SIyppin : 90;
    if (siFlags->SIfb_pbits == SI_FB_NULL) {
	/* initialize fb_width to a reasonable value */
	siFlags->SIfb_width = siFlags->SIlinelen;
    }
    ioFbs[index].fb_pbits = siFlags->SIfb_pbits;
    ioFbs[index].fb_width = siFlags->SIfb_width;

    i = AddScreen(i386ScreenInit, argc, argv);
    if (i >= 0) {
	/*siScreenSwitch(pScreenInfo->screens[screenNumber], i != 0);*/
	return(1);
    }
    return(0);
}

int
i386CursorInit(pScreen)
  ScreenPtr pScreen;
{
    extern miPointerScreenFuncRec siPointerScreenFuncs;

#if 0
    if (si_havenocursor) {
	return(miDCInitialize(pScreen, &siPointerScreenFuncs));
    } else if (si_havefakecursor) {
	return(miPointerInitialize(pScreen,
				   &siPointerScreenFuncs,
				   &siPointerHWSpriteFuncs,
				   FALSE));
    } else if (si_havefakecursor) {
	retur(miPointerInitialize(pScreen,
				  &siPointerScreenFuncs,
				  &siPointerHWSpriteFuncs,
				  FALSE));
    }
    return(FALSE);
#else /* Software Cursor Only */
    return(miDCInitialize(pScreen, &siPointerScreenFuncs));
#endif
}

/*------------------------------------------------------------------------
 * ioFrameBufferInit --
 *	Attempt to initialize a framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in.  The
 *	video is enabled for the frame buffer...
 *
 *	The graphics context for the screen is created. The CreateGC,
 *	CreateWindow and ChangeWindowAttributes vectors are changed in
 *	the screen structure.
 *
 *	Both a BlockHandler and a WakeupHandler are installed for the
 *	first screen.
 *----------------------------------------------------------------------*/
/*ARGSUSED*/
Bool
ioFrameBufferInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!siScreenInit(pScreen,
		      ioFbs[index].fb_pbits,
		      ioFbs[index].fb_xsize,
		      ioFbs[index].fb_ysize,
		      ioFbs[index].fb_dpix,
		      ioFbs[index].fb_dpiy,
		      ioFbs[index].fb_width))
				
      return (FALSE);

    pScreen->SaveScreen = ioSaveScreen;
    /* - save old CloseScreen function for wrapper */
    savedCloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = ioCloseScreen;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

#ifdef FLUSH_IN_BH
    pScreen->WakeupHandler = xwinHandler;
    pScreen->BlockHandler = xwinHandler;
#else
    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
#endif

    (void) ioSaveScreen(pScreen, SCREEN_SAVER_FORCER);

    if (!i386CursorInit(pScreen)) {
	ErrorF("Can't initialize cursor for screen #%d\n",index);
	return(FALSE);
    }

    return (siCreateDefColormap(pScreen));
}

#ifdef FLUSH_IN_BH
void
xwinHandler(nscreen, pbdata, pptv, pReadmask)
    int nscreen;
    pointer pbdata;
    struct timeval **pptv;
    pointer pReadmask;
{
    si_Flushcache();
}
#endif

/*------------------------------------------------------------------------
 * ioSaveScreen --
 *	Disable the video on the frame buffer to save the screen.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video enable state changes.
 *
 *----------------------------------------------------------------------- */
static Bool
ioSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    SIint32         state;

    si_prepareScreen(pScreen);

    switch (on) {
    case SCREEN_SAVER_FORCER:
	SetTimeSinceLastInputEvent();
	/* TimeSinceLastInputEvent (); */
	screenSaved = FALSE;
	state = SI_TRUE;
	break;
    case SCREEN_SAVER_OFF:
	screenSaved = FALSE;
	state = SI_TRUE;
	break;
    case SCREEN_SAVER_ON:
    default:
	screenSaved = TRUE;
	state = SI_FALSE;
	break;
    }

    si_VBOnOff(state);
    return(TRUE);
}

/*------------------------------------------------------------------------
 * ioCloseScreen --
 *	called to ensure video is enabled when server exits.
 *
 * Results:
 *	Screen is unsaved.
 *
 * Side Effects:
 *	None
 *
 *----------------------------------------------------------------------- */
/*ARGSUSED*/
static Bool
ioCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool ret;

    (*pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF);

    if (pScreen->allowedDepths) {
	if (pScreen->allowedDepths->vids)
	  xfree((unsigned char *)pScreen->allowedDepths->vids);
	xfree((unsigned char *)pScreen->allowedDepths);
	pScreen->allowedDepths = NULL;
    }
 
    /*  pScreen->visuals does not need to be freed here, since it is added as
     *  a resource in ScreenInit and is freed with the rest of the 
     *  resources. 
     */

#if 0
    if(pScreen->devPrivates[siScreenIndex].ptr)
      xfree(pScreen->devPrivates[siScreenIndex].ptr);
#endif

    /* - Unwrap and call old CloseScreen function */
    pScreen->CloseScreen = savedCloseScreen;
    ret = (*pScreen->CloseScreen)(i, pScreen);
    /* - Re-Wrap */
    pScreen->CloseScreen = ioCloseScreen;

    return(ret);
}

static void
i386PixelError(index)
    int index;
{
    ErrorF("Only 0 or 1 are acceptable pixels for device %d\n", index);
}

/*
 * NOTE: ScreenInit is called when the server first starts up and also
 *	 _again_ whenever the server resets (iff all clients gone).
 */

Bool
i386ScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    Bool		retval;
    int			i;
    extern char		*blackValue, *whiteValue;
    extern xqCursorPtr  mouse;
    static Bool		firstTime = TRUE;

    /* tuck away the SI screen pointer */
    pScreen->devPrivates[siScreenIndex].ptr = (pointer)global_pSIScreen;

    if (firstTime) {
	struct kd_quemode	qmode;
	struct sigaction	act;
	extern int		waiting_for_acquire;
	extern void		sigusr1();

	/* The code to open the VT has been moved to a separate routine,
	 * openvt(), and is called later by CreateWellKnownSocket().
	 * This is to prevent switching VT's before making sure that this
	 * is the first server that is running.
	 */
	firstTime = FALSE;

	if ((indev = open ("/dev/mouse", O_RDONLY)) < 0) {
		Error("Cannot open /dev/mouse");

#undef  MOUSELESS
#define MOUSELESS
#ifndef MOUSELESS
		attexit(1);
#endif
	}
	qmode.qsize = 500;
	qmode.signo = xsig;
	ioctl(condev, KDQUEMODE, NULL);
	if (ioctl(condev, KDQUEMODE, &qmode) == -1) {
		Error("KDQUEMODE failed");
		attexit(1);
	}
	queue = (xqEventQueue *)qmode.qaddr;
	qLimit = queue->xq_size;


	/* allocate the mouse cursor here and never free it */
	if (!(mouse = (xqCursorPtr)xalloc(sizeof(xqCursorRec)))) {
	    ErrorF ("i386ScreenInit: can't allocate mouse cursor\n");
	    return (FALSE);
	}

	/*
	 * if the user didn't pass any vendor string, then allocate enough
	 * space here and initialize * the default vendor string later.
	 */
	if (defaultVendorString) {
	   /* allocate the vendor_string here and never free it */
	   if (!(vendor_string =
		(char *)xalloc(VENDOR_STRING_MAXLENGTH * sizeof(char)))) {
	    	ErrorF ("i386ScreenInit: can't allocate vendor string\n");
	    	return (FALSE);
	    }
	}

	orgleds = GetKbdLeds();		/* Save the current kbd LED state */
	SetKbdLeds(ledsdown);		/* Turn 'em all off for starters */

	/* VT switching state:
	 *  1 "Turn it on", set to not-waiting
	 *  2 trap the VT switch signal forever blocking all signals while in
	 *    sigusr1().
	 */
	waiting_for_acquire = 0;
	
	(void)sigfillset(&act.sa_mask);
	act.sa_handler = sigusr1;
	act.sa_flags = 0;
	(void)sigaction(SIGUSR1, &act, NULL);
	(void)sigrelse(SIGUSR1);	/* undo sighold() */

    }	/* end of: if (firstTime) */

    retval = ioFrameBufferInit (index, pScreen, argc, argv);

    /* blackValue, whiteValue may have been set in ddxProcessArgument */

    if(blackValue)
    {
	if((i = atoi(blackValue)) == 0 || i == 1)
	    pScreen->blackPixel = i;
	else  
	    i386PixelError(index);
    }
    if(whiteValue)
    {
	if((i = atoi(whiteValue)) == 0 || i == 1)
	    pScreen->whitePixel = i;
	else  
	    i386PixelError(index);
    }
    return(retval);
}

int
openvt()
{		
    char		vtdev[20];	/* allow up to 20 chars in tty name */
    int			fd;
    struct vt_mode	vtmode;
    struct termio	ttyparms;

    /* Sun River work */

    char	tty[20];	/* allow up to 20 chars in tty name */
    char	suntty[20];	/* allow up to 20 chars in tty name */
    char	errmsg[40];
    extern char	*display;
    int		dispno = atoi( display );
    int		devno;
    int		srchan;
    static int	kdmode = -1;
    extern	char *ttyname;

    /*
     * SunRiver: 100 thru 199
     * MaxSpeed: 200 thru 299
     */
    devno = dispno / 10;

    switch ( devno )
    {
	/* console */
        case 0:		
	    if (ttyname)		/* command line defined ttyname */
            	sprintf( tty, ttyname );
	    else
            	sprintf( tty, "/dev/console"); /* default */

            *suntty = '\0';
            sprintf( errmsg, "Cannot open %s", tty );
            break;

	/* one of the Sun River Stations */
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            SunRiverStation = TRUE;
            srchan = devno - 10;	/* there are 4 channels per board */
					/* and you can use up to 4 boards */

            sprintf( tty, "/dev/s%dvt00",  srchan );
            sprintf( suntty, "s%d", srchan );
            sprintf( errmsg, "Cannot open /dev/s%dvt00", srchan );
            break;

	/* one of the Maxpeed MaxStation */
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
	    if (ttyname) {	/* command line defined ttyname */
		    sprintf( tty, "/dev/tty%s00", ttyname);
		    sprintf( errmsg, "Cannot open /dev/tty%s00", ttyname);
	    }
	    else {
		    /* gaa ?? this is what MaxSpeed wants. There should
		     * be a better way of doing this.
		     */ 
		    sprintf( tty, "/dev/tty%s00", "gaa" );
		    sprintf( errmsg, "Cannot open /dev/tty%s00", "gaa");
	    }
            *suntty = '\0';
            break;

        default:
            Error("Illegal display number specified");
            return ( -1 );
    }

    if (getenv("XWinTTY")) {
	condev = atoi(getenv("XWinTTY"));
	GetKDKeyMap (condev);		/* Get default keyboard mappings */
	return(condev);
    }

    /* Sun River work: add timeout if monitor NOT hooked up to board */

    alarm(20);	/* 20 seconds of real time */

    if ((fd = open(tty, O_WRONLY)) < 0) {
	alarm( 0 );		/* turn off alarm */
	Error(errmsg);
	return(-1);
    }
    alarm(0);		/* turn off alarm */
    GetKDKeyMap (fd);		/* Get default keyboard mappings */
    if (ioctl(fd, VT_OPENQRY, &vtnum) < 0 || vtnum < 0) {
	Error("Cannot find a free VT");
	return(-1);
    }

    /* Sun River work: add suntty below */

    sprintf(vtdev, "/dev/%svt%02d", suntty, vtnum);

    if (ioctl (fd, TCGETA, &ttyparms) < 0)
    {
	Error ("openvt:	 TCGETA failed");
	return (-1);
    }

    if (ttyparms.c_lflag & (ICANON | ISIG) == 0)
    {
	ErrorF ("openvt:  /dev/console is in raw mode\n");
	return (-1);
    }

    setpgrp();

    condev = open(vtdev, O_RDWR);

    {
	sprintf(errmsg, "XWinTTY=%d", condev);
	putenv(strdup(errmsg));
    }
    if (condev < 0) {
	ErrorF("Cannot open device %s", vtdev);
	Error(" ");
	return(-1);
    }

    if (ioctl (condev, TCSETAW, &ttyparms) < 0)
    {
	Error ("openvt:  TCSETAW failed");
	return (-1);
    }

    sighold(SIGUSR1);		/* Hold till we are set up to service it. */
    signal(SIGUSR2, SIG_IGN);	/* Ignore until we need it later. */

    if (ioctl(condev, VT_ACTIVATE, vtnum) == -1)
    {
	Error("VT_ACTIVATE failed");
    }
    else 
    {
	if (ioctl(condev, VT_WAITACTIVE, 0) == -1)
	    Error("VT_WAITACTIVE failed");
    }

    if (ioctl (condev, VT_GETMODE, &vtmode) == -1) {
	Error("VT_GETMODE failed");
	vtmode.mode = VT_AUTO; /* force VT_SETMODE */
    }

    if (vtmode.mode != VT_PROCESS || vtmode.relsig != SIGUSR1 ||
	vtmode.acqsig != SIGUSR2 || vtmode.frsig != SIGUSR2 ||
	vtmode.waitv != 0) {

	vtmode.mode = VT_PROCESS;
	vtmode.relsig = SIGUSR1;
	vtmode.acqsig = SIGUSR2;
	vtmode.frsig = SIGUSR2;
	vtmode.waitv = 0;

	if (ioctl(condev, VT_SETMODE, &vtmode) == -1) {
	    Error("VT_SETMODE VT_PROCESS failed");
	    /* attexit (1); */
	}
    }

    if (kdmode == -1) {
	kdmode = 0;
	if (ioctl(condev, KDGETMODE, &kdmode) == -1) {
	    Error("KDGETMODE failed");
	    attexit (-1);
	}
    }

    if (kdmode != KD_GRAPHICS) {
	if (ioctl(condev, KDSETMODE, KD_GRAPHICS) == -1)
	{
	    Error("KDSETMODE KD_GRAPHICS failed");
	    attexit (-1);
	}
    }

    return(condev);
}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */
void
AbortDDX()
{
	RestoreOutput ();
}

/* Called by GiveUp(). */
void
ddxGiveUp()
{
	resetmodes ();
}

int siTrace = 0;

int
ddxProcessArgument (argc, argv, i)
    int argc;
    char *argv[];
    int i;
{
    int num_consumed = 0;
    extern char *blackValue, *whiteValue;
    extern char *config_file;
    extern char *color_file;
    extern Bool disableSaveUnders;
    extern short fixedupri, SchedulerClass;
    extern Bool	defeatAccessControl;
    extern Bool defaultVendorString;

	if(strcmp(argv[i], "-bp") == 0)
	{
		if(++i < argc)
		{
		    blackValue = (char *)argv[i];
		    num_consumed = 2;
		}
		else
		    UseMsgExit1();
	}
	else if(strcmp(argv[i], "-wp") == 0)
	{
		if(++i < argc)
		{
		    whiteValue = (char *)argv[i];
		    num_consumed = 2;
		}
		else
		    UseMsgExit1();
    	}
	else if ( strcmp( argv[i], "-config") == 0)
	{
	    if(++i < argc) {
			config_file = strdup(argv[i]);
			num_consumed = 2;
	    }
	    else
		UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-stdvga") == 0) {
	    config_file = "/usr/X/defaults/Xwinconfig.ini";
	    num_consumed = 1;
	}
	else if ( strcmp( argv[i], "-sitrace") == 0) {
	    if(++i < argc) {
		siTrace = (int)atoi(argv[i]);
		num_consumed = 2;
	    } else
	      UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-cmap") == 0)
	{
	    if(++i < argc) {
			color_file = strdup(argv[i]);
			num_consumed = 2;
	    }
	    else
		UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-vs") == 0)
	{
	    /*
	     * -vs <vendor_string>
	     *
	     * this will be an undocumented flag. Some older
	     * clients like VPiX, USL's xterm depend on some key words in 
	     * vendor string. By providing this flag, a user can override
	     * default Vendor String
	     */	
	    if(++i < argc) {
		vendor_string = (char *)xalloc((unsigned long)
					       strlen(argv[i])+1);	
	        strcpy(vendor_string,argv[i]);
		num_consumed = 2;
		defaultVendorString = FALSE;
	    }
	    else
		UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-runclass") == 0)
	{
		/*
		 * if a class is specified on the command line, save it.
		 * default is fixed class
		 */
	        if(++i < argc) {
			if ( (strcmp(argv[i],"fixed")) == 0 )
				SchedulerClass = FIXED;
			else if ( (strcmp(argv[i],"timeshare")) == 0 )
				SchedulerClass = TIMESHARE;
			else if ( (strcmp(argv[i],"realtime")) == 0 )
				SchedulerClass = REALTIME;
			else
				UseMsgExit1();

			num_consumed = 2;
		} else
			UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-upri") == 0)
	{
		/*
		 * run the server in fixed class scheduler using the 
		 * specified priority.
		 */
	        if(++i < argc) {
			fixedupri = (short)atoi(argv[i]);
			num_consumed = 2;
		} else
			UseMsgExit1();
	}
	else if ( strcmp( argv[i], "-x") == 0)
	{
	    if(++i >= argc)
		UseMsgExit1();
	    /* For U**x, which doesn't support dynamic loading, there's nothing
	     * to do when we see a -x.  Either the extension is linked in or
	     * it isn't */
	}
        else if ( strcmp( argv[i], "-xnetaccess") == 0)
        {
            if(++i < argc){
                if ( strcmp( argv[i], "on") == 0)
			defeatAccessControl = FALSE;
                else if ( strcmp( argv[i], "off") == 0)
			defeatAccessControl = TRUE;
                else
                    UseMsgExit1();

		num_consumed = 2;
            }
            else
                UseMsgExit1();
        }
        else if ( strcmp( argv[i], "-tty") == 0)
        {
            if(++i < argc){
		ttyname = xstrdup(argv[i]); 
		num_consumed = 2;
            }
            else
                UseMsgExit1();
        }
    return (num_consumed);
}

void
ddxUseMsg()
{
    ErrorF("-bp color              BlackPixel for screen\n");
    ErrorF("-wp color              WhitePixel for screen\n");
    ErrorF("-config string         configuration file\n");
    ErrorF("-stdvga                Standard VGA, 640x480 16 color mode\n");
    ErrorF("-cmap string           colormap file\n");
    ErrorF("-runclass fixed|timeshare|realtime    run server in the specified class\n");
    ErrorF("-xnetaccess on|off     turn network access on/off\n");
    ErrorF("-x string              loads named extension at init time \n");
    ErrorF("-tty ttyname           open specified tty instead of /dev/console\n");
}

static void
xwinPrintConfig()
{
  int i;

  ErrorF("\n%s\n", vendor_string);
  ErrorF("(protocol Version %d, revision %d, vendor release %d)\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, VENDOR_RELEASE );
  ErrorF("Display Modules:\n");
  for (i = 0; i < siNumScreens; i++)
    if (siScreens[i].cfgPtr->IdentString)
      ErrorF( "	Screen # %d : %s\n", i, siScreens[i].cfgPtr->IdentString );
}
