/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osmsgmon:osMessageMonitor.c	1.13"
/*
 * $XConsortium: osMessageMonitor.c,v 1.9 91/07/25 14:23:46 rws Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, MIT X Consortium
 */
/*
 * Heavly Modified by Univel to use Motif widgets and conform to our
 * environment 
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>		/* for isprint */
#include <time.h>       /* for time stamping messages */

#include <sys/types.h>		
#include <sys/stat.h>		
#include <signal.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeB.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>

#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#include <Xm/MainW.h>

#include <DtI.h>
#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/ModalGizmo.h>

#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/StdSel.h>
#include <X11/Xos.h>


#include <X11/Xmu/Editres.h>
#include <locale.h>
#include "dtFuncs.h"
#include "msg.h"
#include "utils.h"

extern int kill(pid_t,int);
extern int XmuGetHostname (char *, int );

Pixmap pixmap = 0;
Pixmap old_pixmap = 0;

/*
 * Function Prototypes
 */

/* not static referenced by utils.c  mwmOverride() */
void Quit ( Widget , XEvent *, String *params, Cardinal *num_params);
void Help   ( Widget , XEvent *, String *params, Cardinal *num_params);

static void OpenConsole (void);
static void Notify (void);

static void Deiconified(Widget, XEvent *,String *params, Cardinal *num_params);
static void Iconified ( Widget ,XEvent *, String *params, Cardinal *num_params);

static void Clear ( Widget , XEvent *, String *params, Cardinal *num_params);
static void stripNonprint ( char    *b, int     n);
static void inputReady ( XtPointer	w, int	*source, XtInputId	*id);

static long	TextLength (Widget w);
static void TextReplace ( Widget w, int start, int end, XmTextBlockRec *block);
static void TextAppend ( Widget w, char *s, int len);
static void TextInsert ( Widget w, char *s, int len);

static int osm_pipe(void);

static void Usage(void);

static Widget create_menus(Widget maimwindow, Widget parent);
static void set_dialog_wm_name( Widget the_dialog, char *the_name);

static void daInputCB(Widget w,XtPointer clientData,XtPointer callData);
static void scrollBarCB(Widget w,XtPointer clientData,XtPointer callData);
static void saveAsCB(Widget w,XtPointer client_data,XtPointer call_data);
static void saveCB(Widget w,XtPointer client_data,XtPointer call_data);
static void appendCB(Widget w,XtPointer client_data,XtPointer call_data);
static void notifyByFlashCB(Widget w,XtPointer client_data,XtPointer call_data);
static void notifyByColorCB(Widget w,XtPointer client_data,XtPointer call_data);
static void notifyByDeiconifyCB(Widget w,XtPointer client_data,XtPointer call_data);
static void tstampCB(Widget w,XtPointer client_data,XtPointer call_data);
static void StartupTimeOutCB(caddr_t data, XtIntervalId *id );
static void saveData(char *fileName, int which_action );
static void read_name(Widget, int ,XmSelectionBoxCallbackStruct *cbs);
static void TimeOutCB(caddr_t data, XtIntervalId *id );

static void okToOverWrite(Widget , int ,XmSelectionBoxCallbackStruct *);
static void notOkToOverWrite(Widget , int ,XmSelectionBoxCallbackStruct *);
static void errorNoticePopdownCB(Widget , XtPointer , XtPointer );

/*  Need to know when child dies */
static void sig_child(int signo);

/*  Need to know if parent still alive */
static void sig_alrm(int signo);

#ifdef SELECTION
static void CloseConsole (void);
static Boolean ConvertSelection ( Widget, Atom *,Atom *,Atom *, XtPointer *,
							    unsigned long *, int *format);
static void LoseSelection ( Widget  w, Atom *selection);
static void InsertSelection ( Widget, XtPointer, Atom *,Atom *, XtPointer,
    							unsigned long   *, int	*);
#endif

/*
 * Routine to copy pixmap onto the iconWindow
 */
void InstallNewPixmap(Pixmap pixmap);
/*
 * Globals
 */
static int childPid;                   /* the reader process */
static int scrollView = True;       
static int startupDelayOver = False;   /* can't notify till True */

static int fileNameSet = False;        /* file to save to specified */
static char *defLogFileName = NULL;    /* default file name pointer */
static char *fileName = NULL;          /* file name to write to */
static char *dirName = NULL;           /* directory name to write to */

static Widget top = NULL;
static Widget text = NULL;
static Widget vScrollBar = NULL;
static Widget dialog = NULL;
static Widget warningdialog = NULL;

static XtInputId	input_id;
static FILE     *input;
static Boolean	notified;
static Boolean	iconified;

/* Flashing icon support varibles */
static Widget iconWinWidget;
static Window iconWindow;
static unsigned int pixmapWidth;
static unsigned int pixmapHeight;
static unsigned int depth;
static GC gc;
static XtIntervalId id;
static int doEventLoop = False;

static Atom	wm_delete_window;
static Atom	mit_console;
#define MIT_CONSOLE_LEN	12
#define MIT_CONSOLE "MIT_CONSOLE_"
static char	mit_console_name[255 + MIT_CONSOLE_LEN + 1] = MIT_CONSOLE;

#define MAX_ARGS  30

#define CANCEL          0   /* responses to certain dialogs */
#define OK              1   /* responses to certain dialogs */
#define ACTIVATE        2   /* display the file prompt dialog */
#define SAVE            3   /* overwrite existing file */
#define APPEND          4   /* append to an existing file */


/*
 * X Resources
 */
static struct _app_resources {
    Boolean stripNonprint;
    Boolean notifyByFlash;
    Boolean notifyByDeiconify;
    Boolean notifyByColorChange;
    Boolean daemon;
    Boolean verbose;
	int     rows;
	int     columns;
    Boolean exitOnFail;
	char  * fileName;
    Boolean timeStamp;  
    char *colorChangeTuples; 
} app_resources;

#define Offset(field) XtOffsetOf(struct _app_resources, field)

#define COLUMNS 80
#define ROWS 10
#define DEFAULT_FILENAME "/tmp/osmLog"

static XtResource  resources[] = {
	/*-- WARNING this must be FIRST (hard coded position later on in code--*/
    {"saveFile", "SaveFile",XtRString, sizeof(char *),
        Offset(fileName), XtRString, "/tmp/osmLog"
    },
    {"notifyByFlash",	"NotifyByFlash",   XtRBoolean,	sizeof (Boolean),
		Offset (notifyByFlash), XtRImmediate, (XtPointer)False 
	},
    {"notifyByDeiconify",	"NotifyByDeiconify",   XtRBoolean,	sizeof (Boolean),
		Offset (notifyByDeiconify), XtRImmediate, (XtPointer)False 
	},
    {"notifyByColorChange",	"NotifyByColorChange",   XtRBoolean,	sizeof (Boolean),
		Offset (notifyByColorChange), XtRImmediate, (XtPointer)False 
	},
    {"stripNonprint",	"StripNonprint",    XtRBoolean, sizeof (Boolean),
		Offset (stripNonprint), XtRImmediate, (XtPointer)True 
	},
    {"daemon",		"Daemon",	    XtRBoolean,	sizeof (Boolean),
		Offset (daemon), XtRImmediate, (XtPointer)True
	},
    {"verbose",		"Verbose",	    XtRBoolean,	sizeof (Boolean),
		Offset (verbose),XtRImmediate, (XtPointer)True
	},
    {"columns",	"Columns",    XtRInt,	sizeof (int),
		Offset (columns),XtRImmediate, (XtPointer)COLUMNS
	},
    {"rows",	"Rows",    XtRInt,	sizeof (int),
		Offset (rows),XtRImmediate, (XtPointer)ROWS
	},
    {"exitOnFail",	"ExitOnFail",    XtRBoolean,	sizeof (Boolean),
		Offset (exitOnFail),XtRImmediate, (XtPointer)True
	},
    {"timestamp",   "Timestamp",    XtRBoolean, sizeof (Boolean),
    	Offset (timeStamp), XtRImmediate, (XtPointer)False
	},
    {"colorChangeTuples",   "ColorChangeTuples",XtRString, sizeof (char *),
    	Offset (colorChangeTuples), XtRString, NULL
	}
};

#undef Offset

static XrmOptionDescRec options[] = {
    {"-nf", "*notifyByFlash",		XrmoptionNoArg,	    "TRUE"},
    {"+nf", "*notifyByFlash",		XrmoptionNoArg,	    "FALSE"},
    {"-nd", "*notifyByDeiconify",		XrmoptionNoArg,	    "TRUE"},
    {"+nd", "*notifyByDeiconify",		XrmoptionNoArg,	    "FALSE"},
    {"-nc", "*notifyByColorChange",		XrmoptionNoArg,	    "TRUE"},
    {"+nc", "*notifyByColorChange",		XrmoptionNoArg,	    "FALSE"},
    {"-v",  "*verbose",	XrmoptionNoArg,	    "TRUE"},
    {"+v",  "*verbose",	XrmoptionNoArg,	    "FALSE"},
    {"-e",  "*exitOnFail",	XrmoptionNoArg,	    "TRUE"},
    {"+e",  "*exitOnFail",	XrmoptionNoArg,	    "FALSE"},
    {"-t",  "*timestamp",  XrmoptionNoArg,     "TRUE"},
    {"+t",  "*timestamp",  XrmoptionNoArg,     "FALSE"},
    {"-daemon",  "*daemon",		XrmoptionNoArg,	    "TRUE"},
    {"+daemon",  "*daemon",		XrmoptionNoArg,	    "FALSE"},
};

static XtActionsRec actions[] = {
    "Quit",         Quit,
    "Iconified",    Iconified,
    "Deiconified",  Deiconified,
    "Clear",        Clear,                /* override in resource file */
};

main (int argc, char    **argv)
{
    Arg arglist[MAX_ARGS];
    Cardinal num_args;
    char     *hostname;
	char     *tmp;
	Widget   form;
	Widget   menu_bar;

	XtSetLanguageProc(NULL, NULL, NULL);

    top = XtInitialize (APP_NAME, APP_CLASS_NAME, options, XtNumber (options),
			&argc, argv);
    
	if ( argc >  1 )
	{
		Usage();
	}
    XtVaSetValues(top, XmNtitle, getStr(TXT_Title),NULL);

	/*
	 * Internationalize the default save file name
	 * ( User can override in the app defaults file "OsMessageMonitor" )
	 */
	resources[0].default_addr = (XtPointer)getStr( TXT_defFileName) ;

    XtGetApplicationResources (top, (XtPointer)&app_resources, resources,
			       XtNumber (resources), NULL, 0);


    if (app_resources.daemon)
	{
		if (fork ()) 
			exit (0);
		setsid();
	}


	/*
	 * Get the icon pixmaps and change colors on the notify
	 * pixmap
	 */
	getIconPixmap(XtDisplay(top), ICON_NAME, &pixmap);
	ChangePixmapColors(top,app_resources.colorChangeTuples,pixmap);

	getIconPixmap(XtDisplay(top), ICON_NAME, &old_pixmap);
	setIconPixmap(top, &old_pixmap, APP_NAME);
	iconWinWidget = SetIconWindow(top,old_pixmap,&pixmapWidth,
							&pixmapHeight,&depth);
    XtAddCallback(iconWinWidget,
					XmNinputCallback,(XtCallbackProc)daInputCB, 
							(XtPointer)NULL);
	iconWindow = 0;

    /*
     * Tell the shell widget to DO_NOTHING on window close
     *  we need to be able to kill our child ( AHHHH that sounds bad )
     */
    XtVaSetValues (top, XmNdeleteResponse, XmDO_NOTHING,NULL);


    form=XtVaCreateManagedWidget ("form", xmFormWidgetClass, top, NULL);

    XtAddCallback(form,XmNhelpCallback,(XtCallbackProc)helpCB, 
							(XtPointer)APP_HELP);
	/*
	 * Can't have any two set at the same time
	 */
	if (( app_resources.notifyByFlash  &&  !app_resources.notifyByDeiconify 
							&& !app_resources.notifyByColorChange )
			|| ( app_resources.notifyByDeiconify  &&  !app_resources.notifyByFlash
					&& !app_resources.notifyByColorChange )
			|| ( app_resources.notifyByColorChange  &&  !app_resources.notifyByDeiconify 
					&& !app_resources.notifyByFlash )
			|| ( !app_resources.notifyByColorChange  &&  !app_resources.notifyByDeiconify 
					&& !app_resources.notifyByFlash ))
	{
	}
	else
	{
		app_resources.notifyByFlash  = False;
		app_resources.notifyByDeiconify  = False;
		app_resources.notifyByColorChange  = False;
	}
	
    menu_bar = create_menus(form, top ); /* create menus */
    XtManageChild(menu_bar);

	/*
	 * For those that would put zero in the resource file
	 */
	if (app_resources.rows == 0 ) 
		app_resources.rows = ROWS;
	if (app_resources.columns == 0 ) 
		app_resources.columns = COLUMNS;

	dirName = app_resources.fileName;
	tmp = strrchr(dirName,'/');
	if ( tmp )
		fileName = strdup(&tmp[1]);
	else
		fileName = strdup(dirName);
	defLogFileName = fileName;
	if ( tmp )
		*tmp = NULL;

	/*
	 * Create the scrolled text window
	 */
    num_args = 0;
    XtSetArg (arglist[num_args], XmNrows, app_resources.rows);num_args++;
    XtSetArg (arglist[num_args], XmNcolumns, app_resources.columns);num_args++;
    XtSetArg (arglist[num_args], XmNscrollHorizontal, False);num_args++;
    XtSetArg (arglist[num_args], XmNwordWrap, False);num_args++;
    XtSetArg (arglist[num_args], XmNtopAttachment, XmATTACH_WIDGET);num_args++;
    XtSetArg (arglist[num_args], XmNtopWidget, menu_bar);num_args++;
    XtSetArg (arglist[num_args], XmNbottomAttachment, XmATTACH_FORM);num_args++;
    XtSetArg (arglist[num_args], XmNrightAttachment, XmATTACH_FORM);num_args++;
    XtSetArg (arglist[num_args], XmNleftAttachment, XmATTACH_FORM);num_args++;
    XtSetArg (arglist[num_args], XmNwordWrap, True);num_args++;
    XtSetArg (arglist[num_args], XmNeditable, False);num_args++;
    XtSetArg (arglist[num_args], XmNeditMode, XmMULTI_LINE_EDIT);num_args++;
    XtSetArg (arglist[num_args], XmNcursorPositionVisible, False);num_args++;
    text = XmCreateScrolledText (form,"text", arglist, num_args);
    XtManageChild(text);

    XtAddCallback(text,XmNhelpCallback,(XtCallbackProc)helpCB, 
							(XtPointer)APP_HELP);

	/*
	 * Point the vertical scrollbar callback to my callback.
	 * We need to manage the text view scroll.  When the
	 * slider is not at the trough bottom we don't want to scroll
	 * the text view,  user is probably looking at it.  Annoyed
	 * me enough that I put this code in.  Got to get all those
 	 * scrollbar movements.
	 */
    XtVaGetValues(XtParent(text), XmNverticalScrollBar, &vScrollBar,NULL);
	XtAddCallback ( vScrollBar, XmNvalueChangedCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNtoBottomCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNtoTopCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNpageDecrementCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNpageIncrementCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNdecrementCallback,
			scrollBarCB, (XtPointer) NULL );
	XtAddCallback ( vScrollBar, XmNincrementCallback,
			scrollBarCB, (XtPointer) NULL );

	/*
	 * Override some translations
	 */
    XtAddActions (actions, XtNumber (actions));
    XtOverrideTranslations(top,
		XtParseTranslationTable("<Message>WM_PROTOCOLS: Quit()"));
    XtOverrideTranslations(top,
		XtParseTranslationTable("<UnmapNotify>: Iconified()"));
    XtOverrideTranslations(top,
		XtParseTranslationTable("<MapNotify>: Deiconified()"));

	mwmOverride(top);
    
	/*
	 * On startup 'startupDelayOver' gives us a 5 second period where
	 * we can read from /dev/osm without a user notification.
	 * This ensures that iconic startup of osMessageMonitor will stay in the
	 * icon state until new data arrives.   Also protects against putting
	 * an asterix in the icon name until new data arrives, when 'notify' 
	 * is False.
	 */
	(void) XtAddTimeOut(5 * 1000,(XtTimerCallbackProc) StartupTimeOutCB,NULL);
	startupDelayOver = False;
    XtRealizeWidget (top);


    XtVaGetValues(top, XtNiconic, &iconified, NULL);
    if (iconified)
	{
		/* Keep the compiler quiet on this one */
		Iconified ((Widget)NULL ,(XEvent *)NULL,
							(String *)NULL,(Cardinal *)NULL);
	}
    else
	{
		/* Keep the compiler quiet on this one too */
		Deiconified ((Widget)NULL ,(XEvent *)NULL,
							(String *)NULL,(Cardinal *)NULL);
	}
    wm_delete_window = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW",
				   False);
    (void) XSetWMProtocols (XtDisplay(top), XtWindow(top),
                            &wm_delete_window, 1);

    XmuGetHostname (mit_console_name + MIT_CONSOLE_LEN, 255);

#ifndef SELECTION
    OpenConsole ();
#else
    mit_console = XInternAtom(XtDisplay(top), mit_console_name, False);

    if (XGetSelectionOwner (XtDisplay (top), mit_console))
    {
	    XtGetSelectionValue(top, mit_console, XA_STRING, InsertSelection,
			    NULL, CurrentTime);
    }
    else
    {
	    XtOwnSelection(top, mit_console, CurrentTime,
		       ConvertSelection, LoseSelection, NULL);
	    OpenConsole ();
    }
#endif
	XtAddEventHandler(top,(EventMask) 0, True, _XEditResCheckMessages, NULL);
    XtMainLoop ();
    return 0;
}

static void
Usage()
{
	fprintf(stderr,"%s",getStr( TXT_syntax ));
	fprintf(stderr,"%s",getStr( TXT_use ));
    fprintf(stderr,"%s",getStr( TXT_notifyDeiconify ));
    fprintf(stderr,"%s",getStr( TXT_notifyFlash ));
    fprintf(stderr,"%s",getStr( TXT_daemon ));
    fprintf(stderr,"%s",getStr( TXT_verbose ));
    fprintf(stderr,"%s",getStr( TXT_exitOnFl ));
    fprintf(stderr,"%s",getStr( TXT_tstamp ));
	exit(1);
}

static void
OpenConsole ()
{
	int pipeFd;                     /* readers fd */

    input = 0;
    
	pipeFd = osm_pipe();
    input = fdopen(pipeFd, "r");
    if ((pipeFd != -1) && (input != NULL) && app_resources.verbose)
    {
	    char	*hostname;
	    TextAppend (text, getStr( TXT_conLog ), 0);
	    hostname = mit_console_name + MIT_CONSOLE_LEN;
	    TextAppend (text, hostname, strlen (hostname));
	    TextAppend (text, "\n", 1);
    }
	if (input == NULL || pipeFd == -1)
	{
	    if (app_resources.exitOnFail)
		{
			if ( input == NULL )
				fprintf(stderr,"%s",getStr( TXT_pipeErr ));
			fprintf(stderr,"%s",getStr( TXT_abort ));
			kill(childPid,9);
		    exit(0);
		}
	    TextAppend (text,getStr( TXT_osmOpen ),0);
	    input = stdin;
	}
    input_id = XtAddInput (fileno (input), (XtPointer) XtInputReadMask,
			       inputReady, (XtPointer) text);
}

/*
 * On SYSV386 there is a special device, /dev/osm, where system messages
 * are sent.  Problem is that we can't perform a select(2) on this device.
 * So this routine creates a streams-pty where one end reads the device and
 * sends the output to osMessageMonitor.
 */
int pty;
static int
osm_pipe()
{
    extern int childPid;
    int pv[2];

    if ( pipe(pv) < 0) 
	{
	    fprintf(stderr, getStr ( TXT_pipeOpen));
        return(-1);
	}
    
	/*
	 * If the other end closes, we need to die
	 * Either parent or child
	 */
	(void) signal(SIGPIPE, sig_child);

    if ((childPid = fork()) == 0) 
    {
        int osm, nbytes;
        char buf[256];
		extern int errno;
		int useFile = True;
		int sleep_cnt = 0;
		int i;

		setsid();
		pty = pv[1];
		close(pv[0]);

		/* THANKS! x-faq_5 Subject 129 */
		close (ConnectionNumber(XtDisplay(top)));
	    
        osm = open("/etc/.osm", O_RDONLY);
		if ( osm == -1 )
		{
			useFile = False;
        	osm = open("/dev/osm", O_RDONLY);
		}
	    if ( osm == -1 )
	    {
	    	fprintf(stderr, getStr ( TXT_osmOpen ));
		    kill(childPid,9);
		    exit(0);
	    }
		for(;;)
	    {
			if ( useFile == True )
			{
				sleep(sleep_cnt);
        		nbytes = read(osm, &buf, sizeof(buf));
				if ( nbytes <= 0 )
					sleep_cnt = 1;
			}
			else
			{
				/*
			 	 * Wake-up every 30 seconds and ensure our parent
			 	 * hasn't croked yet, lets sigPipe signal through
			 	 */
				alarm(30);
   				signal(SIGALRM,sig_alrm);

        		if (( nbytes = read(osm, &buf, sizeof(buf))) <= 0)
				{
					/* 
				 	* Any thing but an interrupted system call
				 	* we want to exit on
				 	*/
					if ( errno != EINTR )
						break;
					/*
				 	* Alarm fired, reset and continue read
				 	*/
					continue;
				}
				alarm(0);
			}
			if ( nbytes != -1 )
				write(pty, &buf, nbytes);
    	}
		kill(childPid,9);
		exit(0);
    }
	close(pv[1]);
	(void) signal(SIGCLD, sig_child);
	(void) signal(SIGHUP, sig_child);
	(void) signal(SIGINT, sig_child);
	(void) signal(SIGQUIT, sig_child);
	(void) signal(SIGABRT, sig_child);
	(void) signal(SIGTERM, sig_child);

    return (pv[0]);
}

static void sig_child(int signo)
{
/*	kill(childPid,9);*/
	exit(0);
}
static void sig_alrm(int signo)
{
	/*
	 * Causes a SIGPIPE if other end closed
	 * Which is trapped and causes an exit
	 * Otherwise no effect
	 */
	write(pty, NULL, 0);
}

static void
inputReady ( XtPointer	w,int *source, XtInputId *id)
{
    char    buffer[1025];
    char    buffer2[80];  
    int	    n,i;

    n = read (*source, buffer, sizeof (buffer) - 1);
    if (n <= 0)
    {
	    fclose (input);
	    XtRemoveInput (*id);
    }
    Notify ();
	for(i=0;i<n;i++)
	{
		if ( buffer[i] == '\0' )
			buffer[i] = ' ';
	}
    buffer[n] = '\0';
    if (app_resources.stripNonprint)
    {
	    stripNonprint (buffer,n);
	    n = strlen (buffer);
    }
    if (app_resources.timeStamp)               
    {
        int     timesize;
        struct  tm      *tm;
        time_t  t;

        (void) time(&t);
        tm = localtime(&t);
		strcpy(buffer2,"OSM_TS:- ");
		timesize = strftime(&buffer2[strlen(buffer2)],
		             sizeof(buffer2) - strlen(buffer2) - 4,
					 NULL, tm);
		strcat(buffer2," -: ");
		/*
		 * Find first newline and send this partial string
		 * to the text widget. Send time stamp and then
		 * finish the string.
		 */
		for(i = 0; i < n; )
		{
			if ( buffer[i] == '\n' )
			{
   				TextAppend ((Widget) text, buffer, i + 1 );
   				break;
			}
			i++;
		} 
		/*
		 * In case packet read did not contain a newline
		 */
		if ( buffer[i] != '\n' )
		{
    		TextAppend ((Widget) text, "\n", 1);
			i = 0;
		}
		else
			i++;
    	TextAppend ((Widget) text, buffer2, strlen(buffer2));
		TextAppend ((Widget) text, &buffer[i], n - i );
    } 
	else
    	TextAppend ((Widget) text, buffer, n);
}


void
Quit ( Widget widget, XEvent *event, String *params, Cardinal *num_params)
{
    kill(childPid,9);
    exit (0);
}

void
InstallNewPixmap(Pixmap pixmap)
{
	if ( iconWindow == 0 )
		iconWindow = XtWindow(iconWinWidget);
	XSetWindowBackgroundPixmap(XtDisplay(top),iconWindow,pixmap);
	XClearWindow(XtDisplay(top),iconWindow);
}

static void
Notify ()
{
	XWMHints * wmhints;
	XEvent event;
	int i;

    if (!iconified )
        return;
	/*
	 * Already notified the user or it is too soon to notify
	 * then return
	 */
	if ( notified == True || startupDelayOver == False )
		return;
    if ( app_resources.notifyByDeiconify == True )
	{
		XMapWindow(XtDisplay(top),XtWindow(top));
        return;
	}
	else if ( app_resources.notifyByColorChange == True )
	{
		InstallNewPixmap(pixmap);
		return;
	}
	else if ( app_resources.notifyByFlash == True  )
	{
		if ( old_pixmap == 0 || pixmap == 0 )
			return;
    	notified = True;
		InstallNewPixmap(pixmap);
		id = XtAddTimeOut(1000, (XtTimerCallbackProc)TimeOutCB,NULL);
	}
}
void
TimeOutCB(caddr_t data, XtIntervalId *tid)
{
	static int toggle = 1;

	if ( toggle )
	{
		InstallNewPixmap(old_pixmap);
	}
	else
	{
		InstallNewPixmap(pixmap);
	}
	toggle ^= 1;
	id = XtAddTimeOut(1 * 1000, (XtTimerCallbackProc)TimeOutCB,NULL);
}
static void 
daInputCB ( Widget w , XtPointer clientData, XtPointer callData )
{
	/*
 	* Leave the notify color pixmap up
 	*/
	if (notified == True  )
	{
   		XtRemoveTimeOut(id);
		InstallNewPixmap(pixmap);
    	notified = False;
	}
}

static void
Deiconified ( Widget widget, XEvent *event, String *params, 
				Cardinal *num_params)
{

    iconified = False;
    if ( notified == False )
	    return;
	if ( app_resources.notifyByFlash == True || app_resources.notifyByColorChange == True  )
	{
		if ( app_resources.notifyByColorChange != True )
    		XtRemoveTimeOut(id);
		InstallNewPixmap(old_pixmap);
	}
    notified = False;
}

static void
Iconified ( Widget widget, XEvent *event, String *params, Cardinal *num_params)
{
	InstallNewPixmap(old_pixmap);
    iconified = True;
}

static void
Clear ( Widget widget, XEvent *event, String *params, Cardinal *num_params)
{
    long	    last;
    XmTextBlockRec    block;

    last = TextLength (text);
    block.ptr = "";
    block.length = 0;
    block.format = FMT8BIT;
    TextReplace (text, 0, last, &block);
}


static long 
TextLength ( Widget  w)
{
    return XmTextGetLastPosition(w);
}

static void
TextReplace ( Widget w, int start, int end, XmTextBlockRec *block)
{
    XtSetMappedWhenManaged(text,False);
    XmTextReplace (w, start, end, block->ptr);
	XFlush(XtDisplay(w));
    XtSetMappedWhenManaged(text,True);
}

static void
TextAppend ( Widget  w, char *s, int len)
{
    long	    last;
	char x;
	

    last = TextLength (w);

    x = s[len];

	/*
	 * Strings without a length are assumed to be NULL terminated by the caller
	 */
	if ( len )
    	s[len] = 0;
    XmTextInsert(w,last,s);    
	if ( len )
    	s[len] = x;
	
	/*
	 * Only scroll the text window when True
	 * ( See the ScollBarCallback routine )
	 */
	if ( scrollView == True )
		XmTextSetInsertionPosition(w,TextLength (w));
}

static void
TextInsert ( Widget  w,char  *s,int len)
{
    XmTextBlockRec    block;
    XmTextPosition    current;

    current = XmTextGetInsertionPosition (w);
    block.ptr = s;
    block.length = len;
    block.format = FMT8BIT;
    TextReplace (w, 0, 0, &block);
    if (current == 0)
	    XmTextSetInsertionPosition (w,(XmTextPosition) len);
}

static void
stripNonprint ( char *b, int n)
{
    char    *c;

    c = b;
    while (n)
    {
	    n--;
	    if (isprint (*b) || isspace (*b) && *b != '\r')
	    {
	        if (c != b)
		    *c = *b;
	        ++c;
	    }
	    ++b;
    }
    *c = '\0';
}
static void 
scrollBarCB ( Widget w ,
                      XtPointer clientData,
                      XtPointer callData )
{
	int		value;
	int		max;
	int		min;
	int		size;

	XtVaGetValues(vScrollBar, XmNmaximum, &max,
							  XmNminimum, &min,
							  XmNsliderSize, &size,
							  XmNvalue, &value,NULL);
	/*
	 * We only set scrollView to True if the the
	 * slider is at the end of the trough.  Annoying
	 * to have an async update move the text view when
	 * you are trying to scan the old data.
	 */
	if ( (max - size) == value )
		scrollView = True;
	else
		scrollView = False;
	return;
}


/*
 * create_menus - this function creates all the pull down menus and attaches
 *                the callbacks to them.  It returns the menu bar widget.
 */
static Widget
create_menus( Widget form, Widget parent)
{
    int i, j;
    Cardinal n;
    Arg args[MAX_ARGS];
    Widget menu_bar_frame;
	Widget menuBarW;
	Widget pullDownW;
	Widget viewPullDownW;
	Widget optionPullDownW;
	Widget notifyPullDownW;
	Widget helpPullDownW;
	Widget s;

	Widget fileWidget;
	Widget viewWidget;
	Widget optionWidget;
	Widget notifyWidget;
	Widget helpWidget;

	XmString file;
	XmString view;
	XmString options;
	XmString notify;
	XmString help;
	char * notifyString;



    /*
     * Create MenuBar in MainWindow.
     */
    n = 0;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    XtSetArg(args[n], XmNshadowType, XmSHADOW_OUT); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    menu_bar_frame =  XmCreateFrame(form, "menu_bar_frame", args, n);
    XtManageChild(menu_bar_frame);


    n = 0;
    XtSetArg(args[n], XmNhighlightThickness,     0); n++;
    XtSetArg(args[n], XmNshadowThickness,     0); n++;
	menuBarW = XmCreateMenuBar(menu_bar_frame,"menuBar",args,n);
    XtManageChild(menuBarW);

	/*
	 * Create the "File" menu
	 */
	pullDownW = XmCreatePulldownMenu(menuBarW,"pullDown",NULL,0);
	file = XmStringCreateLtoR (getStr(TXT_file), XmSTRING_DEFAULT_CHARSET);
	fileWidget =XtVaCreateManagedWidget("File",xmCascadeButtonWidgetClass,
				menuBarW,
				XmNlabelString,   file,
				XmNmnemonic,      GetMnemonic(TXT_FILE_MNEMONIC),
				XmNsubMenuId,     pullDownW,
				XmNhighlightThickness,     0,
				NULL);

	XmStringFree(file);
	s = XtVaCreateManagedWidget(getStr(TXT_save),xmPushButtonWidgetClass,
							pullDownW,
							XmNmnemonic,  GetMnemonic(TXT_SAVE_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,saveCB,(XtPointer)ACTIVATE);

	s =XtVaCreateManagedWidget(getStr( TXT_saveAs),xmPushButtonWidgetClass,
							pullDownW,
							XmNmnemonic,  GetMnemonic(TXT_SAVEAS_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,saveAsCB,(XtPointer)ACTIVATE);

	s = XtVaCreateManagedWidget(getStr(TXT_append),xmPushButtonWidgetClass,
							pullDownW,
							XmNmnemonic,  GetMnemonic(TXT_APPEND_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,appendCB,(XtPointer)NULL);

	XtVaCreateManagedWidget("separator",xmSeparatorWidgetClass,
							pullDownW,
							NULL);
	s = XtVaCreateManagedWidget(getStr( TXT_exit ),xmPushButtonWidgetClass,
							pullDownW,
							XmNmnemonic,  GetMnemonic(TXT_EXIT_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,(XtCallbackProc)Quit,NULL);

	/*
	 * Create the "View" pulldown menu
	 */
	view = XmStringCreateLtoR (getStr(TXT_view), XmSTRING_DEFAULT_CHARSET);
	viewPullDownW = XmCreatePulldownMenu(menuBarW,"viewPullDown",NULL,0);
	viewWidget = XtVaCreateManagedWidget("View",xmCascadeButtonWidgetClass,
				menuBarW,
				XmNlabelString,   view,
				XmNmnemonic,      GetMnemonic(TXT_VIEW_MNEMONIC),
				XmNsubMenuId,     viewPullDownW,
				XmNhighlightThickness,     0,
				NULL);

	XmStringFree(view);
	s = XtVaCreateManagedWidget(getStr(TXT_clear),
							xmPushButtonWidgetClass,
							viewPullDownW,
							XmNmnemonic,  GetMnemonic(TXT_CLEAR_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,(XtCallbackProc)Clear,NULL);

	/*
	 * Create the "Options" pulldown menu
	 */
	options = XmStringCreateLtoR (getStr(TXT_options), XmSTRING_DEFAULT_CHARSET);
	optionPullDownW = XmCreatePulldownMenu(menuBarW,"optionPullDown",NULL,0);
	optionWidget = XtVaCreateManagedWidget("Option",xmCascadeButtonWidgetClass,
				menuBarW,
				XmNlabelString,   options,
				XmNmnemonic,      GetMnemonic(TXT_OPTIONS_MNEMONIC),
				XmNsubMenuId,     optionPullDownW,
				XmNhighlightThickness,     0,
				NULL);

	XmStringFree(options);

	/*
	 * Create the time-stamp Optoin
	 */
	s = XtVaCreateManagedWidget( "tstampButton",
            xmToggleButtonWidgetClass, optionPullDownW,
            XmNset, ( app_resources.timeStamp == True ) ? True : False,
            XmNindicatorType, XmONE_OF_MANY,
            XmNvisibleWhenOff, True,
            XmNindicatorOn, True,
			XmNmnemonic,  GetMnemonic(TXT_TSTAMP_MNEMONIC),
            XmNlabelString, XmStringCreateLtoR (getStr(TXT_tstamp_option), XmSTRING_DEFAULT_CHARSET),
            NULL );
   	XtAddCallback(s, XmNvalueChangedCallback,tstampCB,NULL);

	
	/*
	 * Create the pull_right Notify Menu Optoin
	 */
	notify = XmStringCreateLtoR (getStr(TXT_notify_option), XmSTRING_DEFAULT_CHARSET);
    notifyPullDownW = XmCreatePulldownMenu(optionPullDownW, "notify", NULL, 0);
	XtVaSetValues(notifyPullDownW,XmNradioBehavior,True,NULL);
	notifyWidget = XtVaCreateManagedWidget("Notify",
				xmCascadeButtonWidgetClass, optionPullDownW,
				XmNlabelString,   notify,
				XmNmnemonic,      GetMnemonic(TXT_NOTIFY_MNEMONIC),
				XmNsubMenuId,     notifyPullDownW,
				XmNhighlightThickness,     0,
				NULL);
	XmStringFree(notify);

	/*
	 * Create the deiconify and Flash Optoin
	 */
	s  = XtVaCreateManagedWidget( "deiconifyButton",
            xmToggleButtonWidgetClass, notifyPullDownW,
            XmNset, ( app_resources.notifyByDeiconify == True ) ? True : False,
            XmNindicatorType, XmONE_OF_MANY,
            XmNvisibleWhenOff, True,
            XmNindicatorOn, True,
			XmNmnemonic,  GetMnemonic(TXT_DEICONIFY_MNEMONIC),
            XmNlabelString, XmStringCreateLtoR (getStr(TXT_deiconify_option), XmSTRING_DEFAULT_CHARSET),
            NULL );
   	XtAddCallback(s, XmNvalueChangedCallback,notifyByDeiconifyCB,NULL);
	s = XtVaCreateManagedWidget( "flashButton",
            xmToggleButtonWidgetClass, notifyPullDownW,
            XmNset, ( app_resources.notifyByFlash == True ) ? True : False,
            XmNindicatorType, XmONE_OF_MANY,
            XmNvisibleWhenOff, True,
            XmNindicatorOn, True,
			XmNmnemonic,  GetMnemonic(TXT_FLASH_MNEMONIC),
            XmNlabelString, XmStringCreateLtoR (getStr(TXT_flash_option), XmSTRING_DEFAULT_CHARSET),
            NULL );
   	XtAddCallback(s, XmNvalueChangedCallback,notifyByFlashCB,NULL);
	s  = XtVaCreateManagedWidget( "colorChangeButton",
            xmToggleButtonWidgetClass, notifyPullDownW,
            XmNset, ( app_resources.notifyByColorChange == True ) ? True : False,
            XmNindicatorType, XmONE_OF_MANY,
            XmNvisibleWhenOff, True,
            XmNindicatorOn, True,
			XmNmnemonic,  GetMnemonic(TXT_COLORCHANGE_MNEMONIC),
            XmNlabelString, XmStringCreateLtoR (getStr(TXT_colorChange_option), XmSTRING_DEFAULT_CHARSET),
            NULL );
   	XtAddCallback(s, XmNvalueChangedCallback,notifyByColorCB,NULL);
	s = XtVaCreateManagedWidget( "offButton",
            xmToggleButtonWidgetClass, notifyPullDownW,
            XmNset, ( app_resources.notifyByFlash == True  || 
					app_resources.notifyByDeiconify == True ||
					app_resources.notifyByColorChange == True ) ? False : True,
            XmNindicatorType, XmONE_OF_MANY,
            XmNvisibleWhenOff, True,
            XmNindicatorOn, True,
			XmNmnemonic,  GetMnemonic(TXT_OFF_MNEMONIC),
            XmNlabelString, XmStringCreateLtoR (getStr(TXT_off_option), XmSTRING_DEFAULT_CHARSET),
            NULL );
	/*
	 * Create the "Help" pulldown menu
	 */
	help = XmStringCreateLtoR (getStr(TXT_help), XmSTRING_DEFAULT_CHARSET);
	helpPullDownW = XmCreatePulldownMenu(menuBarW,"helpPullDown",NULL,0);
	helpWidget=XtVaCreateWidget("Help",xmCascadeButtonWidgetClass,
				menuBarW,
				XmNlabelString,   help,
				XmNmnemonic,      GetMnemonic(TXT_HELP_MNEMONIC),
				XmNhighlightThickness,     0,
				XmNsubMenuId,     helpPullDownW,
				NULL);

	XmStringFree(help);
	s = XtVaCreateManagedWidget(getStr(TXT_hosmMonitor),xmPushButtonWidgetClass,
							helpPullDownW,
							XmNmnemonic,  GetMnemonic(TXT_XHELP_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,(XtCallbackProc)helpCB,
												(XtPointer)APP_HELP);

	s =XtVaCreateManagedWidget(getStr( TXT_table),xmPushButtonWidgetClass,
							helpPullDownW,
							XmNmnemonic,  GetMnemonic(TXT_TABLE_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,(XtCallbackProc)helpCB,
												(XtPointer)TABLE_HELP);

	s = XtVaCreateManagedWidget(getStr(TXT_helpDesk),xmPushButtonWidgetClass,
							helpPullDownW,
							XmNmnemonic,  GetMnemonic(TXT_HDESK_MNEMONIC),
							NULL);
   	XtAddCallback(s, XmNactivateCallback,(XtCallbackProc)helpCB,
														(XtPointer)HDESK_HELP);

	XtVaSetValues(menuBarW,XmNmenuHelpWidget,helpWidget,NULL);

    XtManageChild(helpWidget);
    XtManageChild(menuBarW);


    return(menu_bar_frame);
}


/*
 * set_dialog_wm_name - this function sets the indicated dialog's name to
 *                      the indicated string.
 */
static void
set_dialog_wm_name( Widget the_dialog, char *the_name)
{
    XtVaSetValues(XtParent(the_dialog), XmNtitle, the_name,NULL);
}


/*
 * helpCB - this function handles the callbacks from the help dialog
 */
void
Help ( Widget w , XEvent *ew, String *params, Cardinal *num_params)
{
	helpCB( w, (XtPointer)APP_HELP,  NULL);
}

/*=======================================================================*/
/* Menu stuff
/*=======================================================================*/


/*==============*/
/* Option Menu 
/*==============*/
static void
notifyByFlashCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	XmToggleButtonCallbackStruct *state =
					 (XmToggleButtonCallbackStruct *) call_data;
    if (state->set)
	{
		app_resources.notifyByFlash  = True;
	}
	else
	{
		app_resources.notifyByFlash  = False;
	}
}
static void
notifyByDeiconifyCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	XmToggleButtonCallbackStruct *state =
					 (XmToggleButtonCallbackStruct *) call_data;
    if (state->set)
	{
		app_resources.notifyByDeiconify  = True;
	}
	else
	{
		app_resources.notifyByDeiconify  = False;
	}
}
static void
notifyByColorCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	XmToggleButtonCallbackStruct *state =
					 (XmToggleButtonCallbackStruct *) call_data;
    if (state->set)
	{
		app_resources.notifyByColorChange  = True;
	}
	else
	{
		app_resources.notifyByColorChange  = False;
	}
}
static void
tstampCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	XmToggleButtonCallbackStruct *state =
					 (XmToggleButtonCallbackStruct *) call_data;
    if (state->set)
	{
		app_resources.timeStamp  = True;
	}
	else
	{
		app_resources.timeStamp  = False;
	}
}
/*==============*/
/* File Menu 
/*==============*/
static void
appendCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	if ( fileNameSet == True && fileName )
	{
		saveData( fileName,APPEND );
	}
	else
	{
		saveAsCB(w,(XtPointer)APPEND,call_data);
	}
}

static void
saveCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	if ( fileNameSet == True && fileName )
	{
		saveData( fileName,SAVE );
	}
	else
	{
		saveAsCB(w,(XtPointer)SAVE,call_data);
	}
}

static  void ghelpCB(Widget wid, XtPointer client_data, XtPointer call_data);

HelpInfo saveInfo =  { NULL, NULL, HELP_FILE_NAME, HELP_SECT_SAVE};

static MenuItems    saveItems[] = {
    {True, TXT_ok,   TXT_OK_MNEMONIC, I_PUSH_BUTTON, NULL, read_name,(XtPointer)ACTIVATE},
    {True, TXT_cancel, TXT_CANCEL_MNEMONIC, I_PUSH_BUTTON, NULL, saveAsCB,CANCEL},
    {True, TXT_help,   TXT_HELP_MNEMONIC, I_PUSH_BUTTON, NULL, ghelpCB, &saveInfo},
    {0, NULL}
};
static MenuGizmo    saveMenu = {
    &saveInfo, "saveMenu", NULL, saveItems, NULL, NULL, XmHORIZONTAL, 1, 0, 1
};

FileGizmo file = {
    &saveInfo, "file", NULL, &saveMenu, NULL, 0,
    NULL, NULL, TXT_pathLabel, TXT_fileLabel, FOLDERS_AND_FILES, NULL, 
};

Gizmo handle = NULL;

static  void
ghelpCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    HelpInfo *help = (HelpInfo *) client_data;

    help->appTitle =
    help->title = GetGizmoText(TXT_saveAsDName);
    help->section = GetGizmoText(STRDUP(help->section));
    PostGizmoHelp(GetFileGizmoShell(handle), help);
}


static void
saveAsCB(Widget w,XtPointer client_data,XtPointer call_data)
{
	XmString prompt;
    Arg arglist[20];
    Cardinal num_args;
	Widget text;
	int which_action;
	XmString okXmString;
	XmString cancelXmString;
	static int  first_time = 1;

    which_action = ((int) client_data) & 0x00FF;

    switch(which_action)
    {
    	case CANCEL:
			XtPopdown(dialog);
            break;
    	case ACTIVATE:
    	case APPEND:
    	case SAVE:
        	if (dialog == NULL)
        	{
    			if (first_time) 
				{
					DtInitialize (top);
					DtiInitialize (top);
        			InitializeGizmos(TXT_Title);
        			first_time = 0;
    			}

				file.directory = dirName;
        		file.dialogType = FOLDERS_AND_FILES;
        		saveItems[0].clientData = (XtPointer)which_action;

				handle = CreateGizmo(top, FileGizmoClass, &file, NULL, 0);
        		dialog = GetFileGizmoShell(handle);
		    	SetFileGizmoInputField(handle, defLogFileName);
		    	saveInfo.appTitle =
		    		saveInfo.title = GetGizmoText(TXT_saveAsDName);

        	}
			/*
			 * Save the action to perform away, so read_name can get the
			 * correct action
			 */
       		saveItems[0].clientData = (XtPointer)which_action;

			if ( which_action == APPEND )
       			XtVaSetValues(dialog, XmNtitle, getStr( TXT_append ), NULL);
			else
       			XtVaSetValues(dialog, XmNtitle, getStr( TXT_saveAsDName ), NULL);
    		SelectFileGizmoInputField(handle);
    		MapGizmo(FileGizmoClass, handle);
            break;
        default:
        	break;
    }
}

/* gizmos for " ok to overwrite"  prompt */
static MenuItems overWriteMenuItems[] = {
 { True, TXT_ok,  TXT_OK_MNEMONIC,  I_PUSH_BUTTON, NULL, okToOverWrite,(XtPointer)NULL},
 { True, TXT_cancel, TXT_CANCEL_MNEMONIC, I_PUSH_BUTTON, NULL, notOkToOverWrite,(XtPointer)NULL},
 { NULL }
};

static MenuGizmo overWriteMenu = {
    NULL, "overWriteMenu", "", overWriteMenuItems, NULL, NULL,
    XmHORIZONTAL, 1, 1
};

static ModalGizmo overWriteGizmo = {
    NULL,                   /* help info */
    "overWriteGizmo",       /* shell name */
    TXT_warningDName,       /* title */
    &overWriteMenu,         /* menu */
    TXT_fileExists,         /* message */
    NULL,                   /* gizmos */
    0,                      /* num_gizmos */
    XmDIALOG_PRIMARY_APPLICATION_MODAL, /* style */
    XmDIALOG_WARNING,       /* type */
};

static void
read_name(Widget w, int which_action,XmSelectionBoxCallbackStruct *cbs)
{
	Widget text;
	XmString prompt;
    Arg arglist[20];
    Cardinal num_args;
	XmString okXmString;
	XmString cancelXmString;
	char *tmp;
	char *saveFileName;
	struct stat sbuf;
	int i;
	
	ExpandFileGizmoFilename(handle);
	fileName = strdup(GetFilePath(handle));

	/*
	 * Get the action to perform, can't use clientData anymore.
 	 * Don't know how to update a MenuItem Gizmo's clientData
	 * dynamically.
	 */
	which_action = (int)saveItems[0].clientData;

	i = stat(fileName,&sbuf);
	if ( i != -1 &&  (sbuf.st_mode & S_IFDIR ))
	{
		/*
		 * Has to be a file
		 */
		return;
	}
	if (( which_action & SAVE) && ( access(fileName, F_OK) == 0 ))
	{
		overWriteMenuItems[0].clientData = (XtPointer)which_action;
		warningdialog = 
			CreateGizmo(top, ModalGizmoClass, &overWriteGizmo, NULL, 0);
		MapGizmo(ModalGizmoClass, warningdialog );
		return;
	}
	fileNameSet = True;
	saveData(fileName,which_action);
}
static void
okToOverWrite(Widget w, int which_action,XmSelectionBoxCallbackStruct *cbs)
{
	Widget shell;

	fileNameSet = True;
	saveData(fileName,which_action);
   	shell = GetModalGizmoShell(warningdialog);
    	FreeGizmo(ModalGizmoClass,warningdialog );
    	XtDestroyWidget(shell);
}
static void
notOkToOverWrite(Widget w, int which_action,XmSelectionBoxCallbackStruct *cbs)
{
	Widget shell;

	fileNameSet = False;
   	shell = GetModalGizmoShell(warningdialog);
    	FreeGizmo(ModalGizmoClass,warningdialog );
    	XtDestroyWidget(shell);
}

static MenuItems errorNoticeMenuItems[] = {
 { True, TXT_ok,  TXT_OK_MNEMONIC,  I_PUSH_BUTTON, NULL, errorNoticePopdownCB,(XtPointer)NULL},
 { NULL }
};

static MenuGizmo errorNoticeMenu = {
    NULL, "errorNoticeMenu", "", errorNoticeMenuItems, NULL, NULL,
    XmHORIZONTAL, 1, 1
};

static ModalGizmo errorNoticeGizmo = {
    NULL,                   /* help info */
    "errorNoticeGizmo",       /* shell name */
    TXT_errorDName,       /* title */
    &errorNoticeMenu,         /* menu */
    NULL,                   /* message */
    NULL,                   /* gizmos */
    0,                      /* num_gizmos */
    XmDIALOG_PRIMARY_APPLICATION_MODAL, /* style */
    XmDIALOG_ERROR,       /* type */
};
static void
errorNoticePopdownCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown(DtGetShellOfWidget(widget));
}

static void
saveData(char *fileName, int which_action )
{
	FILE *fp;
	char *data;
	XmString prompt;
    Arg arglist[20];
    Cardinal num_args;
	Widget error;
	int length;
	int writeLen;
	char w_action[] = "w";
	char a_action[] = "a";
	char *opType;
	XmString okXmString;

	if ( which_action == APPEND )
		opType = a_action;
	else
		opType = w_action;

	if ((fp = fopen(fileName, opType)) == NULL)
	{
		errorNoticeGizmo.message = TXT_fileOpenErr;
		error = CreateGizmo(top, ModalGizmoClass, &errorNoticeGizmo, NULL, 0);
		MapGizmo(ModalGizmoClass, error);
		fileNameSet = False;
		return;
	}
	else
	{
	XtPopdown(dialog);
        length = XmTextGetLastPosition(text);
		data = XmTextGetString(text);
		/*
         * Write out the information
         */
        writeLen = fwrite((char *)data,1,length,fp);
		if ( length != writeLen )
		{
			errorNoticeGizmo.message = TXT_fileWriteError;
			error = CreateGizmo(top, ModalGizmoClass, &errorNoticeGizmo, NULL, 0);
			MapGizmo(ModalGizmoClass, error);
		}
		fclose(fp);
		XtFree(data);
	}
}

static void
StartupTimeOutCB(caddr_t data, XtIntervalId *tid)
{
	startupDelayOver = True;
}

/*------------------------------------------------------------------------*/
#ifdef SELECTION 
static void
CloseConsole ()
{
    if (input) 
    {
	    XtRemoveInput (input_id);
	    fclose (input);
    }
}

static Boolean
ConvertSelection (w, selection, target, type, value, length, format)
    Widget w;
    Atom *selection, *target, *type;
    XtPointer *value;
    unsigned long *length;
    int *format;
{
    Display* d = XtDisplay(w);
    XSelectionRequestEvent* req =
	XtGetSelectionRequest(w, *selection, (XtRequestId)NULL);

    if (*target == XA_TARGETS(d)) {
	    Atom* targetP;
	    Atom* std_targets;
	    unsigned long std_length;
	    XmuConvertStandardSelection(w, req->time, selection, target, type,
				      (caddr_t*)&std_targets, &std_length, format);
	    *value = (XtPointer)XtMalloc(sizeof(Atom)*(std_length + 5));
	    targetP = *(Atom**)value;
	    *targetP++ = XA_STRING;
	    *targetP++ = XA_TEXT(d);
	    *targetP++ = XA_LENGTH(d);
	    *targetP++ = XA_LIST_LENGTH(d);
	    *targetP++ = XA_CHARACTER_POSITION(d);
	    *length = std_length + (targetP - (*(Atom **) value));
	    bcopy((char*)std_targets, (char*)targetP, sizeof(Atom)*std_length);
	    XtFree((char*)std_targets);
	    *type = XA_ATOM;
	    *format = 32;
	    return True;
    }

    if (*target == XA_LIST_LENGTH(d) ||
	*target == XA_LENGTH(d))
    {
    	long * temp;
    	
    	temp = (long *) XtMalloc(sizeof(long));
    	if (*target == XA_LIST_LENGTH(d))
      	  *temp = 1L;
    	else			/* *target == XA_LENGTH(d) */
      	  *temp = (long) TextLength (text);
    	
    	*value = (XtPointer) temp;
    	*type = XA_INTEGER;
    	*length = 1L;
    	*format = 32;
    	return True;
    }
    
    if (*target == XA_CHARACTER_POSITION(d))
    {
    	long * temp;
    	
    	temp = (long *) XtMalloc(2 * sizeof(long));
    	temp[0] = (long) 0;
    	temp[1] = TextLength (text);
    	*value = (XtPointer) temp;
    	*type = XA_SPAN(d);
    	*length = 2L;
    	*format = 32;
    	return True;
    }
    
    if (*target == XA_STRING ||
      *target == XA_TEXT(d) ||
      *target == XA_COMPOUND_TEXT(d))
    {
	    extern char *_XawTextGetSTRING();
    	if (*target == XA_COMPOUND_TEXT(d))
	        *type = *target;
    	else
	        *type = XA_STRING;
	    *length = TextLength (text);
    	*value = (XtPointer)XmTextGetString(text);
    	*format = 8;
	/*
	 * Drop our connection to the file; the new console program
	 * will open as soon as it receives the selection contents; there
	 * is a small window where console output will not be redirected,
	 * but I see no way of avoiding that without having two programs
	 * attempt to redirect console output at the same time, which seems
	 * worse
	 */
	    CloseConsole ();
    	return True;
    }
    
    if (XmuConvertStandardSelection(w, req->time, selection, target, type,
				    (caddr_t *)value, length, format))
	    return True;

    return False;
}

static void
LoseSelection (w, selection)
    Widget  w;
    Atom    *selection;
{
    Quit ();
}

static void
InsertSelection (w, client_data, selection, type, value, length, format)
    Widget	    w;
    XtPointer	    client_data;
    Atom	    *selection, *type;
    XtPointer	    value;
    unsigned long   *length;
    int		    *format;
{
    if (*type != XT_CONVERT_FAIL)
	TextInsert (text, (char *) value, *length);
    XtOwnSelection(top, mit_console, CurrentTime,
		   ConvertSelection, LoseSelection, NULL);
    OpenConsole ();
}


#endif  /* SELECTION */


