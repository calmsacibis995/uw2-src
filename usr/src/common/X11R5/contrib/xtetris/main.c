/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:main.c	1.3"
#endif

static char * aboutstring = 
"Copyright 1992, \n\
Dan R. Greening, Didier Tallot, Phill Everson, and Martyn Shortley.\n\
\n\
Permission to use, copy, modify, and distribute this program for any\n\
purpose and without fee is hereby granted, provided that the above\n\
copyright notice and this permission notice appear in all copies, and\n\
that the about box (which contains the copyright and credits the\n\
authors) is retained verbatim.  The names of the authors may not be\n\
used in advertising or publicity pertaining to this document without\n\
specific, written prior permission.  The authors make no\n\
representations about the suitability of this document for any\n\
purpose.  It is provided ``as is'' without expressed or implied\n\
warranty.\n\
\n\
We invite communication.  Since this program is provided free of charge,\n\
please be polite in documenting bugs or missing features.\n\
\n\
E-mail:		<xtetris@cs.ucla.edu>";


#include <X11/IntrinsicP.h>
#include "defs.h"

static XtActionsRec actions[] = {
  {"Refresh", restore_widget},
  {"ShowScore", show_score},
  {"Quit", quit_proc},
  {"Done", done_proc},
  {"EndGame", end_game},
  {"NewGame", newgame_proc},
  {"Resume", resume_proc},
  {"Start", start_proc},
  {"Pause", pause_proc},
  {"MoveLeft", left_proc},
  {"MoveRight", right_proc},
  {"RotateCW", clock_proc},
  {"RotateCCW", anti_proc},
  {"Drop", fast_proc},
  {"About", about_proc},
  {"Scores", print_high_scores},
};

#define offset(field) XtOffset(struct resource_struct *, field)
#define soffset(field) XtOffset(struct shape_table *, field)

static XtResource Resources[] = {
  {"foreground",	"Foreground",	XtRPixel,	sizeof(Pixel),
     offset(foreground),	XtRString,	XtDefaultForeground},
  {"background",	"Background",	XtRPixel,	sizeof(Pixel),
     offset(background),	XtRString,	XtDefaultBackground},
  {"eraseStipple","Bitmap",	XtRBitmap,	sizeof(Pixmap),
     offset(erasestipple),	XtRString,	"gray"},
  {"useScoreFile",	"Boolean",	XtRBoolean,	sizeof(Boolean),
     offset(usescorefile),	XtRImmediate,	(XtPointer)True},
  {"customization",	"Customization",	XtRString,	sizeof(String),
     offset(customization),	XtRString,	".bw"},
  {"boxSize",	"BoxSize",	XtRDimension,	sizeof(Dimension),
     offset(boxsize),	        XtRImmediate,	(XtPointer)16},
  {"speed",       "Speed",        XtRDimension,   sizeof(Dimension),
     offset(speed),             XtRImmediate,      (XtPointer)STANDARD_SPEED},
  {"scorefile",    "ScoreFile",   XtRString,      sizeof(String),
     offset(scorefile),         XtRString,      HIGH_SCORE_TABLE},
  {"boxesHigh",    "BoxesHigh",   XtRInt,      sizeof(int),
     offset(boxes_high),         XtRImmediate,      (XtPointer)25},
  {"boxesWide",    "BoxesWide",   XtRInt,      sizeof(int),
     offset(boxes_wide),         XtRImmediate,      (XtPointer)10},
};
  
static XrmOptionDescRec Options[] = {
  {"-score",	"useScoreFile",	 	      XrmoptionNoArg,	"TRUE"},
  {"-noscore",	"useScoreFile",	 	      XrmoptionNoArg,	"FALSE"},
  {"-speed",    "speed",                      XrmoptionSepArg,  NULL },
  {"-boxsize",  "boxSize",                    XrmoptionSepArg,  NULL },
  {"-color",	"customization",	      XrmoptionNoArg,	".c" },
  {"-bw",	"customization",	      XrmoptionNoArg,	".bw" },
  {"-boxeshigh","boxesHigh",		      XrmoptionSepArg,	NULL },
  {"-boxeswide","boxesWide",		      XrmoptionSepArg,	NULL },
};

static XtResource ShapeResources[] = {
  {"foreground",  "Foreground",   XtRPixel,       sizeof(Pixel),
     soffset(foreground),         XtRString,      XtDefaultForeground},
  {"background",	"Background",	XtRPixel,	sizeof(Pixel),
     soffset(background),	XtRString,	XtDefaultBackground},
};

static char * fallback_resources[] = {
  "customization: .bw",
  "*TitleBar.Font:	-*-clean-*-*-*-*-10-*-*-*-*-*-*-*",
  "*TitleBar.Label:	XTETRIS 2.5 REQUIRES INSTALLATION OF APP-DEFAULTS OR SET YOUR XUSERFILESEARCHPATH TO .../xtetris-2.5/%N%C.ad",
  "*TitleBar.height:	60",
  "*TitleBar.width:	700",
  NULL,
};

#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>

int main(argc, argv, envp)
  int     argc;
  char  **argv;
  char  **envp;
{
  int     i, j;
  Atom wm_delete_window;
  Widget 
    status, buttons, nextobjectlabel,
    tbar, scores_bt, quit_bt, about_bt;
  static XtConvertArgRec screenConvertArg[] = {
    {XtWidgetBaseOffset, (caddr_t) XtOffset(Widget, core.screen),
       sizeof(Screen *)}
  };
  XVisualInfo vi;
  int screenno;
  Display * display;
  XrmDatabase db;
  
  programname = argv[0];
  XtToolkitInitialize();
  context = XtCreateApplicationContext();
  display = XtOpenDisplay( context, NULL, NULL, "Xtetris", Options, XtNumber(Options),
		&argc, argv );

  /* Get a default customization. */

  screenno = DefaultScreen(display);
  if (XMatchVisualInfo( display, screenno, 4, PseudoColor, &vi )
      || XMatchVisualInfo( display, screenno, 4, StaticColor, &vi )
      || XMatchVisualInfo( display, screenno, 6, DirectColor, &vi )
      || XMatchVisualInfo( display, screenno, 6, TrueColor, &vi ))
  {
    resources.customization = ".c";
  }
  else
  {
    resources.customization = ".bw";
  }

  db = XtDatabase(display);
  XrmPutStringResource(&db, "customization", resources.customization );
  
  toplevel = XtVaAppCreateShell( NULL, "Xtetris",
				applicationShellWidgetClass,
				display,
				NULL );
    
  XtAppAddConverter( context, "String", "Bitmap", XmuCvtStringToBitmap, 
		    screenConvertArg, XtNumber(screenConvertArg) );
  XtGetApplicationResources( toplevel, (caddr_t) &resources,
			    Resources, XtNumber(Resources),
			    NULL, (Cardinal) 0);
  XtAppAddActions( context, actions, XtNumber(actions) );

  if (resources.boxes_high > MAXBOXESHIGH)
	resources.boxes_high = MAXBOXESHIGH;
  if (resources.boxes_wide > MAXBOXESWIDE)
	resources.boxes_wide = MAXBOXESWIDE;

  for (i = 0; i < 7; i++) {
    static char *names[7] = { "object0","object1","object2","object3","object4",
				"object5","object6" };
    XtGetSubresources( toplevel, (caddr_t) &shape[i], names[i], "Object",
		      ShapeResources, XtNumber(ShapeResources),
		      NULL, (Cardinal) 0 );
  }
  frame = XtVaCreateManagedWidget("Frame", formWidgetClass, toplevel, NULL );
  tbar = XtVaCreateManagedWidget("TitleBar", labelWidgetClass, frame, NULL );
  status = XtVaCreateManagedWidget("Status", boxWidgetClass, frame, NULL );
  score_item = XtVaCreateManagedWidget("Score", labelWidgetClass, status, NULL );
  level_item = XtVaCreateManagedWidget("Level", labelWidgetClass, status, NULL );
  rows_item = XtVaCreateManagedWidget("Rows", labelWidgetClass, status, NULL );
  game_over = XtVaCreateManagedWidget("Game", labelWidgetClass, status, NULL );
  
  buttons = XtVaCreateManagedWidget("Buttons", formWidgetClass, frame, NULL );
  start_bt = XtVaCreateManagedWidget("Start", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  pause_bt = XtVaCreateManagedWidget("Pause", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  newgame_bt = XtVaCreateManagedWidget("NewGame", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  quit_bt = XtVaCreateManagedWidget("Quit", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  about_bt = XtVaCreateManagedWidget("About", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  if (resources.usescorefile)
    scores_bt = XtVaCreateManagedWidget("Scores", commandWidgetClass, buttons,
		XtNresizable, True,
		XtNright, XtChainRight,
		XtNleft, XtChainLeft,
		NULL );
  canvas = XtVaCreateManagedWidget( "Canvas", simpleWidgetClass, frame, 
				   XtNwidth, (XtArgVal)(resources.boxes_wide*resources.boxsize),
				   XtNheight, (XtArgVal)(resources.boxes_high*resources.boxsize),
				   XtNresizable, False,
				   NULL );
  shadow = XtVaCreateManagedWidget("Shadow", simpleWidgetClass, frame, 
				   XtNwidth, (XtArgVal)(resources.boxes_wide*resources.boxsize),
				   XtNheight, (XtArgVal)resources.boxsize,
				   NULL );
  nextobjectlabel = XtVaCreateManagedWidget("NextObjectLabel", labelWidgetClass, frame, NULL );
  nextobject = XtVaCreateManagedWidget("NextObject", simpleWidgetClass, frame,
				   XtNwidth, (XtArgVal)(4*resources.boxsize),
				   XtNheight, (XtArgVal)(4*resources.boxsize),
				   NULL );
  
  if (resources.usescorefile) 
  {
    score_frame = XtVaCreatePopupShell("ScoreFrame", transientShellWidgetClass, toplevel, 
				       XtNtransientFor, (XtArgVal)toplevel, 
				       XtNallowShellResize, True, 
				       NULL ); 

    score_text = XtVaCreateManagedWidget( "ScoreText", labelWidgetClass, score_frame,
					XtNresize, (XtArgVal)True,
					XtNborderWidth, 0,
					NULL );
  }
  about_frame = XtVaCreatePopupShell("AboutFrame", transientShellWidgetClass, toplevel,
				     XtNtransientFor, (XtArgVal)toplevel, 
				     XtNallowShellResize, True, 
				     NULL ); 

  about_text = XtVaCreateManagedWidget( "AboutText", labelWidgetClass, about_frame, 
				       XtNlabel, (XtArgVal)aboutstring,
				       XtNresize, (XtArgVal)True,
				       NULL );
  XtInstallAllAccelerators( canvas, toplevel );
  XtInstallAllAccelerators( shadow, toplevel );
  XtInstallAllAccelerators( nextobject, toplevel );
  XtInstallAllAccelerators( frame, toplevel );
  XtRealizeWidget(toplevel);
  XtRealizeWidget(about_frame);
  XtRealizeWidget(score_frame);
/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 *
 *  Honouring ICCCM WM_DELETE_WINDOW protocol.
 *	ENDCHANGE # UNKNOWN
 */
   	wm_delete_window  = XInternAtom(XtDisplay(toplevel),
                        "WM_DELETE_WINDOW", False);
	XSetWMProtocols(XtDisplay(toplevel), XtWindow(toplevel),
                        &wm_delete_window, 1);
	XtOverrideTranslations(toplevel,
                        XtParseTranslationTable("<Message>WM_PROTOCOLS:Quit()"));
	XSetWMProtocols(XtDisplay(about_frame), XtWindow(about_frame),
                        &wm_delete_window, 1);
	XtOverrideTranslations(about_frame,
                        XtParseTranslationTable("<Message>WM_PROTOCOLS:Done()"));
	XSetWMProtocols(XtDisplay(score_frame), XtWindow(score_frame),
                        &wm_delete_window, 1);
	XtOverrideTranslations(score_frame,
                        XtParseTranslationTable("<Message>WM_PROTOCOLS:Done()"));
/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 *  Mapping is not requirred after the Realize widget.
 *
 *  XtMapWidget(toplevel);
 *	ENDCHANGE # UNKNOWN
 */

/*
 *	CHANGE # UNKNOWN
 *	FILE # main.c
 *  On being resized,
 *		->	either objects should be drawn proportionately
 *		->	either number of small boxes that can be fitted in new width and
 *			height, should be changed.
 *		->	or do not allow resize of the client, by setting WMNormal Hints.
 *			Currently done this.
 *
 *	ENDCHANGE # UNKNOWN
 */
  {
   	XWindowAttributes win_attr;
	XSizeHints		 wm_hints;

        if (XGetWindowAttributes (XtDisplay(toplevel), XtWindow(toplevel),
		&win_attr) != 0)
	{
		wm_hints.min_width = win_attr.width;
		wm_hints.max_width = win_attr.width;
		wm_hints.min_height = win_attr.height;
		wm_hints.max_height = win_attr.height;
		wm_hints.flags |= PMinSize | PMaxSize;
            XSetWMNormalHints (XtDisplay(toplevel), XtWindow(toplevel),
                                &(wm_hints));
	}
  }


    
  {
    XGCValues gcv;
    
    gcv.foreground = resources.foreground;
    gcv.background = resources.background;
    gc = XCreateGC( XtDisplay(toplevel),XtWindow(toplevel), 
		   (unsigned long) GCForeground|GCBackground, &gcv );
    
    gcv.foreground = resources.background;
    gcv.background = resources.foreground;
    gcv.fill_style = FillStippled;
    gcv.stipple = resources.erasestipple;
    erasegc = XCreateGC( XtDisplay(toplevel),XtWindow(toplevel), 
			(unsigned long) GCForeground|GCBackground|GCStipple|GCFillStyle, &gcv );
  }
  initialise(); 
  
  XtAppMainLoop( context );
}
/*
emacs mode: indented-text
 
emacs Local Variables: 
emacs mode: c 
emacs c-indent-level: 2
emacs c-continued-statement-offset: 2
emacs c-continued-brace-offset: -2
emacs c-tab-always-indent: nil
emacs c-brace-offset: 0 
emacs tab-width: 8
emacs tab-stop-list: (2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80 82 84)
emacs End:
*/
