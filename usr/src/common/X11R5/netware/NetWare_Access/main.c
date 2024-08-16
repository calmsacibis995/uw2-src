/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:main.c	1.5"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/main.c,v 1.5 1994/09/01 16:32:42 renuka Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#include <stdio.h>
#include <pwd.h>
#include <locale.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/RubberTile.h>
#include <Xol/Panes.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/Form.h>
#include <Xol/OlCursors.h>
#include <Xol/StaticText.h>

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>

#include "main.h"

/****************************************************************
	enum declaration for menu buttons
*****************************************************************/
enum {
    Actions_Button, View_Button, Help_Button,
};
enum {
	Format_Button,
};

/****************************************************************
	forward declarations
*****************************************************************/
void		HelpCB (Widget, XtPointer, XtPointer);
static void	AddCB (Widget, XtPointer, XtPointer);
static void	ExitCB (Widget, XtPointer, XtPointer);
static void	SingleLoginCB (Widget, XtPointer, XtPointer);
static void	DisplayHelp (Widget , HelpText *);
static void     MakeIcon (Widget);
static void     setsinglelogin (Widget);
static char    	*getsingleloginfile(); 
static char	*AppTitle;

extern void	NewUserCB (Widget, XtPointer, XtPointer);
extern void	SaveSelectCB (Widget, XtPointer, XtPointer);
extern void	ViewCB (Widget, XtPointer, XtPointer);
extern void	UpdateCB (Widget, XtPointer, XtPointer);
extern void	SetPrimaryServerCB (Widget, XtPointer, XtPointer);

/****************************************************************
	global defines	
*****************************************************************/
#define 	SectionTag			"10"
#define		USER_SINGLELOGIN		".slogin" 
#define		SYSTEM_SINGLELOGIN		"/etc/.slogin" 
#define		SLASH				"/" 
#define		ENABLE				1
#define		DISABLE				0
#define		SingleLogin_Button		3

/***************************************************************
* 		Menu items and fields 
***************************************************************/
static String	MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

static int	NumMenuFields = XtNumber (MenuFields);

/*******************************************************************
		 Menu Bar Items 
*********************************************************************/
static MenuItem MenuBarItems [] = {
    { (XtArgVal) TXT_actions, (XtArgVal) MNEM_actions, (XtArgVal) True,
	  (XtArgVal) 0, (XtArgVal) True, },		/* Actions */
    { (XtArgVal) TXT_view, (XtArgVal) MNEM_view, (XtArgVal) True,
	  (XtArgVal) 0, },		/* View */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Help */
};

/*******************************************************************
	MENU ITEMS IN THE PULLDOWN MENU
***********************************************************************/
static MenuItem ActionItems [] = {
    { (XtArgVal) TXT_new, (XtArgVal) MNEM_new, (XtArgVal) True,
	  (XtArgVal) NewUserCB, },			/* New User */
    { (XtArgVal) TXT_saveselect, (XtArgVal) MNEM_saveselect, 
	(XtArgVal) True, (XtArgVal) SaveSelectCB, },	/* save_select */
    { (XtArgVal) TXT_update, (XtArgVal) MNEM_update, 
	(XtArgVal) True, (XtArgVal) UpdateCB, },	/* update */
    { (XtArgVal) TXT_singleloginenabled, (XtArgVal) MNEM_singleloginenabled, 
	(XtArgVal) True, (XtArgVal) SingleLoginCB, },	/* SingleLoginCB */
    { (XtArgVal) TXT_setprimaryserver, (XtArgVal) MNEM_setprimaryserver, 
	(XtArgVal) True, (XtArgVal) SetPrimaryServerCB, },
							/* SetPrimaryServerCB */
    { (XtArgVal) TXT_exit, (XtArgVal) MNEM_exit, (XtArgVal) True,
	  (XtArgVal) ExitCB, },				/* Exit */
};

/*******************************************************************
	MENU ITEMS IN THE PULLDOWN MENU
***********************************************************************/
static MenuItem ViewItems [] = {
    { (XtArgVal) TXT_format, (XtArgVal) MNEM_format, (XtArgVal) True,
	  (XtArgVal) 0, },			/* format */
};

/*******************************************************************
	MENU ITEMS IN THE PULLDOWN MENU
***********************************************************************/
static MenuItem FormatItems [] = {
    { (XtArgVal) TXT_long, (XtArgVal) MNEM_long, (XtArgVal) True,
	  (XtArgVal) ViewCB, },			/* Long format */
    { (XtArgVal) TXT_short, (XtArgVal) MNEM_short, (XtArgVal) True,
	  (XtArgVal) ViewCB, },			/* short format */
};

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText AppHelp = {
    TXT_appHelp,  SectionTag,
};

static HelpText TOCHelp = {
    TXT_tocHelp, 0, 
};

static MenuItem HelpItems [] = {
    { (XtArgVal) TXT_application, (XtArgVal) MNEM_application,(XtArgVal)True,
	  (XtArgVal) HelpCB, (XtArgVal) True,
	  (XtArgVal) &AppHelp, },			/* Application */
    { (XtArgVal) TXT_TOC, (XtArgVal) MNEM_TOC, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &TOCHelp, },		/* Table o' Contents */
    { (XtArgVal) TXT_helpDesk, (XtArgVal) MNEM_helpDesk, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) 0, },				/* Help Desk */
};

static XrmOptionDescRec	Options [] =
{
	{ "-o", ".administrator", XrmoptionNoArg, (XtPointer) "True" },
};

/****************************************************************
	global declarations
******************************************************************/
Boolean			endApplication;
Widget			TopLevel;

/****************************************************************
		 extern functions 
******************************************************************/
extern void     	SetLabels (MenuItem *, int);
extern void		SetHelpLabels (HelpText *);
extern void		GUIError (Widget, char *);
extern char		*GetStr (char *idstr);
extern uid_t	 	getuid();
extern struct passwd 	*getpwuid ();
extern int   		BuildScrolledList (Widget, Widget);

/***************************************
	main program starts here
****************************************/
main (argc, argv)
    int		argc;
    char	**argv;
{
    	Widget		form, panes, menuBar, ActionMenu, ViewMenu;
    	Widget		SingleLoginMenu, FormatMenu;
    	Cardinal	numActionItems;
    	register	i;
    	char        	*title_header;
    	XtAppContext	AppContext;
	uid_t       	uid;
	struct passwd   *pwd;

	/* Set the boolean to end application to false 
	 */
	endApplication = False;

	/* Set the Language Proc call for localization
	 */
    	XtSetLanguageProc(NULL, NULL, NULL);

	/* Initialize the toolkit
	 */
    	OlToolkitInitialize (&argc, argv, NULL);

	/* Initiliaze the toplevel widget
	 */
    	TopLevel = XtAppInitialize(
			&AppContext,		/* app_context_return	*/
			APPNAME,		/* application_class	*/
			Options,		/* options		*/
			XtNumber (Options),	/* num_options		*/
			&argc,			/* argc_in_out		*/
			argv,			/* argv_in_out		*/
			(String *) NULL,	/* fallback_resources	*/
			(ArgList) NULL,		/* args			*/
			(Cardinal) 0		/* num_args		*/
    	);

	/* Set the application title
	 */
    	AppTitle = GetStr (TXT_appName);

	/*
	 * Re-enable access control if we are directed to - from newuser
	 */
	if ( argc > 1 && argv[1][1] == 'a' ) {
#ifdef DEBUG
printf("access control argument recieved, enabling access control\n");
#endif
		XEnableAccessControl(XtDisplay(TopLevel));
		XFlush(XtDisplay(TopLevel));
	}

    	/*******************************************************
	get the login name for the title of the screen 
    	*******************************************************/
    	uid = getuid ();
/*  Release 1.0 workaround for security - removed and replaced by filepriv
    in the package installation.
	setuid (0);
	seteuid (uid);
*/
    	pwd = getpwuid (uid);
    	title_header = (char *) XtMalloc (strlen(GetStr (TXT_appTitle)) + 
		+ strlen (GetStr (TXT_for)) +  strlen (pwd->pw_name) + 1); 
    	strcpy (title_header, GetStr (TXT_appTitle));
    	strcat (title_header, GetStr (TXT_for));
    	strcat (title_header, pwd->pw_name); 
    	XtVaSetValues (TopLevel, XtNtitle,(XtArgVal) title_header, 
				XtNresizable,		(XtArgVal) True,
				0);
    	XtFree (title_header);
	
    	/************************************************************ 
		initialize the desktop over here 
    	**********************************************************/ 
	DtInitialize (TopLevel); 
    	/************************************************************* 
		create the panes container to hold the others 
	*************************************************************/ 
	panes = XtVaCreateManagedWidget ("form", rubberTileWidgetClass,
			TopLevel, 
			XtNshadowThickness,	(XtArgVal) 0, 
			XtNsetMinHints,		(XtArgVal) False, 
			XtNorientation,		(XtArgVal) OL_VERTICAL, 
			0); 

    	/************************************************************* 
		create the forms under panes container to manage
	*************************************************************/ 
	form = XtVaCreateManagedWidget ("panes", panesWidgetClass,
			panes, XtNshadowThickness,	(XtArgVal) 2,
				XtNweight,		(XtArgVal) 0,
			0);
		
    	/************************************************************* 
		Set the labels here with the help labels
	*************************************************************/ 
	SetLabels (MenuBarItems, XtNumber (MenuBarItems)); 
	SetLabels (ActionItems, XtNumber (ActionItems)); 
	SetLabels (ViewItems, XtNumber (ViewItems)); 
	SetLabels (FormatItems, XtNumber (FormatItems)); 
	SetLabels (HelpItems, XtNumber (HelpItems)); 
	SetHelpLabels (&AppHelp);
    	SetHelpLabels (&TOCHelp);

    	numActionItems = XtNumber (ActionItems);

    	/************************************************************* 
		Create the Actions menu button with its Pull down menu
	*************************************************************/ 
    	MenuBarItems [Actions_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("actionMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    	ActionMenu = XtVaCreateManagedWidget ("actionMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [Actions_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) ActionItems,
		XtNnumItems,		(XtArgVal) numActionItems,
		0);

    	/************************************************************* 
		Create the View menu button with its Pull down menu
	*************************************************************/ 
    	ViewItems [Format_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("formatMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    	FormatMenu = XtVaCreateManagedWidget ("formatMenu",
		flatButtonsWidgetClass,
	        (Widget) ViewItems [Format_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) FormatItems,
		XtNnumItems,		(XtArgVal) XtNumber (FormatItems),
		0);

    	MenuBarItems [View_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("viewMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    	ViewMenu = XtVaCreateManagedWidget ("viewMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [View_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) ViewItems,
		XtNnumItems,		(XtArgVal) XtNumber (ViewItems),
		0);

    	/************************************************************* 
		Create the help menu button with its Pull down menu
	*************************************************************/ 
    	MenuBarItems [Help_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("helpMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    	(void) XtVaCreateManagedWidget ("helpMenu",
		flatButtonsWidgetClass,
	        (Widget) MenuBarItems [Help_Button].subMenu,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) HelpItems,
		XtNnumItems,		(XtArgVal) XtNumber(HelpItems),0);

    	menuBar = XtVaCreateManagedWidget ("menuBar",
		flatButtonsWidgetClass, form,
		XtNgravity,		(XtArgVal) NorthWestGravity,
		XtNmenubarBehavior,	(XtArgVal) True,
		XtNhPad,		(XtArgVal) 6,
		XtNvPad,		(XtArgVal) 6,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) MenuBarItems,
		XtNnumItems,		(XtArgVal) XtNumber (MenuBarItems),
		XtNweight,		(XtArgVal) 0,
		0);

    	/************************************************************* 
		Build the scroll list for display - server list
	*************************************************************/ 
    	if (!BuildScrolledList (panes, menuBar))	{
    		MakeIcon (TopLevel);
    		XtRealizeWidget (TopLevel);
		XSync (XtDisplay (TopLevel), False);

		/* Set the single login state here after the Toplevel 
		 * widget has been realized
		 */
		setsinglelogin (ActionMenu);
/* Formatting is now dynamic 
		XtVaSetValues(TopLevel,
			XtNminHeight, (XtArgVal) 325,
			XtNminWidth, (XtArgVal) 600,
			0);
*/
	}
	else
		/* Build scroll list failed.  We want the user
		 * to exit out after the error message has been
		 * displayed.  Setting this global boolean to true
	 	 * informs the error message callback to terminate
		 * the program.
		 */
		endApplication = True;

    	XtAppMainLoop (AppContext);

} /* End of main () */


/********************************************************************* 
 *	set single login status for the user in the actions pull
 * 	down menu. This takes the form of an action that the user
 * 	can perform to change the state of single login for himself. 
 *********************************************************************/
static void 
setsinglelogin (Widget widget)
{ 
	struct stat	buf;
	char		*mnem, *filename;

	/* Get the filename for single login file for the user
	 */
	filename = getsingleloginfile();
#ifdef DEBUG
	printf ("filename %s\n", filename);
#endif

	/* Set label to the button depending on the status of single
	 * login for the user.  Set userdata to appropriate action.
	 * i.e if the $HOME/.slogin file exists - single login DISABLE
	 * if the $HOME/.slogin file does not exist - single login ENABLE
	 */
	if ((stat (filename, &buf)) < 0) {
		mnem = (char *) GetStr (MNEM_singleloginenabled);
		OlVaFlatSetValues (widget, SingleLogin_Button,
				XtNlabel,  (XtArgVal) GetStr 
						(TXT_singleloginenabled),
				XtNmnemonic,  	(XtArgVal) mnem[0],
				XtNuserData, (	XtArgVal) DISABLE,
				0);
	}
	else {
		mnem = (char *) GetStr (MNEM_singlelogindisabled);
		OlVaFlatSetValues (widget, SingleLogin_Button,
				XtNlabel,  (XtArgVal) GetStr 
						(TXT_singlelogindisabled),
				XtNmnemonic,  	(XtArgVal) mnem[0],
				XtNuserData, 	(XtArgVal) ENABLE,
				0);
	}

	/* If single login is turned off system wide i.e. if /etc/.slogin
	 * file does not exist then de-sensitize the button. User cannot
	 * do anything about it. Sorry!
	 */
	if ((stat (SYSTEM_SINGLELOGIN, &buf)) < 0) {
		OlVaFlatSetValues (widget, SingleLogin_Button,
					XtNsensitive,  (XtArgVal) False,
					0);
	}
	XtFree (filename);
}

/********************************************************************* 
 * get the file name for single login  for the user.
 *********************************************************************/
static char *getsingleloginfile ()
{
	char		*filename;
	FILE		*fp;
	struct passwd 	*pwd;
	uid_t		uid;

	uid = getuid();
    	pwd = getpwuid (uid);

	/* get the name of the single login file in the home directory
	 * of the user.  If it is root then do not append another slash.
	 */
	filename = XtMalloc (strlen (pwd->pw_dir) + strlen (USER_SINGLELOGIN) 
				+ strlen (SLASH) + 3);
	strcpy (filename, pwd->pw_dir);
	if (strlen(pwd->pw_dir) > 1)
		strcat (filename, SLASH);
	strcat (filename, USER_SINGLELOGIN);

	return filename;
}

/********************************************************************* 
 * SingleLoginCB.  If the action is to change status do that here.
 *********************************************************************/
static void
SingleLoginCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
	char		*filename, *mnem;
	int		userdata;
	Boolean		sensitive;
	FILE		*fp;

	/* If the widget is insensitive then just go back
	 * user cannot do much here
	 */
	OlVaFlatGetValues (widget, SingleLogin_Button, 
				XtNsensitive, &sensitive, 0);
	if (!sensitive)
		return;

	/* get the name of the single login file in the home directory
	 */
	filename = getsingleloginfile (); 
#ifdef DEBUG
	printf ("filename %s\n", filename);
#endif

	/* get the action associated with the widget 
	 */
	OlVaFlatGetValues (widget, SingleLogin_Button, 
					XtNuserData, &userdata, 0);

	/* Depending on what the action was, set the label string on
	 * the menu button to 'Enable or Disable Single Login' and 
	 * set the userdata for the same.  If it was ENABLE then remove
	 * .slogin else create it.
	 */
	if (userdata == DISABLE) {
#ifdef DEBUG
		printf ("creating %s\n", filename);
#endif
		if ((fp = fopen(filename, "w")) == NULL) {
			GUIError (widget, GetStr (TXT_slopenfail));
			return;
		}	
		fclose (fp);
		mnem = (char *) GetStr (MNEM_singlelogindisabled);
		OlVaFlatSetValues(widget, SingleLogin_Button,
				XtNlabel,	(XtArgVal) GetStr
						(TXT_singlelogindisabled),
				XtNmnemonic, 	(XtArgVal) mnem[0], 
				XtNuserData,	(XtArgVal) ENABLE,
				0);
	}
	else if (userdata == ENABLE) {
#ifdef DEBUG
		printf ("removing %s\n", filename);
#endif
		if ((unlink (filename)) < 0) {
			GUIError (widget, GetStr (TXT_slremovefail));
			return;
		}
		mnem = (char *) GetStr (MNEM_singleloginenabled);
		OlVaFlatSetValues(widget, SingleLogin_Button,
				XtNlabel, 	(XtArgVal) GetStr
						(TXT_singleloginenabled), 
				XtNmnemonic, 	(XtArgVal) mnem[0], 
				XtNuserData,	(XtArgVal) DISABLE,
				0);
	}

	XtFree (filename);
}	/* End of setsinglelogin () */

/********************************************************************* 
 *
 * ExitCB
 * exiting here 
 *********************************************************************/
static void
ExitCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    exit (0);
}	/* End of ExitCB () */

/********************************************************************* 
 * HelpCB
 * Display help.  userData in the item is a pointer to the HelpText data.
 *********************************************************************/
void
HelpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    MenuItem		*selected;

    selected = (MenuItem *) flatData->items + flatData->item_index;
    DisplayHelp (widget, (HelpText *) selected->userData);
}	/* End of HelpCB () */

/**********************************************************************
 * DisplayHelp
 * Send a message to dtm to display a help window.  If help is NULL, then
 * ask dtm to display the help desk.
***********************************************************************/
void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay (widget);
    Window			win = XtWindow (XtParent (XtParent (widget)));

    memset (&displayHelpReq, 0, sizeof (displayHelpReq));
    req = (DtRequest *) &displayHelpReq;
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
    displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = AppTitle;
	displayHelpReq.app_title = AppTitle;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = GetStr (HELP_FILE);
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */

/**********************************************************************
		Make the icon for the app
**********************************************************************/
static void
MakeIcon(Widget toplevel)
{
	Pixmap		icon, iconmask;
	DmGlyphPtr	glyph;
	Cardinal	PixmapDepth;
	Colormap	PixmapColormap;

	glyph = DmGetPixmap (XtScreen (toplevel), "xnetware.icon");
	if (glyph) {
		icon = glyph->pix;
		iconmask = glyph->mask;
	} else
		icon = iconmask = (Pixmap) 0;

	XtVaSetValues(toplevel,
		XtNiconPixmap, (XtArgVal) icon,
		XtNiconMask, (XtArgVal) iconmask,
		XtNiconName, (XtArgVal) GetStr(TXT_appName),
		0
	);
}
