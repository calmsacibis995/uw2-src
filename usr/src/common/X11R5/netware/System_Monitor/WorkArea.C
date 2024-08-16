#ident	"@(#)systemmon:WorkArea.C	1.7"
////////////////////////////////////////////////////////////////////
// WorkArea.C: A base area to hold all the gui components
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "WorkArea.h"
#include "IconBox.h"
#include "Graph.h"
#include "SarList.h"
#include "Question.h"
#include "Options.h"
#include "Help.h"
#include "i18n.h"
#include "ErrDialog.h"
#include <Xm/PanedW.h>
#include <Xm/Form.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

/****************************************************************
		 GLOBAL DEFINES
******************************************************************/
#define		TIMER_DEFAULT		5000
#define		COLON			":"
#define 	LOG_FILE		"sys_mon.log" 
#define 	ALARM_FILE		"sysmon_alarm.log" 
#define 	appHelpSect		"10"

/* The following declarations pertain to the iconify/de-iconify
 * actions on the top  level window.  When this happens the new
 * icons need to be installed in the iconified window of the program
 * These values are declared global because the Xt Call to add these
 * actions does not acccept any application data as parameters. So,
 * there is no way to create these as variables of the WorkArea class. 
 */
static void Iconified (Widget,  XEvent *, String *, Cardinal *);
static void Deiconified (Widget,  XEvent *, String *, Cardinal *);

Boolean 	iconified;
void 		InstallNewPixmap (Widget, Pixmap);
Pixmap 		oldpixmap, newpixmap;

static XtActionsRec actions[] = {
    {"Iconified",    Iconified},
    {"Deiconified",  Deiconified },
};

static char 		*sys_mon_icon = "/usr/X/lib/pixmaps/sysmon48.icon";
static char 		*sys_monalt_icon ="/usr/X/lib/pixmaps/sysmon48alt.icon";
#define 	DTMSG	"/usr/X/desktop/rft/dtmsg "

static SarDetail list_struct [] = {
	{ TXT_usr,  TXT_100 },
	{ TXT_sys,  TXT_100 },
	{ TXT_wio,  TXT_100 },
	{ TXT_idle, TXT_100 },
	{ TXT_freemem,   TXT_10000 },
	{ TXT_freeswap,  TXT_10000 },
	{ TXT_pgin,  TXT_1000 },
	{ TXT_pgout,  TXT_1000 },
	{ TXT_swapin,  TXT_1000 },
	{ TXT_swapout,  TXT_1000 },
	{ TXT_bswapin,  TXT_1000 },
	{ TXT_bswapout,  TXT_1000 },
	{ TXT_pswch,  TXT_1000 },
};

extern "C" int open_mets();
extern "C" int snap_mets();
extern "C" int no_cpu();
extern "C" void close_mets();
extern "C" int XReadPixmapFile (Display *,Drawable, Colormap, char *,
		unsigned int *, unsigned int *, unsigned int, Pixmap *, long);

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
//HelpText AppHelp = { HELP_FILE, TXT_appHelp, TXT_appHelpSect, TXT_helptitle,};
HelpText AppHelp = { HELP_FILE, TXT_appHelp, appHelpSect, TXT_helptitle,};

/*****************************************************************
	CTOR - WorkArea:
	Set up the application window.  Set up all the variables
	that are going to be used in the application.  Make sure
	that the settings are used if they had been saved earlier.
	Set up the sar table with all the values. Create the name
	of the logfile and the save settings file to be used later.
*****************************************************************/
WorkArea::WorkArea ( Widget parent, char *name) : BasicComponent (name)
{
	struct passwd	*pwd;
	uid_t		uid;

	/* open the kernel metrics database and get the # of cpus
	 * that the machine has, and store it in _cpus. Set default
	 * to global or 0 for single processor.
	 */
	if ((open_mets()) < 0)   {
		char		buf[BUFSIZ];

		sprintf (buf, "%s %s%s%s", DTMSG, "\"", 
			I18n::GetStr (TXT_nometrics), "\"");
		system (buf);
		exit (1);
	}
	snap_mets();
	_which_cpu = 0;
	_cpus = no_cpu();

	/*	set the boolean variables */ 
	_log = False;
    	iconified = False;

	/* get the home directory and append sys_mon.log to 
	 * create the log file name to store the log data
	 * and create the savesettings file(.sys_mon) and the alarm file
	 */
	setpwent ();
	uid = geteuid ();	
	while (pwd = getpwent()) {
		/* get the name of the home directory */
		if (uid	== pwd->pw_uid)  {
			_homedir = new char[strlen (pwd->pw_dir) + 1];
			strcpy (_homedir, pwd->pw_dir);
			_logfilename = new char[strlen(pwd->pw_dir) + 
						strlen(LOG_FILE) + 2];
			_savefilename = new char[strlen (pwd->pw_dir) + 
						strlen(SAVE_FILE) + 2];
			_alarmfilename = new char[strlen (pwd->pw_dir) + 
						strlen(ALARM_FILE) + 2];
			strcpy (_logfilename, pwd->pw_dir);
			strcpy (_savefilename, pwd->pw_dir);
			strcpy (_alarmfilename, pwd->pw_dir);
			if (strncmp (SLASH, pwd->pw_dir, strlen(pwd->pw_dir)) 
				!= 0){ 
				strcat (_logfilename, SLASH);
				strcat (_savefilename, SLASH);
				strcat (_alarmfilename, SLASH);
			}
			strcat (_logfilename, LOG_FILE);
			strcat (_savefilename, SAVE_FILE);
			strcat (_alarmfilename, ALARM_FILE);
			break;
		}
     	} 
	endpwent ();

	/* Create the sar table here - either from the saved settings
	 * file or else with default values.
	 */
	SetSarTable (parent);

	/*	Create the Paned window.	
	 */
	_w = XmCreatePanedWindow (parent, name, NULL, 0);
		
	/* create the menu bar area - all menus with items in them
	 */
	_menubar = new MenuBar (_w, "MenuBar", this);
	_menubar->manage();

	/* create the icon area - all pushbuttons 
	 * with icons on them
	 */
	_iconbox = new IconBox (_w, "IconBox", this);
	_iconbox->manage();

    	 /* Create the graph 
	  */
    	_graph = new Graph (_w, this, "Graph");

	/* this is the sar list area with the 3 headers
	 * - list, scale and colors 
	 */
	_list = new SarList (_w, this, "SarList", _graph); 
	_list->manage ();

	/* Set the Processor label to the cpu # that was set in the
	 * beginning which is 0
	 */
	SetProcLabel();

	XtAddCallback (_w, XmNhelpCallback, &WorkArea::HelpCB, this);

	/*
	 * Create the new pixmap and the old pixmap here 
	 * Install the old pixmap i.e the original one.
	 */
	CreatePixmap  (&oldpixmap, sys_mon_icon, theApplication->Background());
	CreatePixmap  (&newpixmap, sys_monalt_icon, 
			theApplication->Background());
	InstallNewPixmap (parent, oldpixmap);

	/*
	 * Override some translations
	 */
    	XtAppAddActions (theApplication->appContext(), actions, 
			XtNumber(actions));
    	XtOverrideTranslations(parent, XtParseTranslationTable
				("<UnmapNotify>: Iconified()"));
    	XtOverrideTranslations(parent, XtParseTranslationTable
				("<MapNotify>: Deiconified()"));

	/* Sensitize or de-sensitize alarm button depending on
	 * whether sar options are to be displayed or not
	 */
	SetAlarmButton();
}

/*****************************************************************
	dtor for WorkArea - delete all classes that were newed.
*****************************************************************/
WorkArea::~WorkArea()
{
	delete _menubar;
	delete _list;
	delete _iconbox;
	delete _graph;
	delete _logfilename;
	delete _savefilename;
	delete _alarmfilename;
	delete _homedir;
	close_mets();
}

/*****************************************************************
	Help callback for top level window
*****************************************************************/
void WorkArea::HelpCB(Widget w, XtPointer client_data, XtPointer)
{
	WorkArea	*obj = (WorkArea *) client_data;

	obj->help(w);	
}

/*****************************************************************
	help routine that calls the dtm help routine to display
	help
*****************************************************************/
void WorkArea::help(Widget w)
{
	theHelpManager->DisplayHelp (w, &AppHelp);
}

/*****************************************************************
	Create a graphic context with the background color and the
	foreground color as the color passed as a parameter. This is in 
	order to set the color of the lines to be drawn in the color
	specified.
*****************************************************************/
GC WorkArea::GetColorGC (Widget w, int i)
{
	XGCValues 		val;	
	GC			gc;	

/*
  	val.background = theApplication->ColorEntry(i); 
	val.foreground = theApplication->ColorEntry(i);
*/
  	val.background = i;
	val.foreground = i;
  	gc = XCreateGC (XtDisplay(w), RootWindowOfScreen (XtScreen(w)), 
			GCForeground|GCBackground, &val);
	return gc;
  
}

/*****************************************************************
	Create a graphic context with the background color of the widget
	and the foreground color as the color passed as a parameter. This 
	is in order to set the color of the value to the color of the line
	when the button is pressed on the line.
*****************************************************************/
GC WorkArea::SetForegroundGC (Widget w, int i)
{
	XGCValues 		val;	
	GC			gc;	

	XtVaGetValues (w, XmNbackground, &val.background, 0);
/*
	val.foreground = theApplication->ColorEntry(i);
*/
	val.foreground = i;
  	gc = XCreateGC (XtDisplay(w), RootWindowOfScreen(XtScreen(w)), 
			GCBackground|GCForeground,
			&val);
	return gc;
  
}

/*****************************************************************
	Reset the colors of the sar list item whose alarm was
	removed. Also make sure the alarm icon is unmanaged.
*****************************************************************/
void WorkArea::ResetSarToggle(int i)
{
	static Pixel		fg, bg;

	/* reset header */
	if (_sar[i].flashed) {
		_sar[i].flashed = False;
		XtVaGetValues (_sar[i].toggle,
			XmNbackground, &fg, 
			XmNforeground, &bg,
			0);
		XtVaSetValues (_sar[i].toggle,
			XmNbackground, bg,
			XmNforeground, fg,
			0);
	}
	if (XtIsManaged (_sar[i].alarmicon))
		XtUnmanageChild (_sar[i].alarmicon);
}

/*****************************************************************
	Send a clear area to the drawing area canvas to
	generate an expose event that re-draws the canvas
*****************************************************************/
void WorkArea::ClearGraph()
{
	XClearArea(XtDisplay(_graph->baseWidget()),
			XtWindow(_graph->baseWidget()),0, 
			_graph->YOrigin(), _graph->Width(), 
			_graph->Height(), True); 
}

/*****************************************************************
	Draw the horizontal grid.  Set the boolean to true.
	And send a clear area to the drawing area canvas to
	generate an expose event that draws the horizontal lines
*****************************************************************/
void WorkArea::HorizontalLines(Boolean draw)
{
	_draw_HZ = draw;
	XClearArea(XtDisplay(_graph->baseWidget()),
			XtWindow(_graph->baseWidget()),0, 
			_graph->YOrigin(), _graph->Width(), 
			_graph->Height(), True); 
}

/*****************************************************************
	Draw the vertical grid.  Set the boolean to true.
	And send a clear area to the drawing area canvas to
	generate an expose event that draws the vertical lines
*****************************************************************/
void WorkArea::VerticalLines(Boolean draw)
{
	_draw_VT = draw;
	XClearArea(XtDisplay(_graph->baseWidget()),
			XtWindow(_graph->baseWidget()),0, 
			_graph->YOrigin(), _graph->Width(), 
			_graph->Height(), True); 
}

/****************************************************************
	Save the current settings of the sar options in the list.
	Save the color, scale and title of each option. Also save
	whether the option was selected or not in the past.
	Save the settings in the home directory of the user in	
	the file ".sys_mon"
******************************************************************/
void WorkArea::SaveSettings(Widget w)
{
	FILE		*fp;
	Boolean		failed = False; 
	XColor 		color;

	fp = fopen (_savefilename, "w");
	if (!fp) {
		theErrDialogMgr->postDialog (w, I18n::GetStr (TXT_errdialog), 
						I18n::GetStr (TXT_nofile));
		theErrDialogMgr->setModal(); 
	}
	else {
		char  	buf[BUFSIZ];
		int	i;

		/* 	get the values of sar options*/
		for (i = 0; i < SAR_TOTAL; i++)  { 
			color.pixel = _sar[i].color;
			XQueryColor (theApplication->display(), 
					theApplication->cmap(), &color);
			sprintf (buf, "%s:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n",
				_sar[i].title, _sar[i].scale, 
				color.red, color.green, color.blue, 
				_sar[i].selected, _sar[i].alarm_over, 
				_sar[i].alarm_under, _sar[i].beep, 
				_sar[i].flash, _sar[i].is_alarm);

			if (!fputs (buf, fp))  {
				failed = True;
				break;
			}
		}

		/* store the horizontal/vertical grid options
	 	 * and the timer value. 
		 */
		sprintf (buf, "H=%d:V=%d:T=%d\n", _draw_HZ, _draw_VT, _timer);
		if (!failed && !fputs (buf, fp))
			failed = True;
		if (failed) {
			theErrDialogMgr->postDialog (_w, I18n::GetStr 
				(TXT_errdialog),I18n::GetStr(TXT_savefailed));
			theErrDialogMgr->setModal(); 
		}
		fclose (fp);
	}
}

/*****************************************************************
	Set the value of the sar options in the table. If any
	settings had been saved earlier then restore the values
	in the table for color, scale and the selection option.
	Also set the variables for hz/vt grid and the timer value.
*****************************************************************/
void WorkArea::SetSarTable (Widget parent)
{
	char		tmp[BUFSIZ], *ptr;
	int		i = 0, j = 0;
	FILE		*fp;
	Boolean		not_found;

	/* Open the save settings file. If  the file is found set 
	 * the boolean variable not_found to be false.
	 */
	fp = fopen (_savefilename, "r");

	/* 	set up the SAR TABLE 	*/
	for (i = 0; i < SAR_TOTAL; i++)  {

		/* set widget toggle,scaletext & alarmicon  to null.
		 * set the title info and the default scale value
		 * also set the flashed boolean variable to false
		 */
		_sar[i].title = strdup (I18n::GetStr (list_struct[i].title));
		_sar[i].def_scale = atoi (I18n::GetStr (list_struct[i].scale));
		_sar[i].toggle = _sar[i].scaletext = _sar[i].alarmicon = NULL; 
		_sar[i].flashed =  False;

		not_found = True;

		/* if the line was found and text was there then make sure
		 * that values are retrieved for scale, color and selected,
		 * by parsing to the first colon and then getting each 
		 * value thereafter.
		 */
		if (fp) {
			fgets (tmp, BUFSIZ, fp);
			not_found = False;
			ptr = strtok (tmp, COLON);

			/* Compare the title in the file with the title
			 * we are going to use
			 */
			if (strncmp (ptr, _sar[i].title, strlen(_sar[i].title))
				!= 0) 
				not_found = True;
			else {
				XColor 		color;
				j = 0;
				while (ptr = strtok (NULL, COLON)) {
					switch (j) {
					case 0:	_sar[i].scale = atoi(ptr); 
						break;
					case 1: color.red = atoi(ptr); 
						break;
					case 2: color.green = atoi(ptr); 
						break;
					case 3: color.blue = atoi(ptr); 
						break;
					case 4: _sar[i].selected = atoi(ptr); 
						break;
					case 5: _sar[i].alarm_over = atoi(ptr); 
						break;
					case 6: _sar[i].alarm_under =atoi(ptr); 
						break;
					case 7: _sar[i].beep = atoi(ptr); 
						break;
					case 8: _sar[i].flash = atoi(ptr); 
						break;
					case 9: _sar[i].is_alarm = atoi(ptr); 
						break;
					}
					j++;
				}
				/* get the RGB values from the saved file and 
			 	 * try to get the appropriate pixel value using
				 * AllocColor. If it fails use black
				 */
				if (!XAllocColor (theApplication->display(),
					theApplication->cmap(), &color))
					_sar[i].color = (int)BlackPixel(
						theApplication->display (), 
						theApplication->scrnum()); 
				else
					_sar[i].color = (int)color.pixel;
			} /* sar line was found */
		} /*  the file was found */

		/* if the line for the option was not found (for whatever 
		 * reason)  then use the default values.
		 */
		if (not_found) {
			_sar[i].scale = atoi(I18n::GetStr (
						list_struct[i].scale));
			_sar[i].selected = False; 
			_sar[i].color = (int)BlackPixel(theApplication->display
						(), theApplication->scrnum()); 
			_sar[i].alarm_over = _sar[i].alarm_under = 0;
			_sar[i].beep = _sar[i].flash = 0;
			_sar[i].is_alarm = False;
		}
				
		/* set graphic context to color that was selected 
		 */
		_sar[i].gc  = GetColorGC (parent, _sar[i].color);

	} /* for all entries in the sar table */

	/* get the last line into the buffer - tmp
	 * parse till the first colon. Get the value
	 * after the colons and convert them to integer
	 * values.
	 */
	not_found = True;
	if (fp) {
		fgets (tmp, BUFSIZ, fp);
		not_found = False;
		ptr = strtok (tmp, EQUAL_TO);
		if (*ptr  != 'H') 
			not_found = True;
		else {
			i = 0;
			while  (ptr = strtok (NULL, EQUAL_TO)) {
				switch (i) {
					case 0:		_draw_HZ = atoi (ptr);
							break;
					case 1: 	_draw_VT = atoi (ptr);
							break;
					case 2: 	_timer = atoi (ptr);
							break;
				}
				i++;
			}
		}
	}
	
	/* If the last line is not found (for whatever reason) then don't 
	 * draw the grid and set the timer to 5 seconds.
	 */  
	if (not_found) {
		_draw_HZ = _draw_VT = False;
		_timer = TIMER_DEFAULT;
	}

	if (fp)
		fclose (fp);
}

/*****************************************************
 *	This routine creates a pixmap in xpm format 
 *****************************************************/
void WorkArea::CreatePixmap (Pixmap *icon_pixmap, char *path, long bg)
{
	Screen		*_myscreen;
	int		_width, _height;
	
	_myscreen = theApplication->screen();
	if (XReadPixmapFile((Display *)theApplication->display(), 
			(Drawable)RootWindowOfScreen(_myscreen),
            		(Colormap)DefaultColormapOfScreen(_myscreen),
                      	(char *) path, (unsigned int *)&_width, 
			(unsigned int *)&_height,
			(unsigned int)DefaultDepthOfScreen(_myscreen),
                      	(Pixmap *) icon_pixmap, bg) != BitmapSuccess)  {
				cout << "didnt work";
				exit (1);
	}
}

/*****************************************************
 * create the graphic context for the lines
 * to be drawn once. Get the foreground/background values.
 ***************************************************/
GC WorkArea::GetBlackGC (Widget w)
{
	XGCValues		nameval;
	GC			namegc;

	XtVaGetValues (w, 
			XmNforeground, &nameval.foreground,
			XmNbackground, &nameval.background,
			0);	

  	namegc = XCreateGC (XtDisplay(w), RootWindowOfScreen(XtScreen(w)), 
				GCForeground|GCBackground, &nameval);  
	return namegc;
}

/*****************************************************
 * create the GC for clear lines to be drawn.
 ***************************************************/
GC WorkArea::GetClearGC(Widget w)
{
	XGCValues		nameval;
	GC			cleargc;

	XtVaGetValues (_w,  
			XmNbackground, &nameval.background,
			XmNbackground, &nameval.foreground, 
			0);

  	cleargc = XCreateGC (XtDisplay(w), RootWindowOfScreen(XtScreen(w)), 
				GCForeground, &nameval);
	return cleargc;
}

/*****************************************************
 * Remove the timeout. Redraw the graph from the beginning
 * for the new processor which was chosen in the Options
 *****************************************************/
void WorkArea::RemovePlotTimeout()
{
	_graph->RemoveTimeOut();
	XClearArea(XtDisplay(_graph->baseWidget()),
			XtWindow(_graph->baseWidget()), 0, 0, 0, 0, True); 
}

/*****************************************************
 * If the log flag is set on and if the processors had
 * changed then stop the logging and close the file.
 * Also de-sensitise the menu button to stop logging,
 * and sensitize the button to start logging again.
 *****************************************************/
void WorkArea::IfLog(Widget w)
{
	if (_log) 
		theQuestionMgr->postQuestion (w, I18n::GetStr (TXT_logtitle), 
				I18n::GetStr (TXT_stoplogq), _menubar, 
				STOPLOGQ);
	else {
		theOptionsMgr->SetProc();
		XtDestroyWidget (theOptionsMgr->baseWidget());
	}
}

/**********************************************************************
 * Set the Proc message label to the number of the processor chosen 
 **********************************************************************/
void WorkArea::SetProcLabel()
{
	XmString		xmstr;
	char			buf[BUFSIZ], *tmp;

	if (_which_cpu == 99) 
		strcpy (buf, I18n::GetStr (TXT_global));  
	else
		sprintf (buf, "%d", _which_cpu);  

	tmp = (char *) malloc (strlen (I18n::GetStr (TXT_proc)) + strlen (buf) 
		+ 2);
	strcpy (tmp, I18n::GetStr (TXT_proc));
	strcat (tmp, buf); 

	xmstr = XmStringCreateLtoR (tmp, charset);
	XtVaSetValues (_iconbox->ProcLabelWidget(),
			XmNlabelString, 		xmstr,
			0);
	XmStringFree (xmstr);
	free (tmp);
}

/*****************************************************
 *	Install the new pixmap and set the background
 * 	of the iconified window. (top level shell)
 *****************************************************/
void InstallNewPixmap(Widget w, Pixmap pixmap)
{
	Window		iconWindow = 0, root;
	int 		x, y;
	unsigned int	width, height, border_width, depth;
	Display		*display;

	display = XtDisplay (w);

	XtVaGetValues (w, XmNiconWindow, &iconWindow, NULL);
	if ( iconWindow == 0 ) {

		if (!XGetGeometry (display, pixmap, &root, &x, &y, &width, 
			&height, &border_width, &depth) || 
			!(iconWindow = XCreateSimpleWindow (display, 
			root, 0, 0, width, height,  (unsigned) 0, 
			CopyFromParent, CopyFromParent))){ 
				XtVaSetValues (w, XmNiconPixmap, pixmap, NULL);
				return;
		}
		XtVaSetValues (w, XmNiconWindow, iconWindow, NULL);
	}

	XSetWindowBackgroundPixmap(display,iconWindow, pixmap);
	XClearWindow(display,iconWindow);
}

/*****************************************************
 *  The window is de-iconified.   Set the boolean to false
 *****************************************************/
static void Deiconified ( Widget , XEvent *, String *, Cardinal *)
{
    	iconified = False;
}

/*****************************************************
 *  The window is iconified.   Set the boolean to true
 *  and install the old pixmap back.  i.e the original
 *  one.
 *****************************************************/
static void Iconified ( Widget w, XEvent *, String *params, Cardinal *)
{
    	iconified = True;
	InstallNewPixmap(w, oldpixmap);
}

/*****************************************************************
	Plot the graph after the new timer has been set
	Remove the old timer id and set a new one
*****************************************************************/
void WorkArea::PlotGraph()
{
	_graph->RemoveTimeOut();
	_graph->ResetTimeOut ();
}

/*****************************************************************
	If any of the sar options have been selected sensitize
	the alarm button else de-sensitize it.
*****************************************************************/
void WorkArea::SetAlarmButton () 
{
	Boolean		sensitive = False;
	int		i;

	/*
	 * De-sensitize alarm if no sar options are selected;
	 */
	for (i = 0; i < SAR_TOTAL; i++) {	
		if (_sar[i].selected == True) {
			sensitive = True;
			break;
		}
	}

	if (sensitive)
		XtManageChild (_iconbox->IconAlarmWidget());
	else
		XtUnmanageChild (_iconbox->IconAlarmWidget());
	XtVaSetValues (_menubar->MenuAlarmWidget(), XmNsensitive, sensitive, 0);
}
