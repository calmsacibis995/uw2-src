#ident	"@(#)umailsetup:setup.C	1.11"
//	General Mail Setup

#include	<iostream.h>		//  for cout()
#include	<stdlib.h>		//  for exit()
#include	<unistd.h>		//  for sleep()

//  On production apps, do we allow editres commuication???
#include	<X11/Xmu/Editres.h>	//  so editres can communicate w/ us

#include	<X11/cursorfont.h>	//  to get the watch and pointer cursors

#include	"actionArea.h"		//  for ActionAreaItem definition
#include	"dtFuncs.h"		//  for HelpText, GUI group library func defs
#include	"setup.h"		//  for the AppStruct definition
#include	"setupAPIs.h"		//  includes all needed setup APIs
#include	"setup_txt.h"		//  the localized message database



//  External functions, variables, etc.

extern Widget createSetupWindow (Widget topLevelParent, Widget *actionArea);
extern void   doHelpActionCB (Widget, XtPointer, XmAnyCallbackStruct *);
extern void   errorDialog (Widget topLevel, char *errorText, setupVar_t *curVar);
extern void   setActionAreaHeight (Widget actionArea);
extern void   setButtonSensitivity (ActionAreaItem *buttons, int thisButton,
							      Boolean sensitivity);
extern void	setVariableFocus (setupVar_t *var);
extern ActionAreaItem	actionItems[];



//  Local functions, variables, etc.

void	main (int argc, char **argv);

static Widget	createTopLevelShell (XtAppContext *appContext, int *argc, char **argv);
static void	getWebInitValues (void);
static void	setupAppInit (void);
static void	createWatchCursor (void);
static void	exitSetupWinCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *);

void	setCursor (int cursorType, Window window, Boolean flush);
void	shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs);

AppStruct	app = {0};
HelpText	setupHelp;

static char	*localizedErrorMsg = (char *)0;

XtResource resources[] =
{
    {   //  Debug level for SetupApp (0=OFF, 1=ERR, 2=API, 3=FUNC, 4=PWD, 5=ALL)
        (String)"cDebugLevel",  "CDebugLevel",    XtRInt,           sizeof(int),
        XtOffset (RDataPtr, cDebugLevel),         XtRString,        "0"             },
    {   //  Debug file name for SetupApp client debugging stmts (used if cDebugLevel)
        (String)"cLogFile",     "CLogFile",       XtRString,        sizeof(String),
        XtOffset (RDataPtr, cLogFile),            XtRString,        (String)0       },
    {   //  Debug level for setup APIs (0=OFF, 1-10 are different levels (min to max))
        (String)"sDebugLevel",  "SDebugLevel",    XtRInt,           sizeof(int),
        XtOffset (RDataPtr, sDebugLevel),         XtRString,        "0"             },
};


XrmOptionDescRec cmdLineOptions[] =
{
    {   "-cDebugLevel",         "cDebugLevel",    XrmoptionSepArg,    (caddr_t)0    },
    {   "-cLogFile",            "cLogFile",       XrmoptionSepArg,    (caddr_t)0    },
    {   "-sDebugLevel",         "sDebugLevel",    XrmoptionSepArg,    (caddr_t)0    },
};






/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void main (int argc, char **argv)
//
//  DESCRIPTION:
//	Create the general Setup application.
//
//  RETURN:
//	Nothing.
//

void
main (int argc, char **argv)
{
	Widget	setupWin;

	XtSetLanguageProc (0, 0, 0);

	app.topLevel = createTopLevelShell (&app.appContext, &argc, argv);
	app.display = XtDisplay (app.topLevel);

	XtGetApplicationResources (app.topLevel, &app.rData, resources,
						XtNumber (resources), NULL, 0);
	//  We've got the resources, now initialize debugging
	if (!(cDebugInit (app.rData.cDebugLevel, (char *)app.rData.cLogFile,
								argc, C_ALL)))
	{
		if (app.rData.cDebugLevel < 0 || app.rData.cDebugLevel > C_ALL)
			log2(C_ERR,"ERR: Invalid cDebugLevel ",app.rData.cDebugLevel);
		else
			log2(C_ERR,"ERR: Can't open cLogFile ", app.rData.cLogFile);

		app.rData.cDebugLevel = 0;
	}

	log6 (C_ALL, "\ncDebug=", app.rData.cDebugLevel, ", logFile=",
			    app.rData.cLogFile, ", sDebug=", app.rData.sDebugLevel);

	getWebInitValues ();
	setupAppInit ();

	setupWin = createSetupWindow (app.topLevel, &app.actionArea);

	//  Add the XmNhelpCallback to the highest level form for the "Help" key
	//  (i.e. F1).  Unfortunately, we can't set it on the topLevel widget.
	XtAddCallback (setupWin, XmNhelpCallback,
				(XtCallbackProc)doHelpActionCB, (XtPointer)0);

	XtRealizeWidget (app.topLevel);

	//  Get and set the height of the action area.  We want this area to
	//  remain the same height while the control area gets larger, when
	//  the user resizes the window larger.  Again, unfortunately, we
	//  can't seem to do this before realization time.
	setActionAreaHeight (app.actionArea);

	XtSetMappedWhenManaged (app.topLevel, True);
	XtMapWidget (app.topLevel);

	//  Get the window id now (unfortunately we can't get it before realization
	app.window = XtWindow (app.topLevel);

	//  Set the cursor for this app as the pointer (arrow) cursor
	setCursor (C_POINTER, app.window, C_FLUSH);

	//  If we have a Category option menu (basic/extended), we need to set
	//  the input focus to the first variable in the list and make sure the
	//  descriptive text for that variable gets displayed.  We couldn't really
	//  do it earlier because we weren't realized.  We don't need to do this
	//  if we don't have the Category menu, since in that case, the first
	//  variable in the list automatically gets focus.
	if (app.haveBasic && app.haveExtended)
			setVariableFocus (app.firstVar);

	if (localizedErrorMsg)
		errorDialog (app.topLevel, localizedErrorMsg, (setupVar_t *)0);

	XtAppMainLoop (app.appContext);

}	//  End  main ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static Widget createTopLevelShell (XtAppContext *appContext, int *argc,
//									char **argv)
//
//  DESCRIPTION:
//	Create the topLevel shell for the application.
//
//  RETURN:
//	The widget id of the the top level shell.
//

static Widget
createTopLevelShell (XtAppContext *appContext, int *argc, char **argv)
{
	Widget	topLevel;


	topLevel = XtVaAppInitialize (appContext, "setup", cmdLineOptions,
				XtNumber(cmdLineOptions), argc, (String *)argv,
				(String *)0, XtNmappedWhenManaged, (XtArgVal)False,
				(String)0);
	return (topLevel);

}	//  End  createTopLevelShell ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void getWebInitValues (void)
//
//  DESCRIPTION:
//	Get the name of this executable name so we can let the setupWebNew API
//	create the web using the setup.def file. Also see what kind of modes
//	(extended and/or basic) we have available, get the app window title,
//	the icon filename, and the icon title.
//
//  RETURN:
//	Nothing.
//

static void
getWebInitValues (void)
{
	String		className;


	log1 (C_FUNC, "getWebInitValues(void)");

	//  Get base name of this executable (not full path).  Don't ever modify or
	//  or free app.execName or className! (They're owned by intrinsics).
	XtGetApplicationNameAndClass (app.display, &app.execName, &className);

	//  Make the web out of the setup definition file and the associated files.
	if ((app.web = setupWebNew (app.execName)) == (setupWeb_t *)0)
	{
		//  Can't read the setup file.
		log2 (C_API, "setupWebNew() failed: web=", app.web);
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_setupWebNoAccess;
		app.title = TXT_appNoName;
		app.iconTitle = TXT_iconNoName;
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
		return;
	}

	//  Find out if we have the "basic" and "extended" mode capabilities.
	//  We'll use this info later to determine if we even need to display
	//  the "basic/extended" option menu.
	app.haveBasic    = (setupWebMoveExtended (app.web, False)) ?  True : False;
	app.haveExtended = (setupWebMoveExtended (app.web, True))  ?  True : False;

	//  We've got to have at least 1 mode available, or we have nothing to display.
	if (!app.haveBasic && !app.haveExtended)
	{
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_noModes;
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
	}

	//  Get the app title, the icon file and icon title for our app.
	app.title = setupWebTitle (app.web);
	if (!app.title)
		app.title = getStr (TXT_appNoName);

	app.iconFile = setupWebIconFilename (app.web);
	if (!app.iconFile)
		app.iconFile = getStr (TXT_defaultIconFile);

	app.iconTitle = setupWebIconTitle (app.web);
	if (!app.iconTitle)
		app.iconTitle = getStr (TXT_iconNoName);

	//  Get the Help file, section, and title from the web if it's there.
	setupHelp.file = setupWebHelpFile (app.web);
	if (!setupHelp.file)
	{
		setupHelp.file = HELP_FILE;
		setupHelp.title = TXT_appHelpTitle;
		setupHelp.section = TXT_appHelpSection;
	}
	else
	{
		setupHelp.title = setupWebHelpTitle (app.web);
		if (!setupHelp.title)
			setupHelp.title = TXT_appHelpTitle;

		setupHelp.section = setupWebHelpSection (app.web);
		if (!setupHelp.section)
			setupHelp.section = TXT_appHelpSection;
	}

}	//  End  getWebInitValues ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void setupAppInit (void)
//
//  DESCRIPTION:
//	Get the height and width of this user's display, set the max height and
//	width for the app, set up the exit callback for when the application exits,
//	whether by the mwm "Close" button, or by local exits via XtDestroyWidget(),
//	add the event to allow EditRes to communicate with this application,
//	set the app title, the icon pixmap, and create the wait (watch) and
//	pointer (arrow) cursors.
//
//  RETURN:
//	Nothing.
//

static void
setupAppInit (void)
{

	log1 (C_FUNC, "setupAppInit(void)");

	//  Don't allow this app to be larger than the user's screen resolution.
	//  Also set the app title.
	XtVaSetValues (app.topLevel,
		XmNmaxHeight, DisplayHeight(app.display, DefaultScreen (app.display)),
		XmNmaxWidth,  DisplayWidth (app.display, DefaultScreen (app.display)),
		XmNtitle,     app.title,
		XmNdeleteResponse, XmDESTROY,
		NULL);

	XtAddCallback (app.topLevel, XmNdestroyCallback,
			    (XtCallbackProc)exitSetupWinCB, (XtPointer)app.web);

	//  Allow EditRes to speak with this appplication.
        XtAddEventHandler (app.topLevel, (EventMask)0, True,
				(XtEventHandler)_XEditResCheckMessages, NULL);

	//  Create pixmap for icon
	getIconPixmap (app.display, app.iconFile, &(app.iconPixmap));
	setIconPixmap (app.topLevel, &(app.iconPixmap), app.iconTitle);

	createWatchCursor ();

	if (!setupWebPerm (app.web))
	{
		if (!localizedErrorMsg)
			localizedErrorMsg = TXT_noPerms;
		setButtonSensitivity (actionItems, OK_BUTTON, False);
		setButtonSensitivity (actionItems, RESET_BUTTON, False);
	}

}	//  End  setupAppInit ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void createWatchCursor (void)
//
//  DESCRIPTION:
//	Create (and save for future use) the watch cursor.  We use the 
//	"default" cursor as the pointer cursor so as not to be "different".
//
//  RETURN:
//	Nothing.
//

static void
createWatchCursor (void)
{
	app.cursorWait = XCreateFontCursor (app.display, XC_watch);

	if (app.cursorWait == BadAlloc || app.cursorWait == BadFont
						|| app.cursorWait == BadValue)
	{
		app.cursorWait = 0;
	}

}	//  End  createWatchCursor ()



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void setCursor (int cursorType, Window window, Boolean flush)
//
//  DESCRIPTION:
//	Set the cursor to the cursorType, and make the X library send it to the
//	Server NOW if flush is True.
//
//  RETURN:
//	Nothing.
//

void
setCursor (int cursorType, Window window, Boolean flush)
{

	if (cursorType == C_WAIT)	//  cursor "wait"
		XDefineCursor (app.display, window, app.cursorWait);
	else				//  cursor "pointer"
		//  "None" as the cursor means use the parent's cursor, or,
		//  if the parent is the root window, use the default cursor.
		XDefineCursor (app.display, window, None);

	if (flush)
		XFlush (app.display);
}



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	static void exitSetupWinCB (Widget w, XtPointer clientData,
//							XmAnyCallbackStruct *)
//
//  DESCRIPTION:
//	This is the XmNdestroyCallback, and it gets called when the application
//	exits via an mwm destroy or when the app calls XtDestroyWidget().
//	Since this function performs all necessary application exiting functions
//	(frees all widget-associated resources, calls XDestroyWindow, etc.),
//	it SHOULD be called (via XtDestroyWidget (&app)) each time the app
//	wishes to exit.
//
//  RETURN:
//	Nothing.
//	Note:  There is no return from here.
//

static void
exitSetupWinCB (Widget w, XtPointer clientData, XmAnyCallbackStruct *)
{
	setupWeb_t	*web = (setupWeb_t *)clientData;


	log1 (C_FUNC, "exitSetupWinCB()");

	// Free up the currently used setupVar clientData !!!!!!

	setupWebReset (web);
	//  What does the return value of setupWebFree() mean????
//	setupWebFree (app.web);	//  Causes segv core dump now.
	exit (0);
}



/////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION:
//	void shellWidgetDestroy (Widget w, XtPointer clientData,
//						XmAnyCallbackStruct *cbs)
//
//  DESCRIPTION:
//	This gets called when it is desired to destroy a shell widget and
//	all its descendants.  It finds the shell widget using the widget
//	received, then destroys it.  But, the callback associated with the
//	XmNdestroyCallback resource gets called automatically before the
//	widget is actually destroyed.  The destroy callback itself then
//	performs all the necessary tasks (such as freeing up memory, etc.).
//
//	This function should be called when an XtDestroyWidget() call has
//	NOT already been made, like in a button callback (such as for the
//	Cancel button, or in a menubar exit button callback function).
//	It should not be used when an XtDestroyWidget() call has been made
//	(like in a window manager exit such as when the user presses Alt-F4,
//	(or selects the window manager Close button or double-clicks on the
//	window manager button).
//
//  RETURN:
//	Nothing.
//

void
shellWidgetDestroy (Widget w, XtPointer clientData, XmAnyCallbackStruct *cbs)
{
	log1 (C_FUNC, "shellWidgetDestroy()");

	while (w && !XtIsWMShell (w))
		w = XtParent (w);

	XtDestroyWidget (w);

	log1 (C_ALL, "\tJust destroyed shell widget");
	log1 (C_FUNC, "End  shellWidgetDestroy()");

}	//  End  shellWidgetDestroy ()
