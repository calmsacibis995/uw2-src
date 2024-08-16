/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:main.c	1.6"
/*
 *
 *   File:        Main.c
 *
 *   Project:     DT 3.0
 *
 *   Description: main Dtstyle program
 *
 *
 *  (c) Copyright Hewlett-Packard Company, 1990, 1993.  
 *
 */

#include <signal.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/MessageB.h>
#include <DtI.h>

#include "colormain.h"
#include "colorpalette.h"
#include "colorfile.h"
#include "msgstr.h"

/* include extern functions */
#include "main.h"
#include "common.h"

/*Internal Functions */

#ifdef _NO_PROTO

static int IOErrorHandler();
static void ToolkitErrorHandler();
static void UnmanageCB();
static void DestroyCB();

#else

static int IOErrorHandler(Display *disp);
static void ToolkitErrorHandler(char *message);
static void UnmanageCB(Widget w, XtPointer client_data, XtPointer call_data);
static void DestroyCB(Widget w, XtPointer client_data, XtPointer call_data);

#endif /* _NO_PROTO */


/* Global Variables */
Style 		style;
intstr_t 	istrs;
Widget		pshell;
XtAppContext	app;

/* Misc functions all the dialogs use */

/*
 * BusyPeriod()
 */

void 
BusyPeriod(Widget w, Boolean busy)
{

	XDefineCursor (
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
#ifdef KENBOCURSOR
		(busy? XCreateFontCursor(XtDisplay(w), XC_pirate)
		     : XCreateFontCursor(XtDisplay(w), XC_shuttle) )
#else
		(busy? XCreateFontCursor(XtDisplay(w), XC_watch)
		     : XCreateFontCursor(XtDisplay(w), XC_left_ptr) )
#endif
		);
	return;
}


/*
 * CenterMsgCB 
 *    - to be used with message dialogs (assumptions are being made that 
 *      parent is dialog shell, and child is bb, due to Xm hacks for them)
 *      (eg. it sets bb x,y to 0,0 and parents x,y to x,y set for bb)
 *    - parent for positioning only (may not be real parent)
 *    - use client_data for parent... if NULL, use style.errParent if ok,
 *    - or main style.shell (makes this routine more generally usefull)
 *
 */
void 
#ifdef _NO_PROTO
CenterMsgCB(w, client_data, call_data)
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
CenterMsgCB(Widget w, XtPointer client_data, XtPointer call_data)
#endif /* _NO_PROTO */
{
	int n;
	Position newX, newY;
	Arg args[4];
	Widget   shell;

	/* figure out what to use as "visual" parent */
	shell = (Widget)client_data;

	/* calculate new x,y to be centered in visualParent */
	newX = XtX(shell) + XtWidth(shell)/2 - XtWidth(w)/2;
	newY = XtY(shell) + XtHeight(shell)/2 - XtHeight(w)/2;

	if (newX < 0) 
		newX = 0;
	if (newY < 0) 
		newY = 0;

	n = 0;
	XtSetArg(args[n], XmNx, newX); n++;
	XtSetArg(args[n], XmNy, newY); n++;
	XtSetValues(w, args, n);
}


/*
 * ErrDialog                                                 
 *  	Put up an error dialog and block until the user clicks OK
 * 	by default, there is no cancel or help button, but the 
 * 	dialog is created with autoUnmanage false, and ok/cancel
 * 	callbacks to do the unmanage, so that a help button can  
 * 	be used                                                   
*/
void 
#ifdef _NO_PROTO
ErrDialog(errString, parent )
        char *errString ;
        Widget parent ;
#else
ErrDialog(char *errString, Widget parent )
#endif /* _NO_PROTO */
{
	int           n;
	Arg           args[10];
	XmString      tmpstr;

	/* create the compound string */
	tmpstr = CMPSTR(errString);
	style.errParent = parent;

	n = 0;
	XtSetArg(args[n], XmNmessageString, tmpstr); n++;

	if (style.errDialog == NULL) { 	/* create it */
		/* okstr is init in main() */
		XtSetArg(args[n], XmNokLabelString, style.okstr);		n++;
		XtSetArg(args[n], XmNmwmFunctions, DIALOG_MWM_FUNC);  	n++;
		XtSetArg (args[n], XmNautoUnmanage, False);  		n++;
		XtSetArg (args[n], XmNdefaultPosition, False); 		n++;
		style.errDialog = XmCreateErrorDialog(parent, "ErrorNotice", 
						      args, n);
		XtAddCallback(style.errDialog, XmNokCallback, UnmanageCB,
			      NULL);

		XtUnmanageChild(XmMessageBoxGetChild(style.errDialog,
					             XmDIALOG_CANCEL_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(style.errDialog,
						        XmDIALOG_HELP_BUTTON));

		/* set the dialog shell parent title */
		n=0;
		XtSetArg (args[n], XmNmwmInputMode,
			  MWM_INPUT_PRIMARY_APPLICATION_MODAL); n++;
		XtSetArg (args[n], XmNuseAsyncGeometry, True);  n++;
		XtSetArg (args[n], XmNtitle, getstr(ERRSTR));   n++;
		XtSetValues (XtParent(style.errDialog), args, n);
	} else                  /* change the string */
		XtSetValues(parent, args, n);


	/* free the compound string */
	XmStringFree(tmpstr);
	XtManageChild(style.errDialog);
}


/* 
 * InfoDialog
 * 	Put up a modeless info dialog. 
 * 	There is no cancel or help button.
 * 	Dialog is created with autoUnmanage true.
 * 	An ok callback is added which will destroy the dialog 
 * 	and optionally unmap the parent.
 */
void
#ifdef _NO_PROTO
InfoDialog( infoString, parent)
        char *infoString ;
        Widget parent ;
#else
InfoDialog(
        char *infoString,
        Widget parent )
#endif /* _NO_PROTO */
{
	int             n;
	Arg             args[6];
	Widget          w;
	XmString	tmpstr;

	/* create the compound string */
	tmpstr = CMPSTR(infoString);


	/* create it */
	n = 0;
	XtSetArg(args[n], XmNokLabelString, style.okstr);			n++; 
	XtSetArg(args[n], XmNmessageString, tmpstr);			n++;
	XtSetArg(args[n], XmNdialogStyle, XmDIALOG_MODELESS);		n++;
	XtSetArg(args[n], XmNmwmFunctions, DIALOG_MWM_FUNC);		n++;
	w = XmCreateInformationDialog(parent, "Notice", args, n);

	XtAddCallback(w, XmNokCallback, DestroyCB, NULL);

	XtUnmanageChild(XmMessageBoxGetChild(w, XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(w, XmDIALOG_HELP_BUTTON));

	/* set the dialog shell parent title */
	n = 0;
	XtSetArg (args[n], XmNuseAsyncGeometry, True);			n++;
	XtSetArg (args[n], XmNtitle, getstr(NOTICE)); 			n++;
	XtSetValues (XtParent(w), args, n);

	/* free the compound string */
	XmStringFree (tmpstr);

	/* manage the info dialog */
	XtManageChild(w);
}


/* UnmanageCB                           */
static void 
#ifdef _NO_PROTO
UnmanageCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
UnmanageCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	if (style.flags & EXITOUT) {
		XtDestroyWidget(XtParent(w));
		exit(1);
	} else
		XtUnmanageChild(w);
}



/* DestroyCB                            */
static void 
#ifdef _NO_PROTO
DestroyCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
DestroyCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XtDestroyWidget(XtParent(w));
	if (client_data != NULL)
		XtUnmanageChild(client_data); 
	if (style.flags & EXITOUT) {
		XtUnmanageChild(style.colorDialog);
		UpdateDefaultPalette();
	}
}


/*
 * putDialog
 * 	move a dialog up so it isn't covering the main window 
 * 	Or down if there is no room up.
 *
 * 	Note: "parent" needs to have valid x,y information... 
 *       	ex: a child of dialog shell doesn't, its parent
 *       	does, so the parent shell would be passed in
 */
void 
#ifdef _NO_PROTO
putDialog( parent, dialog )
        Widget parent ;
        Widget dialog ;
#else
putDialog(
        Widget parent,
        Widget dialog )
#endif /* _NO_PROTO */
{
	int n;
	Position newX, newY, pY, pX;
	Dimension pHeight, myHeight, pWidth, myWidth;
	Arg args[4];

	pX = XtX(parent);
	pY = XtY(parent);
	pHeight = XtHeight(parent);
	pWidth = XtWidth(parent);
	myHeight = XtHeight(dialog);
	myWidth = XtWidth(dialog);

	if ((newY = pY - myHeight +5) < 0) 
		newY = pY + pHeight;
	newX = pX + pWidth/2 - myWidth/2;

	n = 0;
	XtSetArg(args[n], XmNx, newX); n++;
	XtSetArg(args[n], XmNy, newY); n++;
	XtSetValues(dialog,args,n);

#ifdef PutDDEBUG
	printf("newX, newY, pY, pX;\n");
	printf("%d    %d    %d  %d\n",newX, newY, pY, pX);
	printf("pHeight, myHeight, pWidth, myWidth;\n");
	printf("%d       %d        %d      %d\n", pHeight, myHeight, pWidth, myWidth);
#endif
}


static void
ackdtreply(Widget w, XtPointer client_data, XEvent *xevent, Boolean *p)
{
	DtReply     reply;

	if (xevent->type != SelectionNotify ||
	    (xevent->xselection.selection != _DT_QUEUE(style.display)))
		return;

	memset(&reply, 0, sizeof (reply));
	(void) DtAcceptReply(XtScreen(pshell), xevent->xselection.selection,
		 XtWindow(pshell), &reply); 
	XtDestroyWidget(style.shell);
	exit(0);
}


/* main                                 */
void 
#ifdef _NO_PROTO
main( argc, argv )
        int argc ;
        char **argv ;
#else
main(
        int argc,
        char **argv )
#endif /* _NO_PROTO */
{
	int 	   	val = 8;
	int 	   	swidth;
	Window		owner;
	char 		*p;

    	/* Register the default language proc */
    	XtSetLanguageProc(NULL, NULL, NULL);


	/* Initialize the toolkit and open the display */


    	style.shell = XtAppInitialize(
			&app,			/* app_context_return	*/
			"Color",		/* application_class	*/
			NULL,			/* options		*/
			0,			/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *) NULL,	/* fallback_resources	*/
			(ArgList) NULL,		/* args			*/
			(Cardinal) 0		/* num_args		*/
    			);
	XtVaSetValues (style.shell, XtNmappedWhenManaged, (XtArgVal) False, 0);

	/* initialize global style data */
	style.display    = XtDisplay(style.shell);
	style.screen     = DefaultScreenOfDisplay(style.display);
	style.screenNum  = DefaultScreen(style.display);
	style.depth      = DefaultDepth(style.display, style.screenNum);
	style.colormap   = DefaultColormap(style.display, style.screenNum);
	style.root       = DefaultRootWindow(style.display);
	style.errDialog  = NULL;
	style.home = (char *) XtMalloc(strlen((char *) getenv("HOME")) + 1);
	strcpy(style.home, (char *) getenv("HOME"));

	style.okstr = CMPSTR(getstr(OKSTR));
	style.cancelstr = CMPSTR(getstr(CANCELSTR));
	style.helpstr = CMPSTR(getstr(HELPSTR));

	CreateDialogBoxD(style.shell);
	pshell = XtParent(style.colorDialog);
	XtVaSetValues (pshell, XtNmappedWhenManaged, (XtArgVal) False, 0);
	/* Realize the shell */
	XtRealizeWidget (pshell);

	/* Check if we are already running. */
	owner = DtSetAppId(XtDisplay(pshell), XtWindow(pshell), "Color");
	if (owner != None) {
		/* 
		 * We are already running.  Bring that window to the 
		 * top and exit. 
		 */
		XRaiseWindow(XtDisplay(pshell), owner);
		XFlush(XtDisplay(pshell));
		exit(0);
	}

	DtInitialize(pshell);


	/* Register error handlers */
	XSetIOErrorHandler(IOErrorHandler);
	XtAppSetErrorHandler(app, ToolkitErrorHandler);

	/* Determine the display resolution */
	swidth = XDisplayWidth(style.display, style.screenNum);
	if (swidth > 1024) 
		val = 3;
	else if (swidth > 800) {
		val = 5;
#ifdef USL
		if (swidth == 1024) 
			val = 3;
#endif
	} else if (swidth > 511)
	    	val = 8;
	else
		fprintf(stderr, "not supported display %d\n", swidth);

	style.horizontalSpacing = style.verticalSpacing = val;

	/* 
	 * Determine the type of Monitor  by quering 
	 * Color server 
	 */
	InitializeAtoms();
	CheckMonitor(pshell);
	/* filename + 1(:) + 4 (for catlog number) + 1 (NULL) */
	style.msgcat = XtMalloc(strlen(CATALOGFILE) + 6);
	istrs.suffix = getstr(PALETTE_SUFFIX);
	istrs.white = strdup(getstr(W_ONLY));
	if (p = strstr(istrs.white, istrs.suffix))
		*p  = '\0';
	istrs.whiteblack = strdup(getstr(W_O_B));
	if (p = strstr(istrs.whiteblack, istrs.suffix))
		*p  = '\0';
	istrs.black = strdup(getstr(B_ONLY));
	if (p = strstr(istrs.black, istrs.suffix))
		*p  = '\0';
	istrs.blackwhite = strdup(getstr(B_O_W));
	if (p = strstr(istrs.blackwhite, istrs.suffix))
		*p  = '\0';
	istrs.prefix = getstr(PALETTE_PREFIX);
	if (InitializePalette() == 0) {

		/* Get the default Palette */
		GetDefaultPal(pshell);

		signal(SIGINT,(void (*)())activateCB_exitBtn); 
		signal(SIGTERM,(void (*)())activateCB_exitBtn); 
		CreateMainWindow(pshell); 
		XtSetMappedWhenManaged(pshell, True);
		XtManageChild(style.colorDialog);
		/* 
		 * Add the WM_DELETE_WINDOW and WM_SAVE_YOURSELF peoperties to
		 * the main window.
		 */

		XmAddWMProtocolCallback(pshell, XA_WM_SAVE_YOURSELF, 
					activateCB_exitBtn, NULL),
		XmAddWMProtocolCallback(pshell, XA_WM_DELETE_WINDOW, 
					activateCB_exitBtn, NULL),
		XtAddEventHandler(pshell, NoEventMask, True, ackdtreply, 
			  	  (XtPointer)NULL);
	}
	XtAppMainLoop(app);
}


/*
 *  IOErrorHandler
 */
static int
#ifdef _NO_PROTO
IOErrorHandler (display)
	Display *display;
#else
IOErrorHandler (
	Display *display)
#endif /* _NO_PROTO */
{
	XtWarning (getstr(IOERR));
	exit (1);
} 


/*
 *  ToolkitErrorHandler
 * 
 *  	All Xt memory allocation errors should fall through to this routine.
 *  	There is no need to check for Xtmalloc errors where they are used.
 */
static void
#ifdef _NO_PROTO
ToolkitErrorHandler( message )
        char *message ;
#else
ToolkitErrorHandler(
        char *message )
#endif /* _NO_PROTO */
{
#ifdef DEBUG
	XtWaring("An X Toolkit error occurred... Exiting.\n");
#endif
	exit (1);
}

void 
#ifdef _NO_PROTO
HelpRequestCB( w, client_data, call_data )
        Widget w ;
        Xtpointer client_data ;
        XtPointer caddr_t call_data ;
#else
HelpRequestCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	DtRequest       req;
	char *section = client_data;

        memset(&req, 0, sizeof(req));
        req.display_help.rqtype = DT_DISPLAY_HELP;
	req.display_help.source_type =  DT_SECTION_HELP; 
	req.display_help.sect_tag = section;
	req.display_help.app_name = "colorprop";
	req.display_help.app_title = "colorprop";
	req.display_help.title = "colorprop";
	req.display_help.help_dir = NULL;
	req.display_help.file_name = "DesktopMgr/clrpref.hlp";
        
	(void)DtEnqueueRequest(style.screen, _HELP_QUEUE (style.display),
                           _HELP_QUEUE (style.display), style.root, &req);
}   
