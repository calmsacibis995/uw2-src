/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xman:buttons.c	1.2"

/*
 *	Copyright (c) 1991, 1992 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

/*
 * xman - X window system manual page display program.
 *
 * $XConsortium: buttons.c,v 1.32 91/09/03 17:41:59 dave Exp $
 *
 * Copyright 1987, 1988 Massachusetts Institute of Technology
 *
 *
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   October 27, 1987
 */

#include "globals.h"

/* The files with the icon bits in them. */

#include "icon_open.h"
#include "icon_help.h"
#include "iconclosed.h"

static void CreateOptionMenu(), CreateSectionMenu();
void StartManpage();
static Widget * ConvertNamesToWidgets();
ManpageGlobals * InitPsuedoGlobals();

/*	Function Name: MakeTopBox
 *	Description: This funtion creates the top menu, in a shell widget.
 *	Arguments: none.
 *	Returns: the top level widget
 */

#define TOPARGS 5

static Atom wm_delete_window;

Widget top;			/* needed in PopupWarning, misc.c */

void
MakeTopBox()
{
  Widget form, command, label; /* widgets. */
  Arg arglist[TOPARGS];		/* An argument list */
  Cardinal num_args = 0;	/* The number of arguments. */
  ManpageGlobals * man_globals;
  static char * full_size[] = {
    "topLabel", MANPAGE_BUTTON, NULL
  };
  static char * half_size[] = {
    HELP_BUTTON, QUIT_BUTTON, NULL
  };
  
/* create the top icon. */

  num_args = 0;
  XtSetArg(arglist[num_args], XtNiconPixmap,
	   XCreateBitmapFromData( XtDisplay(initial_widget), 
				 XtScreen(initial_widget)->root,
				 (char *)iconclosed_bits, iconclosed_width,
				 iconclosed_height));
  num_args++;
  XtSetArg(arglist[num_args], XtNtitle, resources.title);
  num_args++;
  XtSetArg(arglist[num_args], XtNiconic, resources.iconic);
  num_args++;
  top = XtCreatePopupShell(TOPBOXNAME, topLevelShellWidgetClass, 
			   initial_widget, arglist, num_args);

  form = XtCreateManagedWidget("form", formWidgetClass, top, 
			       NULL, (Cardinal) 0);

  label = XtCreateManagedWidget("topLabel", labelWidgetClass, form, 
			       NULL, (Cardinal) 0);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNfromVert, label); num_args++;
  command = XtCreateManagedWidget(HELP_BUTTON, commandWidgetClass, form, 
				  arglist, num_args);

  /* use same vertical as help widget. */
  XtSetArg(arglist[num_args], XtNfromHoriz, command); num_args++;
  command = XtCreateManagedWidget(QUIT_BUTTON, commandWidgetClass, form, 
				  arglist, num_args);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNfromVert, command); num_args++;
  command = XtCreateManagedWidget(MANPAGE_BUTTON, commandWidgetClass, form, 
				  arglist, num_args);

  help_widget = NULL;		/* We have not seen the help yet. */

  FormUpWidgets(form, full_size, half_size);

/*
 *  The widget should be mapped only by calling XtmapWidget explicitly, not by 
 *  XtRealizeWidget.
 */
  XtSetMappedWhenManaged(top,False);

  XtRealizeWidget(top);

  XSetCommand(XtDisplay(top), XtWindow(top), saved_argv, saved_argc);

  man_globals = (ManpageGlobals*) XtMalloc( (Cardinal) sizeof(ManpageGlobals));
  man_globals->label = NULL;
  man_globals->manpagewidgets.directory = NULL;
  man_globals->manpagewidgets.manpage = NULL;
  man_globals->manpagewidgets.box = NULL;
  man_globals->current_directory = 0;
  MakeSearchWidget(man_globals, top);
  MakeSaveWidgets(man_globals, top);

  SaveGlobals( (man_globals->This_Manpage = top), man_globals);

/*
 *  Map it conditionally.
 */
  if (resources.top_box_active) 
  	XtMapWidget(top);
  else
	if (!man_file)   /**  If manpage is not given at the command line.  **/
    	(void) CreateManpage(NULL);

  AddCursor(top, resources.cursors.top);

/*
 * Set up ICCCM delete window.
 */
  wm_delete_window = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW",
				 False);
  XtOverrideTranslations
      (top, XtParseTranslationTable ("<Message>WM_PROTOCOLS: Quit()"));
  (void) XSetWMProtocols (XtDisplay(top), XtWindow(top),
			  &wm_delete_window, 1);
  

}

/*	Function Name: CreateManpage
 *	Description: Creates a new manpage.
 *	Arguments: none.
 *	Returns: none.
 */

Widget
CreateManpage( file )
FILE * file;
{
  ManpageGlobals * man_globals;	/* The psuedo global structure. */

  man_globals = InitPsuedoGlobals();
  CreateManpageWidget(man_globals, MANNAME, TRUE);
  
  
  if (file == NULL)
    StartManpage( man_globals, OpenHelpfile(man_globals), FALSE );
  else {
    OpenFile(man_globals, file);
    StartManpage( man_globals, FALSE, TRUE);
  }
  return(man_globals->This_Manpage);
}

/*	Function Name: InitPsuedoGlobals
 *	Description: Initializes the psuedo global variables.
 *	Arguments: none.
 *	Returns: a pointer to a new pseudo globals structure.
 */

ManpageGlobals * 
InitPsuedoGlobals()
{
  ManpageGlobals * man_globals;

  /*
   * Allocate necessary memory. 
   */

  man_globals = (ManpageGlobals *) 
                XtMalloc( (Cardinal) sizeof(ManpageGlobals));

  man_globals->section_name = (char **) XtMalloc( (Cardinal) (sections *
							      sizeof(char *)));
  man_globals->manpagewidgets.box = (Widget *) XtCalloc( (Cardinal) sections,
						    (Cardinal) sizeof(Widget));
  
  /* Initialize the number of screens that will be shown */

  man_globals->both_shown = resources.both_shown_initial;
  
  return(man_globals);
}
  
/*	Function Name: CreateManpageWidget
 *	Description: Creates a new manual page widget.
 *	Arguments: man_globals - a new man_globals structure.
 *                 name         - name of this shell widget instance.
 *                 full_instance - if true then create a full manpage,
 *                                 otherwise create stripped down version
 *                                 used for help.
 *	Returns: none
 */

#define MANPAGEARGS 10

void
CreateManpageWidget(man_globals, name, full_instance)
ManpageGlobals * man_globals;
char * name;
Boolean full_instance;
{
  Arg arglist[MANPAGEARGS];	/* An argument list for widget creation */
  Cardinal num_args;		/* The number of arguments in the list. */
  Widget top, pane, hpane, sections;	/* Widgets */
  ManPageWidgets * mpw = &(man_globals->manpagewidgets);

  num_args = (Cardinal) 0;
  XtSetArg(arglist[num_args], XtNwidth, default_width);
  num_args++; 
  XtSetArg(arglist[num_args], XtNheight, default_height);
  num_args++; 

  top = XtCreatePopupShell(name, topLevelShellWidgetClass, initial_widget,
			   arglist, num_args);

  man_globals->This_Manpage = top; /* pointer to root widget of Manualpage. */

  num_args = 0;
  if (full_instance)
    XtSetArg(arglist[num_args], XtNiconPixmap,
	     XCreateBitmapFromData( XtDisplay(top), XtScreen(top)->root,
				   (char *)icon_open_bits, icon_open_width,
				   icon_open_height));
  else 
    XtSetArg(arglist[num_args], XtNiconPixmap,
	     XCreateBitmapFromData( XtDisplay(top), XtScreen(top)->root,
				   (char *)icon_help_bits, icon_help_width,
				   icon_help_height));
  num_args++;
  XtSetValues(top, arglist, num_args);


  pane = XtCreateManagedWidget("Manpage_Vpane", panedWidgetClass, top, NULL, 
			       (Cardinal) 0);

/* Create menu bar. */

  hpane = XtCreateManagedWidget("horizPane", panedWidgetClass,
				  pane, NULL, (Cardinal) 0);
  num_args = 0;
  XtSetArg(arglist[num_args], XtNmenuName, OPTION_MENU); num_args++;
  (void) XtCreateManagedWidget("options", menuButtonWidgetClass,
				  hpane, arglist, num_args);

  CreateOptionMenu(man_globals, top);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNmenuName, SECTION_MENU); num_args++;
  sections = XtCreateManagedWidget("sections", menuButtonWidgetClass,
				   hpane, arglist, num_args);

  XtSetArg(arglist[0], XtNlabel, SHOW_BOTH);
  XtSetValues(man_globals->both_screens_entry, arglist, (Cardinal) 1);

  if (full_instance) {
    MakeSearchWidget(man_globals, top);
    CreateSectionMenu(man_globals, top);
    MakeSaveWidgets(man_globals, top);
  } else {
    XtSetSensitive(sections, FALSE);       
    XtSetArg(arglist[0], XtNsensitive, FALSE);
    XtSetValues(man_globals->dir_entry, arglist, ONE);
    XtSetValues(man_globals->manpage_entry, arglist, ONE);
    XtSetValues(man_globals->help_entry, arglist, ONE);
    XtSetValues(man_globals->search_entry, arglist, ONE);
    XtSetValues(man_globals->both_screens_entry, arglist, ONE);
  }

  man_globals->label = XtCreateManagedWidget(MANNAME, labelWidgetClass,
					     hpane, NULL, (Cardinal) 0);

/* Create Directory */

  if (full_instance) {
    num_args = 0;
    XtSetArg(arglist[num_args], XtNallowVert, TRUE);
    num_args++;
    
    mpw->directory = XtCreateWidget(DIRECTORY_NAME, viewportWidgetClass,
				    pane, arglist, num_args);
    
    man_globals->current_directory = INITIAL_DIR;
    MakeDirectoryBox(man_globals, mpw->directory,
		     mpw->box + man_globals->current_directory, 
		     man_globals->current_directory );
    XtManageChild(mpw->box[man_globals->current_directory]);
  }

/* Create Manpage */

  mpw->manpage = XtCreateWidget(MANUALPAGE, scrollByLineWidgetClass,
				pane, NULL, (Cardinal) 0);

}

/*	Function Name: StartManpage
 *	Description: Starts up a new manpage.
 *	Arguments: man_globals - the psuedo globals variable.
 *                 help - is this a help file?
 *                 page - Is there a page to display?
 *	Returns: none.
 */

void
StartManpage(man_globals, help, page)
ManpageGlobals * man_globals;
Boolean help, page;
{
  Widget dir = man_globals->manpagewidgets.directory;
  Widget manpage = man_globals->manpagewidgets.manpage;
  Widget label = man_globals->label;
  Arg arglist[1];

/* 
 * If there is a helpfile then put up both screens if both_show is set.
 */

  if (page || help) {
    if (help) 
      strcpy(man_globals->manpage_title, "Xman Help");

    if (man_globals->both_shown) {
      XtManageChild(dir);
      man_globals->dir_shown = TRUE;

      XtSetArg(arglist[0], XtNpreferredPaneSize, resources.directory_height);
      XtSetValues(dir, arglist, (Cardinal) 1);

      XtSetArg(arglist[0], XtNsensitive, FALSE);
      XtSetValues(man_globals->manpage_entry, arglist, ONE);
      XtSetValues(man_globals->dir_entry, arglist, ONE);

      XtSetArg(arglist[0], XtNlabel, SHOW_ONE);
      XtSetValues(man_globals->both_screens_entry, arglist, ONE);
      ChangeLabel(label,
		  man_globals->section_name[man_globals->current_directory]);
    }
    else {
      ChangeLabel(label,man_globals->manpage_title);
    }
    XtManageChild(manpage);
    man_globals->dir_shown = FALSE;
  }
/*
 * Since There is file to display, put up directory and do not allow change
 * to manpage, show both, or help.
 */
  else {			
    XtManageChild(dir);
    man_globals->dir_shown = TRUE;
    XtSetArg(arglist[0], XtNsensitive, FALSE);
    XtSetValues(man_globals->manpage_entry,        arglist, ONE);
    XtSetValues(man_globals->help_entry,           arglist, ONE);
    XtSetValues(man_globals->both_screens_entry,   arglist, ONE);
    man_globals->both_shown = FALSE;
    ChangeLabel(label,
		man_globals->section_name[man_globals->current_directory]);
  }

/*
 * Start 'er up, and change the cursor.
 */

  XtRealizeWidget( man_globals->This_Manpage );

/*
 *	add WM_COMMAND property to all windows :- to allow xlsclients and
 *	others (session managers etc) to recognise the client.
 */
#ifdef	DELETE
	Get Current Man page name (of the man page being shown)
	New Option : <page> name can be specified like 'man' syntax.
	xman [options]... [<manpagename>]
	so that the session manager can restart the client with the same
	man page.
	So WM_COMMAND would contain :
	"xman" "-..." "<pagename>"
	Multiple page arguments should cause multiple windows to startup.
	The WM_COMMAND property will need to be updated every time we
	change the man page using CTRL-S also.

  XSetCommand(XtDisplay(man_globals->This_Manpage), XtWindow(man_globals->This_Manpage), saved_argv, saved_argc);
#endif	/* DELETE */

  SaveGlobals( man_globals->This_Manpage, man_globals);
  XtMapWidget( man_globals->This_Manpage );
  AddCursor( man_globals->This_Manpage, resources.cursors.manpage);
  XtSetArg(arglist[0], XtNtransientFor, man_globals->This_Manpage);
  XtSetValues(XtParent(man_globals->standby), arglist, (Cardinal)1);
  XtSetValues(XtParent(man_globals->save), arglist, (Cardinal) 1);
  XtRealizeWidget(XtParent(man_globals->standby));
  XtRealizeWidget(XtParent(man_globals->save));
  AddCursor( XtParent(man_globals->standby), resources.cursors.top);
#ifdef	DELETE
	/*
	 *	Wipro : vivek t.
	 *	Do we really need this top cursor for the save widget or can we
	 *	have the manpage cursor itself ?
	 */
  AddCursor( man_globals->save, resources.cursors.top);
#endif

/*
 * Set up ICCCM delete window.
 */
  XtOverrideTranslations
      (man_globals->This_Manpage, 
       XtParseTranslationTable ("<Message>WM_PROTOCOLS: RemoveThisManpage()"));
  (void) XSetWMProtocols (XtDisplay(man_globals->This_Manpage),
			  XtWindow(man_globals->This_Manpage),
			  &wm_delete_window, 1);

}

/*      Function Name: MenuDestroy
 *      Description: free's data associated with menu when it is destroyed.
 *      Arguments: w - menu widget.
 *                 free_me - data to free.
 *                 junk - not used.
 *      Returns: none.
 */

/* ARGSUSED */
static void
MenuDestroy(w, free_me, junk)
Widget w;
XtPointer free_me, junk;
{
  XtFree( (char *) free_me);
}

/*      Function Name:   CreateOptionMenu
 *      Description: Create the option menu.
 *      Arguments: man_globals - the manual page globals.
 *                 parent - the button that activates the menu.
 *      Returns: none.
 */

static void
CreateOptionMenu(man_globals, parent)
ManpageGlobals * man_globals;
Widget parent;
{
  Widget menu, entry;
  int i;
  static char * option_names[] = {	/* Names of the buttons. */
    DIRECTORY,
    MANPAGE,
    HELP,
    SEARCH,
    BOTH_SCREENS, 
    REMOVE_MANPAGE,
    OPEN_MANPAGE,
    SHOW_VERSION,
    QUIT
  };

  menu = XtCreatePopupShell(OPTION_MENU, simpleMenuWidgetClass, parent,
			    NULL, (Cardinal) 0);
  man_globals->option_menu = menu;
  
  for (i = 0 ; i < NUM_OPTIONS ; i++) {
    entry = XtCreateManagedWidget(option_names[i], smeBSBObjectClass,
				  menu, NULL, ZERO);
    XtAddCallback(entry, XtNcallback, OptionCallback, (caddr_t) man_globals);
    switch (i) {
    case 0:
	man_globals->dir_entry = entry;
	break;
    case 1:
	man_globals->manpage_entry = entry;
	break;
    case 2:
	man_globals->help_entry = entry;
	break;
    case 3:
	man_globals->search_entry = entry;
	break;
    case 4:
	man_globals->both_screens_entry = entry;
	break;
    case 5:
	man_globals->remove_entry = entry;
	break;
    case 6:
	man_globals->open_entry = entry;
	break;
    case 7:
	man_globals->version_entry = entry;
	break;
    case 8:
	man_globals->quit_entry = entry;
	break;
    default:
	break;
    }
  }
}

/*      Function Name: CreateSectionMenu
 *      Description: Create the Section menu.
 *      Arguments: man_globals - the manual page globals.
 *                 parent - the button that activates the menu.
 *      Returns: none.
 */

static void
CreateSectionMenu(man_globals, parent)
ManpageGlobals * man_globals;
Widget parent;
{
  Widget menu, entry;
  int i;
  MenuStruct * menu_struct;
  Arg args[1];
  Cardinal num_args;
  char entry_name[BUFSIZ];

  menu = XtCreatePopupShell(SECTION_MENU, simpleMenuWidgetClass, parent,
			    NULL, (Cardinal) 0);

  for (i = 0 ; i < sections ; i ++) {
    num_args = 0;
    XtSetArg(args[num_args], XtNlabel, manual[i].blabel); num_args++;
    sprintf(entry_name, "section%d", i);
      
    entry = XtCreateManagedWidget(entry_name, smeBSBObjectClass,
				  menu, args, num_args);
    menu_struct = (MenuStruct *) XtMalloc(sizeof(MenuStruct));
    menu_struct->data = (caddr_t) man_globals;
    menu_struct->number = i;
    XtAddCallback(entry, XtNcallback, DirPopupCallback, (caddr_t) menu_struct);
    XtAddCallback(entry, XtNdestroyCallback,MenuDestroy, (caddr_t)menu_struct);

  }
}

/*	Function Name: CreateList
 *	Description: this function prints a label in the directory list
 *	Arguments: section - the manual section.
 *	Returns: none
 */

static char **
CreateList(section)
{
  char ** ret_list, **current;
  int count;

  ret_list = (char **) XtMalloc( (manual[section].nentries + 1) * 
				   sizeof (char *));

  for (current = ret_list, count = 0 ; count < manual[section].nentries ;
       count++, current++)
    *current = CreateManpageName(manual[section].entries[count], section,
				 manual[section].flags);
 
  *current = NULL;		/* NULL terminate the list. */
  return(ret_list);
}

/*	Function Name: MakeDirectoryBox
 *	Description: make a directory box.
 *	Arguments: man_globals - the psuedo global structure for each manpage.
 *                 parent - this guys parent widget.
 *                 dir_disp - the directory display widget.
 *                 section - the section number.
 *	Returns: none.
 */

void
MakeDirectoryBox(man_globals,parent,dir_disp,section)
ManpageGlobals *man_globals;
Widget parent, *dir_disp;
int section;
{
  Arg arglist[10];
  Cardinal num_args;
  char * name, label_name[BUFSIZ];

  if (*dir_disp != NULL)	/* If we have one, don't make another. */
    return;

  name = manual[section].blabel;   /* Set the section name */
  sprintf(label_name,"Directory of: %s",name);
  man_globals->section_name[section] = StrAlloc(label_name);

  num_args = 0;
  XtSetArg(arglist[num_args], XtNlist, CreateList(section));
  num_args++;
  XtSetArg(arglist[num_args], XtNfont, resources.fonts.directory);
  num_args++;
  
  *dir_disp = XtCreateWidget(DIRECTORY_NAME, listWidgetClass, parent,
			     arglist, num_args);
  
  XtAddCallback(*dir_disp, XtNcallback,
		DirectoryHandler, (caddr_t) man_globals);
}

/*	Function Name: MakeSaveWidgets.
 *	Description: This functions creates two popup widgets, the please 
 *                   standby widget and the would you like to save widget.
 *	Arguments: man_globals - the psuedo globals structure for each man page
 *                 parent - the realized parent for both popups.
 *	Returns: none.
 */

void
MakeSaveWidgets(man_globals, parent)
ManpageGlobals *man_globals;
Widget parent;
{
  Widget shell, dialog; /* misc. widgets. */
  Arg warg[1];
  Cardinal n = 0;

/* make the please stand by popup widget. */
  if (XtIsRealized(parent)) {
      XtSetArg(warg[0], XtNtransientFor, parent);	n++;
  }
  shell = XtCreatePopupShell( "pleaseStandBy", transientShellWidgetClass,
			      parent, warg, (Cardinal) n);

  man_globals->standby = XtCreateManagedWidget("label", labelWidgetClass, 
					       shell, NULL, (Cardinal) 0);

  man_globals->save = XtCreatePopupShell("likeToSave",
					 transientShellWidgetClass,
					 parent, warg, n);

  dialog = XtCreateManagedWidget("dialog", dialogWidgetClass, 
				 man_globals->save, NULL, (Cardinal) 0);

  XawDialogAddButton(dialog, FILE_SAVE, NULL, NULL);
  XawDialogAddButton(dialog, CANCEL_FILE_SAVE, NULL, NULL);

  if (XtIsRealized(parent)) {
      XtRealizeWidget(shell);
      AddCursor(shell,resources.cursors.top);
      XtRealizeWidget(man_globals->save);
      AddCursor(man_globals->save, resources.cursors.top);
  }
}

/*      Function Name: FormUpWidgets
 *      Description: Sizes widgets to look nice.
 *      Arguments: parent - the common parent of all the widgets.
 *                 full_size - array of widget names that will he full size.
 *                 half_size - array of widget names that will he half size.
 *      Returns: none
 */

void
FormUpWidgets(parent, full_size, half_size)
Widget parent;
char ** full_size, ** half_size;
{
  Widget * full_widgets, * half_widgets, *temp, long_widget;
  Dimension longest, length, b_width;
  int interior_dist;
  Arg arglist[2];
    
  full_widgets = ConvertNamesToWidgets(parent, full_size);
  half_widgets = ConvertNamesToWidgets(parent, half_size);
  
  long_widget = NULL;
  longest = 0;
  XtSetArg(arglist[0], XtNwidth, &length);
  XtSetArg(arglist[1], XtNborderWidth, &b_width);

/*
 * Find Longest widget.
 */

  for ( temp = full_widgets ; *temp != (Widget) NULL ; temp++) {
    XtGetValues(*temp, arglist, (Cardinal) 2);
    length += 2 * b_width;
    if (length > longest) {
      longest = length;
      long_widget = *temp;
    }
  }

  if (long_widget == (Widget) NULL) {          /* Make sure we found one. */
    PopupWarning(GetGlobals(parent), 
		 "Could not find longest widget, aborting...");
    XtFree((char *)full_widgets);
    XtFree((char *)half_widgets);
    return;
  }

/*
 * Set all other full_widgets to this length.
 */

  for ( temp = full_widgets ; *temp != (Widget) NULL ; temp++ )
    if ( long_widget != *temp) {
      Dimension width, border_width;

      XtSetArg(arglist[0], XtNborderWidth, &border_width);
      XtGetValues(*temp, arglist, (Cardinal) 1);
    
      width = longest - 2 * border_width;
      XtSetArg(arglist[0], XtNwidth, width);
      XtSetValues(*temp, arglist, (Cardinal) 1);
    }

/*
 * Set all the half widgets to the right length.
 */

  XtSetArg(arglist[0], XtNdefaultDistance, &interior_dist);
  XtGetValues(parent, arglist, (Cardinal) 1);
  
  for ( temp = half_widgets ; *temp != (Widget) NULL ; temp++) {
    Dimension width, border_width;

    XtSetArg(arglist[0], XtNborderWidth, &border_width);
    XtGetValues(*temp, arglist, (Cardinal) 1);
    
    width = (int)(longest - interior_dist)/2 - 2 * border_width;
    XtSetArg(arglist[0], XtNwidth, width);
    XtSetValues(*temp, arglist, (Cardinal) 1);
  }

  XtFree((char *)full_widgets);
  XtFree((char *)half_widgets);
}
  
/*      Function Name: ConvertNamesToWidgets
 *      Description: Convers a list of names into a list of widgets.
 *      Arguments: parent - the common parent of these widgets.
 *                 names - an array of widget names.
 *      Returns: an array of widget id's.
 */

static Widget *
ConvertNamesToWidgets(parent, names)
Widget parent;
char ** names;
{
  char ** temp;
  Widget * ids, * temp_ids;
  int count;

  for (count = 0, temp = names; *temp != NULL ; count++, temp++);

  ids = (Widget *) XtMalloc( (count + 1) * sizeof(Widget));

  
  for ( temp_ids = ids; *names != NULL ; names++, temp_ids++) {
    *temp_ids = XtNameToWidget(parent, *names);
    if (*temp_ids == NULL) {
      char error_buf[BUFSIZ];
    
      sprintf(error_buf, "Could not find widget named '%s'", *names);
      PrintError(error_buf);
      XtFree((char *)ids);
      return(NULL);
    }
  }
  
  *temp_ids = (Widget) NULL;
  return(ids);
}
    
