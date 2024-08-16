/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vtest.c	1.9.1.18"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>
#include "sidep.h"
#include <sys/inline.h>
#include "vconfig.h"

#ifndef MYMALLOC
#define Malloc malloc
#define Free   free
#endif

static SIScreenRec siscreens[2];
SIScreenRec *pSIScreen = &siscreens[0];
SIFunctions *HWroutines = NULL;
SIFlags *pflags;

extern unsigned char cfgFormat;

#define DURATION 5

void *handle = NULL;


#define MAX_BWIDTH   32
#define MODE_DISP_WIDTH 25

SIbitmap bmap, *bp;
/*
 * This flag is introcued because we have two routines doing dlclose.
 *	cleanexit() and intHandler().
 * We should check whether library is already closed.
 */
int	dl_flag;
/*
 * Flag indicating whether a vt was opened.
 */
int is_open = 0;

int
LoadDisplayLibrary(name, init_func)
char *name;
int *init_func;
{
    SIFunctions  **pDisplayFuncs;
    extern ScreenInterface *HWroutines;
		if(name == NULL)
		{
			ErrorF("Invalid vendor_lib specified.\n");	
			closevt();
			is_open = 0;
			return(SI_FAIL);
		}
		handle = dlopen(name, RTLD_NOW);
    	if(handle == NULL) {
		ErrorF("dlopen <%s> failed\nReason: %s\n", name, dlerror());
	    closevt();
		is_open = 0;
	    return(SI_FAIL);
	}

	/*
	 * Set dl_flag = 1 upon successful open of shared library.
	 */
	dl_flag = 1;

	if(!cfgFormat) {
		/*
		 * old display module
		 */
		pDisplayFuncs = (ScreenInterface  **) dlsym(handle, "DisplayFuncs");
		if(pDisplayFuncs == NULL || *pDisplayFuncs == NULL) {
		    ErrorF("dlsym DisplayFuncs/DM_InitFunction in <%s> failed\nReason: %s\n",
			   name, dlerror());
		    dlclose(handle);
		    return(NULL);
		}
		HWroutines = *pDisplayFuncs;
		pSIScreen->funcsPtr = *pDisplayFuncs;
		return (SI_SUCCEED);
	}
	else {
		*init_func = (int) dlsym(handle, "DM_InitFunction");
	}
	return (SI_SUCCEED);
}

int
UnloadDisplayLibrary()
{
    if (handle == NULL)
      return(0);
	/*
	 * First check that library is open
	 */
	if (dl_flag == 1)
	{
    	if (dlclose(handle) != 0) 
		{
			fprintf(stderr,"dlclose failed, reason: %s\n",dlerror());
			return(-1);
    	}
		dl_flag = 0;
	}
    return(0);
}

#define DEFAULT_CONFIGFILE "/usr/X/defaults/Xwinconfig.ini";
#define DEFAULT_MODE "VGA STDVGA 640x480 16 9.75x7.32" 

/*
 * default configuration structure - built in
 */
SIConfig def_config = {
    "display",		/* resource */
    "VGA16",		/* class */
    2,			/* visual, StaticColor */
    DEFAULT_MODE, 	/* info2classlib */
    "0.0",		/* display */
    0,			/* displaynum */
    0,			/* screen number */
    "/dev/console",	/* device */
    "STDVGA",		/* chipset */
    512,		/* memory in K bytes */
    "VGA",		/* model / brand name */
    "/usr/X/lib/display/stdvga.so.2",	/* default shared library */
    640, 480,		/* virtual width, ht */
    640, 480,		/* display width, ht */
    4,			/* depth of the frame buffer */
    { "STDVGA", 10, 8, 31.5, 48.0 },	/* monitor info record */
    "",			/* info2vendorlib */
    "Standard VGA16 Display Module from USL",
    ""			/* private ptr */
};
SIConfig *pdefcfg = &def_config;

void
Error(str)
  char *str;
{
    perror(str);
}

ErrorF(str,a,b,c,d,e,f,g,h)
  char *str;
  int a,b,c,d,e,f,g,h;
{
    fprintf(stderr,str,a,b,c,d,e,f,g,h);
}

FatalError(str,a,b,c,d,e,f,g,h)
  char *str;
  int a,b,c,d,e,f,g,h;
{
    fprintf(stderr,"Fatal Error");
    fprintf(stderr,str,a,b,c,d,e,f,g,h);
    cleanexit(-1);
}

cleanexit(num)
  int num;
{
	if( is_open )
	{
    	closevt();
	}
    UnloadDisplayLibrary();
	#ifdef MAIN
		exit(num);
	#else
    	cleanexit1(num);
	#endif
}

int buzzer=0;

alarmHandler(sig)
  int sig;
{
    buzzer=1;
}

intHandler(sig)
  int sig;
{
    fprintf(stderr,"interrupt, exiting...\n");
    cleanexit(-1);
}

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stream.h>
#include <sys/fcntl.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <termio.h>
#include <sys/signal.h>

#define REL_SIG	SIGUSR1
#define ACQ_SIG SIGUSR2

int condev;
static int vtnum = 0, old_vtnum;

void
vt_sig_handler(sig)
  int sig;
{
    switch (sig) {
      case REL_SIG:
	if (si_VTSave() == -1 || ioctl(condev, VT_RELDISP, 1) == -1) {
	    Error("Unable to release vt");
	    si_VTRestore();
	}
	break;
      case ACQ_SIG:
	ioctl(condev, VT_RELDISP, VT_ACKACQ);
	if ( si_VTRestore() == SI_FATAL )
	  FatalError("Error restoring screen");
	break;
    }
}

int
openvt()
{
    char vtdev[20];		/* allow up to 20 chars in tty name */
    struct vt_mode vtmode;
    struct termio  ttyparms;
    struct vt_stat vtstat;
    int fd;

	alarm(20);			/* 20 seconds of real time */

    if ((fd = open("/dev/vt00", O_WRONLY)) < 0) {
        alarm( 0 );             /* turn off alarm */
        Error("Cannot open /dev/vt00");
        return(-1);
    }
    alarm(0);			/* turn off alarm */

    if (ioctl(fd, VT_GETSTATE, &vtstat) == -1) {
	Error("VT_GETSTATE");
    }
    old_vtnum = vtstat.v_active;

    if (ioctl(fd, VT_OPENQRY, &vtnum) < 0 || vtnum < 0) {
        Error("Cannot find a free VT");
        return(-1);
    }
    sprintf(vtdev, "/dev/vt%02d", vtnum);
    if (ioctl (fd, TCGETA, &ttyparms) < 0) {
        Error ("openvt:  TCGETA failed");
        return (-1);
    }
    if (ttyparms.c_lflag & (ICANON | ISIG) == 0) {
        ErrorF ("openvt:  /dev/console is in raw mode\n");
        return (-1);
    }

    close(fd);
    /* setpgrp(); */

    condev = open(vtdev, O_RDWR);
    if (condev < 0) {
        ErrorF("Cannot open device %s", vtdev);
        Error(" ");
        return(-1);
    }
    if (ioctl (condev, TCSETAW, &ttyparms) < 0) {
        Error ("openvt:  TCSETAW failed");
        return (-1);
    }
	#ifdef MAIN
    	sigset(SIGINT, intHandler);
	#endif
    sighold(REL_SIG); /* Hold till we are set up to service it. */
	/*
	 * FIX for not allowing user to switch VT while test pattern is
	 * displayed.
	 * Just ignore REL_SIG and ACQ_SIG.
	 */
#ifdef NOTNOW
    sigset(REL_SIG, vt_sig_handler);
    sigset(ACQ_SIG, vt_sig_handler);
#else
    sigset(REL_SIG, SIG_IGN);
    sigset(ACQ_SIG, SIG_IGN);
#endif

    if (ioctl(condev, VT_ACTIVATE, vtnum) == -1) {
        Error("VT_ACTIVATE failed");
    } else {
        if (ioctl(condev, VT_WAITACTIVE, 0) == -1)
	  Error("VT_WAITACTIVE failed");
    }


    vtmode.mode = VT_PROCESS;
#ifdef NOTNOW
    vtmode.relsig = REL_SIG;
#else
	/*
	 * Only in the case we decide to accept this method.
	 * Allowing user to terminate vtest.
	 */
    vtmode.relsig = SIGINT;
#endif
    vtmode.acqsig = ACQ_SIG;
    vtmode.frsig = REL_SIG;

    if (ioctl(condev, VT_SETMODE, &vtmode) == -1) {
        Error("VT_SETMODE VT_PROCESS failed");
        /* cleanexit(-1); */
    }

    if (ioctl(condev, KDSETMODE, KD_GRAPHICS) == -1) {
        Error("KDSETMODE KD_GRAPHICS failed");
        cleanexit (-1);
    }

	is_open = 1;
    return(condev);
}

closevt()
{
    struct vt_mode vtmode;

    sigignore (REL_SIG);	/* Tough luck, going bye-bye */
    ioctl(condev, KDQUEMODE, 0);
	/*
	 * Since we are setting HWroutines upon successful dl_open,
	 * Before using si_Restore we should also check if shared library
	 * is open or not.
	 */
    if ( (HWroutines) && (dl_flag == 1) ) {
	/* 
	fprintf(stderr,"Trying to restore vt through si_Restore\n"); 
	*/
	si_Restore();
	if (ioctl(condev, KDSETMODE, KD_TEXT) == -1)
	  Error("KDSETMODE KD_TEXT failed");

	vtmode.mode = VT_AUTO;
	vtmode.relsig = REL_SIG;
	vtmode.acqsig = ACQ_SIG;
	vtmode.frsig = REL_SIG;
	vtmode.waitv = 0;

	if (ioctl(condev, VT_SETMODE, &vtmode) == -1)
	  Error("VT_SETMODE VT_AUTO failed");
    }

#ifdef NOTNOW
    if (ioctl(condev, VT_ACTIVATE, old_vtnum) == -1) {
        Error("VT_ACTIVATE failed");
    }
#endif

    close(condev);
    condev = -1;
}


screenInit()
{
    sigrelse(REL_SIG);

    /*get_state();*/
}

static int depth,colors,delta,steps,pix;	
static int blackPixel,whitePixel,redPixel,greenPixel,bluePixel;
static int screenWidth,screenHeight;

/*
 * VGA16 (and may be other such older drivers also) doesn't have 
 * the mandatory entry point getcolormap. This flag is used to
 * workaround for such older drivers. (ex: libvga16.so.2) The StaticColor
 * visual flag is set by the driver but doesn't provide the required
 * getcolormap entry point for that visual. Actually it behaves like 
 * a PseudoColor visual. 
 */
int is_getcolormap_avail=1;

/* 
 * Ideally this should be set depending upon environment option
 */
int	do_msbitbltdemo=1;

static
load_color(index,red,green,blue)
  int index,red,green,blue;
{
    SIColor color;

    /*printf("load_color(%d,%d,%d,%d)\n",index,red,green,blue);*/
    color.SCpindex = index;
    color.SCred = red << 8;
    color.SCgreen = green << 8;
    color.SCblue = blue << 8;
    si_setcolormap(0,0,&color,1);
}

static int
is_basic_color(unsigned short color)
{

	if(color == 0)
	{
		return 0;
	}	
	/*
	 * Assumption:-
	 *	If the color component has contiguous ones, then the color is
	 *	a basic color.
	 */
	/*
	 * Look for the first occurence of a '1' in the color component
	 */
	while( !(color & 0x1) )
	{
		color >>=1;
	}

	/*
	 * Skip till a '0'
	 */
	while(color & 0x1)
	{
		color >>=1;
	}

	/*
	 * If there are no more '1's in the color component, then it is 
	 * a basic color.
	 */
	if(color==0)
	{
		return 1;
	}
	
	return 0;
	
}

static void
look_up_basic_colors(SIColor *colors_p, int count)
{
	unsigned int component_sum;
	int i;
	int j=0;
	int non_basic_colors[3] = {0, 0, 0};

	/*
	 * Put some dummy value, first. Useful later on, to check 
	 * whether these colors are found in the colormap.
	 */
	redPixel = greenPixel = bluePixel = 0;

	colors_p += 2;

	for(i=2; i<count; i++)
	{
		component_sum = colors_p->SCred + colors_p->SCgreen 
						+colors_p->SCblue;
		/*
		 * Skip Black.
		 */
		if(component_sum != 0)
		{
				if(component_sum == colors_p->SCred)
				{
					if( is_basic_color(colors_p->SCred) )
					{
						redPixel = i;
					}
				}
				else if(component_sum == colors_p->SCgreen)
				{
					if( is_basic_color(colors_p->SCgreen) )
					{
						greenPixel = i;
					}
				}
				else if(component_sum == colors_p->SCblue)
				{
					if( is_basic_color(colors_p->SCblue) )
					{
						bluePixel = i;
					}
				}
				else
				{
					/*
					 * Save first 3 non-basic colors. Just in case not able to
					 * find all the basic colors, these can be used.
					 */
					if( j < 3)
					{
						non_basic_colors[j] = i;
						j++;
					}
				}
		}
		colors_p++;
	}
	/*
	 * Use some colors in case required basic colors are not available
	 * in the colormap to draw the test pattern.
	 */
	j = 0;
	if(redPixel==0)
	{
		redPixel = non_basic_colors[j];
		j++;
	}
	if(greenPixel==0)
	{
		greenPixel = non_basic_colors[j];
		j++;
	}
	if(bluePixel==0)
	{
		bluePixel = non_basic_colors[j];
		j++;
	}
}

static
init_graphics()
{
    SIColor demoColors[255];
    SIColor staticColors[255];
	SIColor *colors_p;
    int	  demoGSFlags;
    SIGState  demoGState;
    int ii,pix;
    extern SIScreenRec	*pSIScreen;

    screenWidth = si_getscanlinelen;
    screenHeight = si_getscanlinecnt;

    /*printf("width=%d, height=%d\n",screenWidth,screenHeight);/**/
    depth = pSIScreen->flagsPtr->SIvisuals->SVdepth;
    colors = 1 << depth;

	/*
	 * minor hack: 1<<32 gives unpredictable result + in almost all the
	 * the 32-bit cases, the actual depth of the frame buffer is 24 as of 1994.
	 */
	if (depth == 32)
    	colors = 1 << 24;

	if(pSIScreen->flagsPtr->SIvisuals->SVtype == TRUECOLOR_AVAIL)
	{
		blackPixel	= 0;
    	whitePixel 	= colors-1 ;
    	whitePixel 	= 0xFFFFFFFF &
			( pSIScreen->flagsPtr->SIvisuals->SVredmask |
			pSIScreen->flagsPtr->SIvisuals->SVgreenmask |
			pSIScreen->flagsPtr->SIvisuals->SVbluemask ); 
		redPixel	= whitePixel& (pSIScreen->flagsPtr->SIvisuals->SVredmask); 
		greenPixel	= whitePixel& (pSIScreen->flagsPtr->SIvisuals->SVgreenmask);
		bluePixel	= whitePixel& (pSIScreen->flagsPtr->SIvisuals->SVbluemask); 
	}	
	else if(pSIScreen->flagsPtr->SIvisuals->SVtype == STATICCOLOR_AVAIL)
	{
		int i;
		SIBool tmp;
		/*
		 * This first two indices have to be black and white respectively
		 * in this* visual.
		 */ 
		blackPixel	= 0;
    	whitePixel 	= 1;
   		/*
		 * The full colormap has to be gotten, from which pixel indices
		 * with the basic colors if available, should be found.
		 */
		colors_p = staticColors;
		for(i=0; i<colors; i++)
		{
			colors_p->SCpindex = i;
			colors_p++;
		}
		tmp = si_getcolormap(0, 0, staticColors, colors);

		if( tmp == SI_FAIL || tmp == SI_FALSE )
		{
			is_getcolormap_avail = 0;
		}
		else
		{
			look_up_basic_colors(staticColors, colors);
		}
	}
	else if(pSIScreen->flagsPtr->SIvisuals->SVtype == DIRECTCOLOR_AVAIL)
	{
    	SIColor color;
		int	numreds, numgreens, numblues;
		int redmask, greenmask, bluemask;
		int i;

		redmask = pSIScreen->flagsPtr->SIvisuals->SVredmask;
		greenmask = pSIScreen->flagsPtr->SIvisuals->SVgreenmask;
		bluemask = pSIScreen->flagsPtr->SIvisuals->SVbluemask;

		numreds = ((unsigned)pSIScreen->flagsPtr->SIvisuals->SVredmask >> 
				pSIScreen->flagsPtr->SIvisuals->SVredoffset) + 1;
		numgreens = ((unsigned)pSIScreen->flagsPtr->SIvisuals->SVgreenmask >> 
				pSIScreen->flagsPtr->SIvisuals->SVgreenoffset) + 1;
		numblues = ((unsigned)pSIScreen->flagsPtr->SIvisuals->SVbluemask >> 
				pSIScreen->flagsPtr->SIvisuals->SVblueoffset) + 1;


		for (i = 0 ; i < numreds ; ++i)
		{
			color.SCpindex = i;
			color.SCred = 65536 * i / numreds;
			color.SCvalid = VALID_RED;
			si_setcolormap(0,0,&color,1);
		}
		for (i = 0 ; i < numgreens ; ++i)
		{
			color.SCpindex = i;
			color.SCgreen = 65536 * i / numgreens;
			color.SCvalid = VALID_GREEN;
			si_setcolormap(0,0,&color,1);
		}
		for (i = 0 ; i < numblues ; ++i)
		{
			color.SCpindex = i;
			color.SCblue = 65536 * i / numblues;
			color.SCvalid = VALID_BLUE;
			si_setcolormap(0,0,&color,1);
		}

		blackPixel	= 0;
    	whitePixel 	= redmask | greenmask | bluemask;
		redPixel	= redmask;
		greenPixel	= greenmask;
		bluePixel	= bluemask;
	}

	/*
	 * Workaround for older drivers
	 * Fallback to PseudoColor visual,
	 *	if (visual_type == StaticColor) && 
	 *		NO_entry_for "getcolormap" entry point. 
	 */
	if( ((pSIScreen->flagsPtr->SIvisuals->SVtype != TRUECOLOR_AVAIL)    &&
	     (pSIScreen->flagsPtr->SIvisuals->SVtype != STATICCOLOR_AVAIL) &&
	     (pSIScreen->flagsPtr->SIvisuals->SVtype != DIRECTCOLOR_AVAIL)) ||
		(is_getcolormap_avail == 0))
	{
		steps = colors/4;
		delta = 256 / steps;
		/*printf("depth=%d, colors=%d, steps=%d, delta=%d\n",
		   depth,colors,steps,delta);/**/

		for (ii=0; ii < steps; ++ii) {
		pix = ii*delta;
		if (pix == 256)
		  pix = 255;
		load_color(ii,pix,pix,pix);
		load_color(ii+steps,pix,0,0);
		load_color(ii+2*steps,0,pix,0);
		load_color(ii+3*steps,0,0,pix);
		}
		
		blackPixel = 0;
    	whitePixel = steps - 1;
    	redPixel   = steps*2 - 1;
    	greenPixel = steps*3 - 1;
    	bluePixel  = steps*4 - 1;
	}

    demoGSFlags = SetSGpmask | SetSGmode | SetSGfg | SetSGbg | 
			SetSGfillmode | SetSGlinestyle;
    demoGState.SGpmask = -1;
    demoGState.SGmode = GXcopy;
    demoGState.SGfg = whitePixel;
    demoGState.SGbg = blackPixel;
    demoGState.SGfillmode = SGFillSolidFG;
    demoGState.SGlinestyle = SGLineSolid;
    si_downloadstate(0,demoGSFlags,&demoGState);
    si_selectstate(0);

    if (si_hasanycliplist) {
	if (si_setpolyclip)
	  si_setpolyclip(0,0,screenWidth,screenHeight);
	if (si_setlineclip)
	  si_setlineclip(0,0,screenWidth,screenHeight);
	if (si_setdrawarcclip)
	  si_setdrawarcclip(0,0,screenWidth,screenHeight);
	if (si_setfillarcclip)
	  si_setfillarcclip(0,0,screenWidth,screenHeight);
	if (si_fontclip)
	  si_fontclip(0,0,screenWidth,screenHeight);
    }
}

static
set_fg(fg)
  int fg;
{
    int	  demoGSFlags;
    SIGState  demoGState;

    demoGSFlags = SetSGfg;
    demoGState.SGfg = fg;

    si_downloadstate(0,demoGSFlags,&demoGState);
    si_selectstate(0);
}

static
fill_rect(x1,y1,x2,y2)
  int x1,y1,x2,y2;
{
    if (si_canpolyfill) {
		if (pSIScreen->flagsPtr->SIdm_version == 0)
		{

			SIRect	  demoRect;
			demoRect.ul.x = x1;
			demoRect.ul.y = y1;
			demoRect.lr.x = x2;
			demoRect.lr.y = y2;
			si_1_0_fillrectangle(1,&demoRect);
		}
		else {
			/* supports SI 1.1 */
    			SIRectOutline newRect; 
			newRect.x = x1;
			newRect.y = y1;
			newRect.width = x2 - x1 + 1;
			newRect.height = y2 - y1 + 1;
			si_fillrectangle(0,0, 1, &newRect);
		}
    }
}

static
draw_line(x1,y1,x2,y2)
  int x1,y1,x2,y2;
{
    SIPoint demoPoints[2];
	extern SIScreenRec *pSIScreen;

    demoPoints[0].x = x1;
    demoPoints[0].y = y1;
    demoPoints[1].x = x2;
    demoPoints[1].y = y2;

    if (si_haslinedraw) {
		/*
		 * old DM's - pass the arguments in the old order
		 */
		if (pSIScreen->flagsPtr->SIdm_version == 0)
			si_1_0_onebitlinedraw(2,demoPoints);
		else
			si_onebitlinedraw( 0, 0, 2, demoPoints, 
					0 /*isCapNotLast*/, SICoordModeOrigin );
    }
}

#define BDR1  10
#define BDR2  15 /* including BDR1 */

#include"bitmap.h"


SIbitmap bmap = {
Z_BITMAP,
1,
BWIDTH,
BHEIGHT,
0,
0,
(SIArray)bdata,
NULL
};


test_pattern()
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n,nn;
    char ch;

    x1 = BDR2;
    y1 = BDR2;
    x2 = screenWidth-BDR2;
    y2 = screenHeight-BDR2;
    dx = x2 - x1;

    set_fg(whitePixel);

	if(pSIScreen->flagsPtr->SIvisuals->SVtype == TRUECOLOR_AVAIL ||
	  pSIScreen->flagsPtr->SIvisuals->SVtype == DIRECTCOLOR_AVAIL)
	{
		#define BLOCKS_CNT	3
		int blk_size, block_num, bands_per_blk, band_size, fg;
		int r_val, g_val, b_val, r_incr, g_incr, b_incr;
		int num_reds, num_greens, num_blues, min_colors;
		int tmp, cnt=0;

			blk_size   		= (y2-y1+1)/BLOCKS_CNT;

			num_reds 	= ((pSIScreen->flagsPtr->SIvisuals->SVredmask >> 
							pSIScreen->flagsPtr->SIvisuals->SVredoffset)+1);

			num_greens 	= ((pSIScreen->flagsPtr->SIvisuals->SVgreenmask >> 
							pSIScreen->flagsPtr->SIvisuals->SVgreenoffset)+1);

			num_blues 	= ((pSIScreen->flagsPtr->SIvisuals->SVbluemask >> 
							pSIScreen->flagsPtr->SIvisuals->SVblueoffset)+1);

			min_colors = (num_reds < num_greens) ? num_reds : num_greens;
			min_colors = (min_colors < num_blues) ? min_colors : num_blues;

			band_size	= (blk_size-1)/(min_colors-1);
			if (band_size < 1) band_size = 1;

			bands_per_blk	= (blk_size-1)/(band_size+1);

			r_incr= num_reds/bands_per_blk;
			g_incr= num_greens/bands_per_blk;
			b_incr= num_blues/bands_per_blk;
		
			tmp = (bands_per_blk/2.)+0.5;

			if((num_reds%bands_per_blk) >= tmp)
				cnt++;

			if((num_greens%bands_per_blk) >= tmp)
				cnt++;

			if((num_blues%bands_per_blk) >= tmp)
				cnt++;

			if(cnt>=2)
			{
				r_incr++;
				g_incr++;
				b_incr++;
			}

			for(block_num=0; block_num<BLOCKS_CNT; block_num++)
			{
				r_val = g_val = b_val =0;

				for(xx=(y1+block_num*blk_size);
					xx<=(y1-1+(block_num+1)*blk_size); xx+=band_size)
				{
					switch(block_num)
					{
					case 0:
					{
						fg=(r_val<<pSIScreen->flagsPtr->SIvisuals->SVredoffset) 
					 	+(g_val<<pSIScreen->flagsPtr->SIvisuals->SVgreenoffset) 
						+(b_val<<pSIScreen->flagsPtr->SIvisuals->SVblueoffset);

						r_val+=r_incr;
						g_val+=g_incr;
						b_val+=b_incr;
						break;
					}
					case 1:
					{
						fg=(r_val<<pSIScreen->flagsPtr->SIvisuals->SVredoffset);

						r_val+=r_incr;
						break;
					}
					case 2:
					{
					fg=(g_val<<pSIScreen->flagsPtr->SIvisuals->SVgreenoffset);
						g_val+=g_incr;
						break;
					}
					}
					set_fg(fg);
					fill_rect(x1,xx, x2,xx+band_size);

					if(r_val >= num_reds)
						r_val = num_reds-1;
					if(g_val >= num_greens)
						g_val = num_greens-1;
					if(b_val >= num_blues)
						b_val = num_blues-1;
				}
			}
	}
	else if(pSIScreen->flagsPtr->SIvisuals->SVtype == STATICCOLOR_AVAIL)
	{
		int start_y, end_y;

		start_y = y1;
		end_y = start_y + (y2-y1)/3;
		set_fg(whitePixel);
		fill_rect(x1, start_y, x2, end_y);

		start_y = end_y +1;
		end_y = start_y + (y2-y1)/3;
		set_fg(redPixel);
		fill_rect(x1, start_y, x2, end_y);

		start_y = end_y +1;
		set_fg(greenPixel);
		fill_rect(x1, start_y, x2, y2);
	}
	
	if( ((pSIScreen->flagsPtr->SIvisuals->SVtype != TRUECOLOR_AVAIL)    &&
	     (pSIScreen->flagsPtr->SIvisuals->SVtype != STATICCOLOR_AVAIL) &&
	     (pSIScreen->flagsPtr->SIvisuals->SVtype != DIRECTCOLOR_AVAIL)) ||
		(is_getcolormap_avail == 0))
	{
			for (xx=y1; xx < y2; xx+=8) {
				
				set_fg((colors-1)*(xx-y1) / dx);
				fill_rect(x1,xx, x2,xx+8);
			}
	}

    set_fg(whitePixel);
    draw_line(x1,y1,x1,y2);
    draw_line(x1,y1,x2,y1);
    draw_line(x1,y2,x2,y2);
    draw_line(x2,y1,x2,y2);

    {
	int	  demoGSFlags;
	SIGState  demoGState;

	demoGSFlags = SetSGmode;
	demoGState.SGmode = GXxor;
	si_downloadstate(0,demoGSFlags,&demoGState);
	si_selectstate(0);

	dx = 32;
	for (xx=x1; xx < x2; xx += dx) {
		draw_line(xx,y1,xx,y2);
	}
	dy = 32;
	for (yy=y1; yy < y2; yy += dy) {
		draw_line(x1,yy,x2,yy);
	}
	demoGSFlags = SetSGmode;
	demoGState.SGmode = GXcopy;
	si_downloadstate(0,demoGSFlags,&demoGState);
	si_selectstate(0);
    }

    set_fg(whitePixel);
    draw_line(0,0,screenWidth,screenHeight);
    draw_line(screenWidth,0,0,screenHeight);
    draw_line(0,screenHeight/2,screenWidth,screenHeight/2);
    draw_line(screenWidth/2,0,screenWidth/2,screenHeight);
}

display_resoln()
{
	int vfreq_fonts_avail = 1;

	if (do_msbitbltdemo)
	{
		if (pflags->SIavail_stplblt)
		{
			int		i,j, x_left, y_top, tot_width, width, height;
			int		index[MODE_DISP_WIDTH];
			unsigned char padded_bmap_p[BSIZE_BYTES * MAX_BWIDTH / BWIDTH];
			int refresh_rate=0;
			char str[MODE_DISP_WIDTH];
			int len;
			int		ind, startx, starty;
			int		blast_stipple=0;
	
			height = 2 * BHEIGHT;
			width  = BWIDTH + INTER_GLYPH_SPACE ;
			if(cfgFormat == 0)
			{
				if(pSIScreen->cfgPtr->info)
				{
				char *monitor_field;
					monitor_field = 
						(char *)malloc(strlen(pSIScreen->cfgPtr->info)+1);
					sscanf(pSIScreen->cfgPtr->info, "%*s %s", monitor_field);
					if(strrchr(monitor_field, '_')!=NULL)
					{
						refresh_rate = atoi(strrchr(monitor_field, '_')+1);
					}
					free(monitor_field); 
				}
			}
			else
			{
				SIMonitor *mon;
				char *pm;

				mon = &pSIScreen->cfgPtr->monitor_info;
				pm = (char *)mon->model;
				if (mon->vfreq)
					refresh_rate = mon->vfreq;
				else if (strrchr(pm, '_')!=NULL)
						refresh_rate = atoi(strrchr(pm, '_')+1);
			}

			if( (strchr(map, '@')==NULL)||(strchr(map, 'i')==NULL)||(strchr(map, 'H')==NULL)||(strchr(map, 'z')==NULL))
			{
				vfreq_fonts_avail = 0;
			}

			if((refresh_rate > 0)&&(vfreq_fonts_avail==1))
			{
				/*
			     * Lousy assumption: if > 85.0, then we assume it is
				 * interlaced
				 */
				if (refresh_rate > 85.0)
					sprintf(str, " %dx%dx%d @%di Hz ", screenWidth,
						screenHeight, depth, refresh_rate);
				else
					sprintf(str, " %dx%dx%d @%d Hz ", screenWidth,
						screenHeight, depth, refresh_rate);
			}
			else
			{
				sprintf(str, " %dx%dx%d ", screenWidth, screenHeight, depth);
			}
			len = strlen(str);
			for(i=0; i<len; i++)
			{
				for(j=0; (str[i]!=map[j])&&(j< NO_OF_BITMAPS) ; j++)
				{
					;
				}
				if(j== NO_OF_BITMAPS)
				{
					index[i] = NO_OF_BITMAPS - 1;
				}
				else
				{
					index[i] = j;
				}
			}

			blast_stipple=1;
			tot_width = width * len;
			if (blast_stipple == 1)
			{
				tot_width += 2 * INTER_GLYPH_SPACE;
				set_fg(blackPixel);
				x_left = (screenWidth/2) - (tot_width/2);
				y_top = (screenHeight/2) - BHEIGHT;
		    	if (si_canpolyfill) 
				{
					if (pSIScreen->flagsPtr->SIdm_version == 0)
					{
		
						SIRect	  demoRect;
						demoRect.ul.x = x_left;
						demoRect.ul.y = y_top;
						demoRect.lr.x = x_left + tot_width;
						demoRect.lr.y = y_top + height;
						si_1_0_fillrectangle(1,&demoRect);
					}
					else 
					{
		    			SIRectOutline newRect; 
						newRect.x = x_left;
						newRect.y = y_top;
						newRect.width = tot_width;
						newRect.height = height;
						si_fillrectangle(0,0, 1, &newRect);
					}
		    	}
				set_fg(whitePixel);
				/*
				 * Now blit the stipple
				 */
				starty = y_top + ((height - BHEIGHT)/2);
				for (i=0; i<len; i++)
				{
					ind = index[i];
					/*
					 * Read in the appropriate array.
					 */
					bmap.Bptr = (SIArray)&(bdata[ind][0]);
					startx = x_left + INTER_GLYPH_SPACE;
					if( BWIDTH < MAX_BWIDTH )
					{
						(void)memset(padded_bmap_p, 0, BSIZE_BYTES*MAX_BWIDTH/BWIDTH);
						(void)memcpy(padded_bmap_p, bdata[ind], BSIZE_BYTES);
						bmap.Bptr = (SIArray)&(padded_bmap_p[0]);
					}
					else
					bmap.Bptr = (SIArray)&(bdata[ind][0]);

					si_MSstplblt(&bmap, 0, 0, startx, starty, BWIDTH, BHEIGHT, 0, SGStipple);
					x_left += BWIDTH;
					x_left += INTER_GLYPH_SPACE;
				}
			}
		}
	}
}

void
BitBltDemo(int screenWidth, int screenHeight, int num)
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n,nn;

	if (num < 1)
	{
		return;
	}

	x1 = screenWidth/16;
	y1 = screenHeight/16;
	x2 = 7*screenWidth/16;
	y2 = 7*screenHeight/16;

	set_fg(blackPixel);
	fill_rect(x1,y1,x2,y2);

	w = screenWidth/16;
	h = screenHeight/16;

	xx=x1;
	yy=y1;

	si_SSbitblt((screenWidth-w)/2,(screenHeight-h)/2,xx,yy,w,h);
	/*
	si_SSbitblt((screenWidth-w),(screenHeight-h)/2,xx,yy,w,h);
	*/

	dx=3;
	dy=1;

	for (n=0; n < num; ++n) 
	{
		if ((xx+dx < x1) || (xx+dx+w > x2))
		  dx = -dx;
		if ((yy+dy < y1) || (yy+dy+h > y2))
		  dy = -dy;

		si_SSbitblt(xx,yy,xx+dx,yy+dy,w,h);

		xx += dx;
		yy += dy;
	}
}

void
LinesDemo(int screenWidth, int screenHeight, int num)
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n,nn;

	if (num < 1)
	{
		return;
	}

	/* 
	 * upper right corner
	 */
	x1 = 9*screenWidth/16;
	y1 = screenHeight/16;
	x2 = 15*screenWidth/16;
	y2 = 7*screenHeight/16;
#if 0
	/*
	 * make it center of screen
	 */
	x1 = 6*screenWidth/16;
	y1 = 6*screenHeight/16;
	x2 = 10*screenWidth/16;
	y2 = 10*screenHeight/16;
#endif

	set_fg(blackPixel);
	fill_rect(x1,y1,x2,y2);

	dx = x2 - x1;
	dy = y2 - y1;

	/* srandom(0); */
	srand(0);
	set_fg(redPixel);

	for (n=0; n < num; ++n)
	{
		set_fg(n%colors);
		draw_line(x1+(rand()%dx),y1+(rand()%dy),
			  x1+(rand()%dx),y1+(rand()%dy));
	}
}

void
ScrollDemo(int screenWidth, int screenHeight, int num)
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n,nn;

	if (num < 1)
	{
		return;
	}

	x1 = screenWidth/16;
	y1 = 9*screenHeight/16;
	x2 = 7*screenWidth/16;
	y2 = 15*screenHeight/16;

	set_fg(whitePixel);
	fill_rect(x1,y1,x2,y2);

	w = 6*screenWidth/16;
	h = 6*screenHeight/16;

	dx=5;
	dy=7;

	xx=x1 + dx;
	yy=y2 - dy;

	for (n=0; n < num; ++n)
	{
		set_fg(blackPixel);
		for (xx=x1+dx, nn=rand()%(w/5); nn; --nn, xx += dx)
		  fill_rect(xx,yy,xx+dx-1,yy+dy-1);
		  
		si_SSbitblt(x1,y1+dy,x1,y1,w,h-dy);

		set_fg(whitePixel);
		fill_rect(x1,y2-dy,x2,y2);
	}
}

#if 0
/* THIS FUNCTION IS NOT DONE YET - 9/27/94 */
void
FillDemo(int screenWidth, int screenHeight, int num)
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n,nn;


	if (num < 1)
	{
		return;
	}

	x1 = 9*screenWidth/16;
	y1 = 9*screenHeight/16;
	x2 = 15*screenWidth/16;
	y2 = 15*screenHeight/16;

	set_fg(whitePixel);
	fill_rect(x1,y1,x2,y2);

	w = 6*screenWidth/16;
	h = 6*screenHeight/16;

	dx=5;
	dy=7;

	xx=x1 + dx;
	yy=y2 - dy;

	for (n=0; n < num; ++n)
	{
		set_fg(blackPixel);
		for (xx=x1+dx, nn=rand()%(w/5); nn; --nn, xx += dx)
		  fill_rect(xx,yy,xx+dx-1,yy+dy-1);

		set_fg(whitePixel);
		fill_rect(x1,y2-dy,x2,y2);
	}
}
#endif

int cx = 320;
int cy = 240;
int incr = 1;
int incr2 = 20;

draw_rect (x, y, w, h)
 int x, y, w, h;
{
	int x2, y2;

	x2 = x+w-1;
	y2 = y+h-1;
	
	draw_line (x, y, x2, y);
	draw_line (x2, y, x2, y2);
	draw_line (x2, y2, x,  y2);
	draw_line (x, y2, x, y);
}

#ifdef DELETE
alt_pattern()
{
    int xx,yy,x1,x2,y1,y2,dx,dy,h,w,n;
    char ch;
	int width, ht;
	int i;
	int cols[] = {2, 6, 10, 13};

	x1 = BDR2;
	y1 = BDR2;
    x2 = screenWidth - BDR2;
    y2 = screenHeight - BDR2;
	width = screenWidth - BDR2; 
	ht = screenHeight - BDR2;
	dx = x2 - x1;

	for (i=BDR2; i<(ht); i+=4) {
		set_fg((colors-1)*(i-BDR2) / dx);
/*******
		x1 = i;
		y1 = i; 
		fill_rect (x1, y1, x2, y2);
		x2 -= 8;
		y2 -= 8;
**********/
		fill_rect (x1, i, x2, i+4);
/*
		fill_rect (x1, y1, x2, y2);
		x1 += 8;
		y1 += 8;
		x2 -= 8;
		y2 -= 8;
*/
	}
    sleep(2);
}
#endif

#ifndef MAIN
testmode(cfgfile)
  char *cfgfile;
{
#else

main (argc, argv)
 int argc;
 char **argv;
{
    char *cfgfile=NULL;
#endif
    FILE *fp = (FILE *)0;
    char *ptrs[MAXARGS];
    int  num = 0;
    int	 ret = 0;
    char inbuf[MAXLINE], tmpbuf[MAXLINE];
	SIConfig *pconfig;
    int (*initFunc)();
	char	*ep = (char *) getenv("VTEST_DEMO");

    signal(SIGALRM, alarmHandler);

    if (openvt() == -1) {
		return (FAIL);
    }

#ifdef MAIN
    if (argc>1)
    {
	if (!strcmp(argv[1],"-?") || !strcmp(argv[1],"-help") )
	{
		printf ("Usage: vtest <config_file>\n");
		exit(-1);	
	}
	cfgfile = argv[1];
	printf ("Using config file: %s\n", cfgfile);
    }
#endif

    if (cfgfile == NULL) {
	cfgfile = DEFAULT_CONFIGFILE;
	printf ("No configuration file specified.\n");
	printf ("Using default config file: %s\n", cfgfile);
    }

    if ((fp = fopen (cfgfile,"r")) == (FILE *)0) 
    {
		printf ("There is no current Xwinconfig file.\n");
		return (FAIL);
    }

    if (cfgFormat)
		num = r_configfile (fp, pSIScreen, 0);  /* NEW format */
	else
		num = r_oldconfigfile (fp, pSIScreen, 0);  /* OLD format */

    fclose (fp);

    if (!num) 
	{
		printf("Cannot read config file: %s\n", cfgfile);
		return(-1);
    }

	if(pSIScreen->cfgPtr->vendor_lib == NULL)
	{
		ErrorF("\nInvalid vendor_lib specified.\n");
		closevt();
		is_open = 0;
		exit(-1);
	}

	if( strchr(pSIScreen->cfgPtr->vendor_lib, '/') )
	{
		ErrorF("\nInvalid vendor_lib <%s> specified.\n",
							pSIScreen->cfgPtr->vendor_lib);
		ErrorF("Paths other than LD_RUN_PATH and LD_LIBRARY_PATH\n");
		ErrorF("are not allowed.\n\n");
		closevt();
		is_open = 0;
		exit(-1);
	}

    pconfig = pSIScreen->cfgPtr;
    pflags = pSIScreen->flagsPtr;
    initFunc = 0;
    if( !LoadDisplayLibrary(pconfig->vendor_lib, &initFunc) ) 
	{
		printf("Cannot dlopen library: %s\n", pconfig->vendor_lib);
		closevt();
		is_open = 0;
		return(-1);
    }

	/*
	 * This field should be set only upon successful dl_open 
	 * of  shared library.
	 */
	HWroutines = pSIScreen->funcsPtr;

    pconfig->IdentString = NULL;
    pflags->SIdm_version = 0;

    /*
     * call the device-specific initialization routine 
     */
    if (cfgFormat)
    {
		pflags->SIserver_version = X_SI_VERSION1_1;
		/* SI 1.1 compliant */
    	ret=(*initFunc) (condev, pSIScreen); 
    }
    else
    {
		pflags->SIserver_version = 0;
		/* SI 1.0 compliant */
    	ret=(pSIScreen->funcsPtr->si_init) (condev,pSIScreen->cfgPtr,
                                   pSIScreen->flagsPtr,
                                   &HWroutines);
		pSIScreen->funcsPtr = HWroutines; 
    }

	if (!ret) 
	{
		printf ("Error detected at display initialization time.\n");
		if (pflags->SIdm_version > 0x0100) 
		{
			printf ("Screen %d :\n    Display Class: %s\n",
				pconfig->screen, pconfig->class);
			printf ("    Vendor Library: %s\n", pconfig->vendor_lib);
			printf ("    Vendor String: %s\n", pconfig->IdentString);
			printf ("     Display Module SI version: %x\n",
				pflags->SIdm_version);
				/* DO WE really want to print out all this info?
				 * Is there a guarantee, that some of these fields
				 * are initialized properly if si_init fails ?
				 * LOOK INTO THIS LATER 3/11/93
				 */
		}
		else 
		{
			printf ("Display Class: %s\n, SI version: %x\n", 
			pconfig->class, pflags->SIdm_version);
		}

		closevt();
		is_open = 0;
		UnloadDisplayLibrary();
		return (ret);
	}

    /*
     * we succeeded initializing....
     */
    screenInit();
    init_graphics();
    set_fg(whitePixel);

    fill_rect(0,0,screenWidth,screenHeight);
    set_fg(blackPixel);	
    fill_rect(BDR1,BDR1,screenWidth-BDR1,screenHeight-BDR1);

    test_pattern();		/* draws background test pattern */

	num = 0;
	if (ep != NULL)
	{
		num = atoi(ep);
		if (num < 1) num = 0;
	}

	BitBltDemo(screenWidth, screenHeight, num);
	display_resoln();	/* displays the resolution in the center of the scr */
	LinesDemo(screenWidth, screenHeight, num);
	ScrollDemo(screenWidth, screenHeight,((num>>4) > 0 ? (num>>4) : 0));
	/* FillDemo(screenWidth, screenHeight, num); */
	sleep(DURATION);

    closevt();
	is_open = 0;
    UnloadDisplayLibrary();
    return(1);
}
