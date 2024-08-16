#ident	"@(#)systemmon:Play.C	1.6"
////////////////////////////////////////////////////////////////////
// Play.C:  Playback saved data 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "Play.h"
#include "WorkArea.h"
#include "Help.h"
#include "i18n.h"
#include "Buttons.h"
#include "ErrDialog.h"

#include <Xm/ArrowB.h>
#include <Xm/ScrolledW.h>
#include <Xm/DialogS.h>
#include <Xm/PanedW.h>
#include <Xm/Scale.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/DrawingA.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

Play *thePlayManager = new Play ();

/****************************************************************
		GLOBAL DEFINES
******************************************************************/
#define 		BACK		0	
#define 		PLAY		1
#define 		STOP		2
#define			selectHelpSect	"60"

static char		*mnemonics[2];
static char		playfilename[BUFSIZ];

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText PlayHelp = { 
	//HELP_FILE, TXT_appHelp, TXT_selectHelpSect, TXT_helptitle,
	HELP_FILE, TXT_appHelp, selectHelpSect, TXT_helptitle,
};

/****************************************************************
		ACTION ITEMS
******************************************************************/
static ButtonItem ActionItems[] = {
	{(XtArgVal)TXT_cancel, (XtArgVal)MNEM_cancel, 0, },
	{(XtArgVal)TXT_help, (XtArgVal)MNEM_help, (XtArgVal) 0,},
};

/****************************************************************
		CONSTRUCTOR FOR PLAYBACK - Initialize data
******************************************************************/
Play::Play () 
{
}

/****************************************************************
		Post the dialog each time it is destroyed
******************************************************************/
void Play::postDialog (Widget parent, char *file, char *name, WorkArea *w) 
{
	XmString	xmstr;
	Dimension	h;
	int		i, j, retcode;
	char		line[BUFSIZ], procno[BUFSIZ], *tmp; 
	struct stat	stat_buf;
	FILE		*fp;
	XtTranslations	trans_table;
	static int	first = 0;

	/***************************************************************
	 * If the playback file passed is not the same as the file being
	 * played back then inform viewer that playback is going on....
	 * Unfortunately, at this point user can see only one playback at
	 * a time.  Future release should support multiple playbacks.
	 ***************************************************************/
	if ((strncmp (playfilename, file, strlen (file)) != 0) && (_pane && 
		XtIsManaged (_pane))) {
		theErrDialogMgr->postDialog (parent, I18n::GetStr 
				(TXT_errdialog), I18n::GetStr (TXT_playing));
		theErrDialogMgr->setModal(); 
		return;
	}

	/****************************************************************
	 set the labels for the buttons
	 ******************************************************************/
	if (!first) {
		first = 1;
		for (i = 0; i < 2; i++ ) 
			mnemonics[i] = (char *)ActionItems[i]._mnem;
		Buttons::SetLabels (ActionItems, XtNumber (ActionItems));
		theHelpManager->SetHelpLabels ( &PlayHelp);
	}

	/****************************************************************
	 * if window exists then return else open the playback file and 
	 * discard the first line 
	 ******************************************************************/
	if (_pane && XtIsManaged (_pane)) 
		return;

	/****************************************************************
	 * Initialize all variables
	 ******************************************************************/
	_total = _playheight = _playwidth = _maxy = _previous = _lineno = 
	_lastvalue = 0;
	_stopflag = _rewind = False;
	_lasty = _mark = 0; 
	_timeid = NULL;
	_scaleval = 1000;
	retcode = 0;
	
	trans_table = XtParseTranslationTable ("<Key>Return: ArmAndActivate()");

	_workarea = w;

	for (i = 0; i < SAR_TOTAL; i++)
		_playdata[i].draw = True;

	/****************************************************************
	 * Set the clock in case we take too long 
	 ******************************************************************/
	XDefineCursor (theApplication->display(), XtWindow (parent), 
			theApplication->ClockCursor());
	XSync (theApplication->display(), False);

	/****************************************************************
	 * Open the log file to get the # of the lines in the file.	
	 * Store the name of the file for later use. 
	 ******************************************************************/
	fp = fopen (file, "r"); 	
	strcpy (playfilename, file);
	
	/****************************************************************
  	 * check the stat on the file, and get the size. If it is zero
	 * then the file is empty 
	 ******************************************************************/
  	if ((stat (file, &stat_buf)) == NULL) {
		if (!stat_buf.st_size)  {
			theErrDialogMgr->postDialog (parent, 
				I18n::GetStr (TXT_errdialog), 
				I18n::GetStr (TXT_empty));
			theErrDialogMgr->setModal(); 
			XUndefineCursor (theApplication->display(), XtWindow 
					(parent));
			XSync (theApplication->display(), False);
			return;
		}
	}
	/****************************************************************
	 * Get the proc # to add to title from the first line of the log
	 * file
	 ******************************************************************/
	i = 0;
	while (fgets (line, BUFSIZ, fp)) {
		if (!i) {
			tmp = line;
			while (*tmp++ != '#');
			while (*tmp != '\n') 		
				procno[i++] = *tmp++;
			procno[i] = '\0';
		}
		else 
			_lineno++;
	}
	
	/****************************************************************
	 * If the average processor is being viewed then enter
	 * Average in the procno array
	 ******************************************************************/
	if (atoi(procno) == GLOBAL) 
		strcpy (procno, I18n::GetStr (TXT_global));
	
	/************************************************************
	 * each time you come back from destroying the widget initialize
	 * create space for the playdata values and date/time/interval;
	 ************************************************************/
	_datestr = new XmString[_lineno];
	if (_datestr == NULL) 
		retcode = 1;

	if (!retcode) {
		_intstr = new XmString[_lineno];
		if (_intstr == NULL) 
			retcode = 1;
	}

	if (!retcode) {
		_timestr = new XmString[_lineno];
		if (_timestr == NULL) 
			retcode = 1;
	}

	if (!retcode) {
		for (i = 0; i < SAR_TOTAL; i++) {
			_playdata[i].value = new double [_lineno];
			if (_playdata[i].value == NULL)  {
				retcode = 1;
				break;
			}
			_playdata[i].y = new int [_lineno];
			if (_playdata[i].y == NULL) {
				retcode = 1;
				break;
			}
			for (j = 0; j < _lineno; j++) {
				_playdata[i].value[j] = 0;
				_playdata[i].y[j] = 0;
			}
		}
	}

	if (retcode) {
		theErrDialogMgr->postDialog (parent, I18n::GetStr 
				(TXT_errdialog), I18n::GetStr (TXT_nospace));
		theErrDialogMgr->setModal(); 
		XUndefineCursor (theApplication->display(), XtWindow (parent));
		XSync (theApplication->display(), False);
		return;
	}

	/****************************************************************
	 * create the form dialog to hold all the control area
	 * and the buttons, add filename to the title	
	 ******************************************************************/
	tmp = new char[(strlen (I18n::GetStr (TXT_playtitle)) + strlen
			(file) + strlen (I18n::GetStr (TXT_proc)) + 
			strlen (procno) + 2)];
	if (tmp == NULL) {
		theErrDialogMgr->postDialog (parent, I18n::GetStr
			(TXT_errdialog), I18n::GetStr (TXT_nospace));
		theErrDialogMgr->setModal(); 
		XUndefineCursor (theApplication->display(), XtWindow (parent));
		XSync (theApplication->display(), False);
		return;
	}
	strcpy (tmp, I18n::GetStr (TXT_proc)); 
	strcat (tmp, procno); 
	strcat (tmp, I18n::GetStr (TXT_playtitle)); 
	strcat (tmp, file); 
	_w = XtVaCreatePopupShell (name,  xmDialogShellWidgetClass, 
				_workarea->baseWidget(),
				XmNtitle,		tmp,	
				XmNdeleteResponse, XmDESTROY,
				XmNnoResize, 		False,
				0);
	delete tmp;
	XtAddCallback (_w, XmNdestroyCallback, &Play::destroyCB, this);
	
	//
	// Create an under-laying form widget, all others placed on top.
 	//  We get cancelButton and defaultButton behavior this way.
	//
	_shellForm = XtVaCreateWidget ("shellForm", xmFormWidgetClass, _w,
					NULL);

	/****************************************************************
	 * create the paned window to hold the areas below
	 ******************************************************************/
	_pane = XtVaCreateManagedWidget ("Playpane",xmPanedWindowWidgetClass, 
					_shellForm, 0);

	/****************************************************************
	 * create the row column to hold the time and interval and date 
	 ******************************************************************/
	_daterc = XtVaCreateWidget ("daterc",xmRowColumnWidgetClass, _pane, 
					XmNorientation, XmHORIZONTAL,
					0);

	/****************************************************************
	 * create the label for the last update time
	 ******************************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_time),charset);
	_lastupdate = XtVaCreateManagedWidget ("lastupdate",
						xmLabelWidgetClass, 	_daterc,
						XmNlabelString, 	xmstr,	
						0);
	XmStringFree (xmstr);

	/****************************************************************
	 * create the time widget to hold the actual time
	 ******************************************************************/
	xmstr = XmStringCreateLtoR ("00:00:00",charset);
	_time = XtVaCreateManagedWidget ("time", xmLabelWidgetClass, _daterc,
					XmNlabelString, 	xmstr,	
					0);
	XmStringFree (xmstr);

	/****************************************************************
	 * create the interval label 
	 ******************************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_interval),charset);
	_intervallabel = XtVaCreateManagedWidget ("intervallabel",
						xmLabelWidgetClass, 	_daterc,
						XmNlabelString, 	xmstr,	
						0);
	XmStringFree (xmstr);

	/****************************************************************
	 * create the interval widget to hold the value
	 ******************************************************************/
	_interval = XtVaCreateManagedWidget ("interval", xmLabelWidgetClass, 
					_daterc,
					XmNlabelString, 	NULL,	
					0);
					
	/****************************************************************
	 * create the date label
	 ******************************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_date),charset);
	_date = XtVaCreateManagedWidget ("date",
					xmLabelWidgetClass, 	_daterc,
					XmNlabelString, 	xmstr,	
					0);
	XmStringFree (xmstr);

	/****************************************************************
	 * create the date widget to hold the value of the date, manage
	 * the row column manager and set the minimum height to prevent
	 * resizing
	 ******************************************************************/
	_datestring = XtVaCreateManagedWidget ("datestring",xmLabelWidgetClass, 
					_daterc,
					XmNlabelString, 	NULL,	
					0);

	XtManageChild (_daterc);
	XtVaGetValues (_datestring, XmNheight, &h, 0);
	XtVaSetValues (_daterc, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	/****************************************************************
	 * button row column to hold the play/rewind/stop buttons 
	 ******************************************************************/
	_buttonrc  = XtVaCreateWidget ("buttonrc", 
					xmRowColumnWidgetClass, _pane,
					XmNorientation, 	XmHORIZONTAL,	
					NULL);

	/****************************************************************
	 * create the stop , play and rewind push buttons
	 ******************************************************************/
	for (i = 0 ; i < 3; i ++) {
		_action = XtVaCreateManagedWidget ("Action", 
					xmPushButtonWidgetClass,_buttonrc,
					NULL);
		switch (i) {
			case 0: xmstr = XmStringCreateLtoR (I18n::GetStr 
						(TXT_back),charset);
				break;
			case 1: xmstr = XmStringCreateLtoR (I18n::
						GetStr (TXT_play),charset);
				break;
			case 2:	xmstr = XmStringCreateLtoR (I18n::GetStr 
					(TXT_stop),charset);
				break;
		}
		XtVaSetValues (_action,
				XmNlabelString, 	xmstr,	
				XmNuserData, 		i,	
				0);
		XtAddCallback (_action, XmNactivateCallback, actionCB, this);
		XmStringFree (xmstr);
		XtOverrideTranslations (_action, trans_table);
	}

	/****************************************************************
	 * Create the scale label for sliding the timer value
	 ******************************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_speed),charset);
	_speed  = XtVaCreateManagedWidget ("speed", xmLabelWidgetClass, 
					_buttonrc,
					XmNlabelString, 	xmstr,	
					NULL);
	XmStringFree (xmstr);

	/****************************************************************
	 * Create the actual scale for sliding the timer value
	 * add a callback to it, manage the button row colum and set height
	 ******************************************************************/
	_scale  = XtVaCreateManagedWidget ("scale", xmScaleWidgetClass, 
					_buttonrc,
					XmNorientation, 	XmHORIZONTAL,	
					XmNprocessingDirection, 	
								XmMAX_ON_LEFT,	
					XmNminimum, 		100,	
					XmNmaximum, 		10000,	
					XmNvalue, 		_scaleval,	
					NULL);
	XtAddCallback (_scale, XmNvalueChangedCallback, &Play::SliderCB, 
			(XtPointer)this);

	XtManageChild (_buttonrc);
	XtVaGetValues (_action, XmNheight, &h, 0);
	XtVaSetValues (_buttonrc, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	/****************************************************************
	 * create the drawing area widget to hold the playback graph data
	 ******************************************************************/
	_canvas = XtVaCreateManagedWidget ("Playcanvas", 
					xmDrawingAreaWidgetClass, _pane, 
					XmNtraversalOn,	False,
					0);

	/****************************************************************
	 * register all the callbacks for the graph - expose and resize 
	 * and input
	 ******************************************************************/
	XtAddCallback (_canvas, XmNresizeCallback, &Play::ResizeCB, 
			(XtPointer)this);
    	XtAddCallback (_canvas, XmNexposeCallback, &Play::ExposeCB, 
			(XtPointer)this);

	/****************************************************************
	 * create the scrolled list of the sar options, a row column to
	 * hold all the toggle values and set the scrolled window
	 ******************************************************************/
	_sw = XtVaCreateManagedWidget ("sw", xmScrolledWindowWidgetClass,_pane, 
					XmNscrollingPolicy, XmAUTOMATIC,
				 	0);

	_rc = XtVaCreateManagedWidget ("rc", xmRowColumnWidgetClass, _sw, 
					XmNspacing,		7, 
					0);
	
	for (i = 0; i < SAR_TOTAL;i++) {

		_toggle =  XtVaCreateManagedWidget ("toggle", 
					xmToggleButtonWidgetClass, _rc, 
					XmNalignment,  XmALIGNMENT_BEGINNING,
					XmNlabelString, XmStringCreateLtoR(
					_workarea->_sar[i].title, charset),
					XmNforeground,_workarea->_sar[i].color,
					XmNuserData, 		i,
					XmNset,			True, 
					0);
		XtAddCallback (_toggle, 
				XmNvalueChangedCallback, &Play::toggleCB, 
				this);
	}
	XmScrolledWindowSetAreas (_sw, NULL, NULL, _rc);

	/****************************************************************
	 * create the form for the lower area to hold the
	 * action buttons
	 ******************************************************************/
	_actionform = XtVaCreateManagedWidget ("actionform", xmFormWidgetClass, 
						_pane, 
						XmNfractionBase, 	5,
						NULL);

	/****************************************************************
	 * set the function pointers to the appropriate callbacks
	 ******************************************************************/
	ActionItems[0]._proc_ptr = (XtArgVal)Play::cancelCB;
	ActionItems[1]._proc_ptr = (XtArgVal)Play::helpCB;

	/****************************************************************
	 * create the action buttons
	 ******************************************************************/
	int position = 1;
	for (i = 0; i < 2; i++)  {
		xmstr = XmStringCreateLtoR((char *)ActionItems[i]._label, 
								charset);
    		_actionbuttons[i] = XtVaCreateManagedWidget ("actionbuttons",
				xmPushButtonWidgetClass, _actionform, 
				XmNlabelString, 	xmstr,
//				XmNmnemonic, 		ActionItems[i]._mnem,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNleftAttachment, 	XmATTACH_POSITION,
				XmNleftPosition, 	position,
				XmNrightAttachment, 	XmATTACH_POSITION,
				XmNrightPosition, 	position + 1,
				XmNshowAsDefault, 	i == 0 ? True:False,
				XmNdefaultButtonShadowThickness, 	1,
				0);
		position += 2;
		XtAddCallback (_actionbuttons[i], XmNactivateCallback,	
			(XtCallbackProc)ActionItems[i]._proc_ptr, this);		
		XmStringFree (xmstr);

		XtOverrideTranslations (_actionbuttons[i], trans_table);
	}

	for(i = 0; i < 2; i++) 
		registerMnemInfo(_actionbuttons[i], (char *)mnemonics[i], 
					MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	registerMnemInfoOnShell(_w);

	/****************************************************************
	 * manage the action form area
	 ******************************************************************/
	XtManageChild (_actionform);

	/****************************************************************
	 * make sure the action area does not resize
	 ******************************************************************/
	XtVaGetValues (_actionbuttons[0], XmNheight, &h, 0);
	XtVaSetValues (_actionform, XmNpaneMaximum, h, XmNpaneMinimum, h, 0);

	/****************************************************************
	 * manage the pane window , get the graphic context to draw lines in
	 * the playback canvas , set max x value and popup the transient shell 
	 ******************************************************************/
	XtManageChild (_shellForm);
	XtVaSetValues (_shellForm, XmNcancelButton, _actionbuttons[0], 0);

	_gc = _workarea->GetBlackGC(_pane); 
	_maxx = theApplication->Max_X (); 

	XtPopup (_w, XtGrabNone);

	/* do not allow the width to resize
	 */
	XtVaGetValues (_w, XmNwidth, &i, 0);
	XtVaSetValues (_w, XmNmaxWidth, i, 0);

	/* remove the clock
	 */
	XUndefineCursor (theApplication->display(), XtWindow (parent));
	XSync (theApplication->display(), False);
	XtDestroyWidget (parent);
}

/****************************************************************
	dtor to kill the class
 ******************************************************************/
Play::~Play ()
{
	_w = NULL;
}

/****************************************************************
 * Cancel callback as static function calls cancel ()
 ******************************************************************/
void   Play::cancelCB(Widget, XtPointer client_data, XtPointer )
{
	Play *obj = (Play *) client_data;
	obj->cancel () ;
}

/****************************************************************
 * destroys the widget for the playback class
 ******************************************************************/
void Play::cancel ()
{
	XtDestroyWidget (_w);
}

/****************************************************************
 * Help callback calls help ()
 ******************************************************************/
void   Play::helpCB(Widget w, XtPointer client_data, XtPointer )
{
	Play *obj = (Play *) client_data;
	obj->help (w) ;
}

/****************************************************************
 * Displays desktop help for Playback screen
 ******************************************************************/
void Play::help (Widget w)
{
	theHelpManager->DisplayHelp (w, &PlayHelp);
}

/*****************************************************************
	destroy callback calls destroy routine () 
*****************************************************************/
void Play::destroyCB (Widget , XtPointer client_data, XtPointer ) 
{
	Play	*obj = (Play *) client_data;

	obj->destroy ();
}

/*****************************************************************
	Make sure you also set pane (popup's child) to null.
	So that when it returns it does not think that pane is
	still up. Remove the timer proc and set all new'ed
	areas free. 
*****************************************************************/
void Play::destroy()
{
	int	i;

	_w = _pane = _canvas = NULL;
	if (_timeid)  {
		XtRemoveTimeOut (_timeid);
		_timeid = NULL;
	}

	for (i = 0; i < SAR_TOTAL; i++) {
		delete _playdata[i].value;
		delete _playdata[i].y;
	}

	delete _datestr;
	delete _intstr;
	delete _timestr;
}

/*-------------------------------------------------------------
 *           SliderCB calls slider proc with the calldata
 *------------------------------------------------------------*/
void   Play::SliderCB(Widget, XtPointer client_data, XtPointer call_data)
{
	Play *obj = (Play *) client_data;
	XmScaleCallbackStruct 	*cb = ( XmScaleCallbackStruct 	*) call_data;


	obj->slider (cb) ;
}

/*-------------------------------------------------------------
 * set the new timer value and clear the canvas - causes exposure
 *------------------------------------------------------------*/
void Play::slider (XmScaleCallbackStruct *cb)
{
	_scaleval = cb->value;	
	XClearArea(XtDisplay (_canvas),XtWindow (_canvas), 0, 0, 0, 0, True);
}

/*-------------------------------------------------------------
 *           toggleCB 
 *------------------------------------------------------------*/
void   Play::toggleCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Play *obj = (Play *) client_data;
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *) 
						call_data;
	obj->toggle (w, cb) ;
}

/*-------------------------------------------------------------
 * 	toggle switch to toggle the sar options - send expose
 * 	event upon toggle to clear or display the option.
 *------------------------------------------------------------*/
void Play::toggle (Widget w,  XmToggleButtonCallbackStruct *cb)
{
	int	i;
	
	XtVaGetValues (w, XmNuserData, &i, 0);
	_playdata[i].draw = cb->set;
	XClearArea(XtDisplay (_canvas),XtWindow (_canvas), 0, 0, 0, 0,  True);
}

/*-------------------------------------------------------------
	play button,  back button, stop button
  *------------------------------------------------------------*/
void   Play::actionCB(Widget w, XtPointer client_data, XtPointer )
{
	Play *obj = (Play *) client_data;
	obj->action (w) ;
}

/*-------------------------------------------------------------
	switch the userdata based on what action is to be taken
  *------------------------------------------------------------*/
void Play::action(Widget w)
{
	int	i;
	
	XtVaGetValues (w, XmNuserData, &i, 0);
	switch (i) { 
		case PLAY:
			_stopflag = False; _rewind = False;
			break;
		case BACK:
			_stopflag = False; _rewind = True;
			break;
		case STOP:
			_stopflag = True; _rewind = False;
			break;
	}
}

/*-------------------------------------------------------------
  **	playback resize
  *------------------------------------------------------------*/
void  Play::ResizeCB(Widget, XtPointer client_data, XtPointer call_data) 
{
//	XmDrawingAreaCallbackStruct *cb = (XmDrawingAreaCallbackStruct *) 
//					call_data;

	Play *obj = (Play *) client_data;
	obj->resize ();
}

/*-------------------------------------------------------------
	resize the playarea - get the new width and height
	clear the play area and send an expose event if the canvas
	is realized
  *------------------------------------------------------------*/
void Play::resize ()
{
  	XtVaGetValues (_canvas, 
			XmNwidth, &_playwidth, 
			XmNheight, &_playheight,
			0);
	if (XtIsRealized (_canvas))
		XClearArea(XtDisplay(_canvas), XtWindow (_canvas), 0, 0, 0, 0, 
			True);
}

/*-------------------------------------------------------------
  **	playback expose
  **            playback 
  *------------------------------------------------------------*/
void   Play::ExposeCB(Widget, XtPointer client_data, XtPointer call_data)
{
//	XmDrawingAreaCallbackStruct *cb = (XmDrawingAreaCallbackStruct *) 
//						call_data;
	Play *obj = (Play *) client_data;
	obj->expose () ;
}

 /**------------------------------------------------------------
  ** expose event : first time it is entered add the timeout
  ** procedure otherwise redraw the screen that is exposed.
  **-----------------------------------------------------------*/
void Play::expose ()
{
	double		eachy;

	/*****************************************************
	 * the height of each mark and the last mark on screen
	 ***************************************************/
	_maxy = _playheight - YOFFSET; 				/* Y ORIGIN */
	eachy = (_maxy - YOFFSET) / PERCENTAGE ;		/* Each y */
	_mark = floor((eachy * UNIT) + 0.5);			/* MARK */
	_lasty = _maxy - (_mark * UNIT);

	/* DRAW THE X AND Y AXIS LINES 
	 */
	DrawXandYAxis (_canvas); 		

	/*****************************************************
	 * the timeout procedure should be registered here
	***************************************************/
	if (_timeid == NULL) {
		_total = _previous = _lastvalue = 0;
		playback (this, NULL);
	}
	else {
		/*****************************************************
		 * for all the points that have been drawn for each of
	 	 * the lines draw them again on an expose event
	 	***************************************************/
		int 	i, j, x;

		for (j = 0; j < SAR_TOTAL; j++)  {
			x = _maxx + PIXEL;
			DrawLine (j, _previous, x, True);
			for (i = _previous + 1; i < _lastvalue; i++)  {
				x += EACH_WIDTH;
				DrawLine (j, i, x, False);
			}
		}
	}
}

/*****************************************************
 * A generic draw x and y axis routine. 
 ***************************************************/
void Play::DrawXandYAxis (Widget w)
{
  	int             	i, x, y;
	char			buf[10];
	int 			asc, desc, d;
	XCharStruct		all;
	
  	/* draw the x/y axis line */
  	XDrawLine (XtDisplay(w), XtWindow(w), _gc, _maxx, _maxy, _playwidth, 
			_maxy);
  	XDrawLine (XtDisplay(w), XtWindow(w), _gc, _maxx, _maxy, _maxx, 0);

  	/*****************************************************
	 * draw the y values for usage on the line 
	 * depending on the scale that is being used
	 ***************************************************/
	x = 0; y = _maxy; 

  	for (i = 0; i <= PERCENT; i += (PERCENT / UNIT)) {

		sprintf (buf, "%d", i);
		XTextExtents (theApplication->AppFont(),  buf, strlen (buf),  
				&d, &asc, &desc, &all); /* TEXT extents of x */
		x = all.width; 				/* width of number */
		x = _maxx - x;				/* position of x  */

	 	/* draw the coord strings */
	 	XDrawImageString (XtDisplay(w),XtWindow(w), _gc, x, y, buf, 
				strlen (buf));

		if (i != PERCENT) 
	 		y -=(int) _mark; 		/* - y by eachy */
	}
}

/*****************************************************************
 * 	Draw the Line for each value of the item being displayed
 *****************************************************************/
void Play::DrawLine (int j, int i, int x, Boolean _draw_from_beginning)
{
	double 		y = (double) _maxy;
	double		dval, value;
	
	value = _playdata[j].value[i];

	/************************************************************
	 * 	Calculate the y values 
	 ************************************************************/
	if (value != PERCENTAGE) {
		dval = floor ((value * (_mark/UNIT)));
		if (y - dval < _lasty)  
			(_playdata[j].y[i]) = (int)_lasty;
		else	
			(_playdata[j].y[i]) = (int)(y - dval);
	}
	else
		(_playdata[j].y[i]) = (int)_lasty;

	/************************************************************
	 * draw the lines for each value from one width to another
	 * if the first item or if it is starting at the beginning
	 * merely use the same y values at the starting point, otherwise
	 * use the previous y values.
	 ************************************************************/
	if (_playdata[j].draw ) 
		XDrawLine (XtDisplay(_canvas), XtWindow(_canvas), 
			_workarea->_sar[j].gc, x, (i == 0 || 
			_draw_from_beginning) ? (_playdata[j].y[i]) :
			(_playdata[j].y[i-1]), x + EACH_WIDTH, 
			(_playdata[j].y[i]));
}

/************************************************************
 * This timeout procedure is called repeatedly. It rewinds,plays
 * and stops the graph that is being reviewed.
 ************************************************************/
void Play::playback (XtPointer client_data, XtIntervalId *)
{
	Play	*obj	= 	(Play *) client_data;
	int			j, i, x;
	char			*tmp, line[BUFSIZ], timetmp[BUFSIZ];
	static int		eachx = 0; 
	time_t			clock;
	struct tm 		*timeset;

	if (obj->_timeid == NULL) 
		eachx = obj->_maxx + PIXEL + EACH_WIDTH; 

	/************************************************************
	 * create the timeout - so that this routine keeps getting
	 * called  again and again
	 ************************************************************/
	obj->_timeid = XtAppAddTimeOut (theApplication->appContext (), 
					obj->_scaleval, 
					(XtTimerCallbackProc) &Play::playback, 
					obj);

	/************************************************************
	 * if asked to stop the playback then just return
	 ************************************************************/
	if (obj->_stopflag) 
		return;

	/************************************************************
	 * if asked to rewind and the values have not exceeded width of
	 * screen then ring bell and stop,  else rewind all
	 ************************************************************/
	if (obj->_rewind) {

		if (obj->_previous) {
			obj->_previous--; 
			obj->_lastvalue--;

			obj->DrawAllLines(obj->_previous,obj->_lastvalue);
			return;
		}
		/*****************************************************
		* nothing to rewind, stop the train, sound bell and return 
		******************************************************/
		else {
			obj->_rewind = False;
			XBell (XtDisplay (obj->_canvas), 100);
			obj->_stopflag = True;
			return;
		}
	}

	/************************************************************
	 * get each line from the log file.  If the lines have all not
	 * been read then keep reading.... 
	 ************************************************************/
	if (obj->_total != obj->_lineno)  {

		/************************************************
	 	 * if the max width has been reached clear window 
		 * draw x/y axis push all points to the left by one 
		 * value i.e scroll to left to do this starting 
		 * counting previous values.
	 	 ************************************************/
		if (eachx > obj->_playwidth) {
			XClearWindow (XtDisplay(obj->_canvas), 
					XtWindow(obj->_canvas));
			obj->DrawXandYAxis (obj->_canvas); 	
			obj->_previous++;
		}

		/************************************************************
	 	 *  We have to use total + 1 becoz the first line is not used.
	 	************************************************************/
		obj->GetLine (obj->_total+1, line);
		
		/************************************************************
	 	 * Loop thru all the semi colon entries and display the value 
	 	 * as they are encountered starting with time/interval/date
	 	 ************************************************************/
		tmp = strtok (line ,";");
		clock = atoi (tmp);	
		timeset = localtime(&clock);

		sprintf (timetmp, "%d/%d/%d", timeset->tm_mon + 1, 
			timeset->tm_mday, timeset->tm_year);
		obj->_timestr[obj->_total] = XmStringCreateLtoR(timetmp,
								charset);

		sprintf (timetmp, "%02d:%02d:%02d", timeset->tm_hour, 
			timeset->tm_min, timeset->tm_sec);
		obj->_datestr[obj->_total] = XmStringCreateLtoR(timetmp, 
								charset);	
		tmp = strtok (NULL,";");
		obj->_intstr[obj->_total] = XmStringCreateLtoR(tmp,charset);

		j = 0;
		while (tmp = strtok (NULL, ";")) {
					
			/**************************************
			 * Get value . If max width is reached 
			 * redraw pts from previous to last 
	 		 **************************************/
			obj->_playdata[j].value[obj->_total]
					 = (double) atoi(tmp);
			if (eachx > obj->_playwidth)  {
				x = obj->_maxx + PIXEL; 
				obj->DrawLine (j,obj->_previous, x, True);
				for (i = obj->_previous + 1; 
					i <= obj->_lastvalue; i++) {
						x += EACH_WIDTH;
						obj->DrawLine (j, i, x, False);
					}
			}
			else
				/**************************************
				 * draw the last point if the screen
				 * width has not been reached
	 			 **************************************/
				obj->DrawLine (j, obj->_total, 	
						eachx - EACH_WIDTH, False);

			j++;  /* increment the sar options */

		} 		/* till all semi colon entries are read */
		
		
		/**********************************************************
		 * If j did not make it thru all the parameters, i.e there
		 * might have been an incomplete line in the file. so re-
		 * draw everything till last point. Reduce the line number
	 	 * by one. First clear the window. Make sure you go back
		 * one point as well, which means _previous--.
 		 *********************************************************/
		if (j != SAR_TOTAL) { 

			XClearWindow (XtDisplay(obj->_canvas), 
					XtWindow(obj->_canvas));
			obj->DrawXandYAxis (obj->_canvas); 	

			obj->_previous--;

 			for (j = 0; j < SAR_TOTAL; j++) {

				x = obj->_maxx + PIXEL; 
				obj->DrawLine (j, obj->_previous, x, True);

				for (i = obj->_previous + 1; 
					i < obj->_lastvalue; i++) {
					x += EACH_WIDTH;
					obj->DrawLine (j, i, x, False);
				}

			}
			obj->_lineno--;
		}
		else {

			/****************************************************
  		 	 * set the time/date interval & INCREMENT THE COUNTERS 
 		 	 ***************************************************/
			obj->SetTimeDateInterval (obj->_lastvalue);

			obj->_total++; 		/* add up the cycles */
			obj->_lastvalue++; 	/* add up last value times */
		}

		/*********************************************************
		 * If the width being added to eachx is less than overall 
		 * width then increment it.
 		 *******************************************************/
		if (eachx <=  obj->_playwidth) 
			eachx += EACH_WIDTH; /* increment the eachx value */
	}

	/*************************************
 	 * No more values to print - all done
 	 ************************************/
	else if (obj->_total == obj->_lastvalue) {
		XBell (XtDisplay (obj->_canvas), 100);
		obj->_stopflag = True;
		return;
	}

	/***************************************************
 	 * If rewind has occured and play is being requested 
	 * increment the lastvalue and previous values and go 
	 * forward.
 	 **************************************************/
	else if (obj->_lastvalue < obj->_total) {

		obj->_lastvalue++;
		obj->_previous++;

		obj->DrawAllLines(obj->_previous, obj->_lastvalue);
	}
} 

/******************************************************************
	Re-Draw the lines for rewind/play  from previous to last
	for all sar options one by one
******************************************************************/
void Play::DrawAllLines(int previous, int last)
{
	int		x, i, j;
			
	XClearWindow (XtDisplay(_canvas), XtWindow(_canvas));
	DrawXandYAxis (_canvas); 	

 	for (j = 0; j < SAR_TOTAL; j++) {
		x = _maxx + PIXEL; 
		DrawLine (j, previous, x, True);
		for (i = previous + 1; i < last; i++) {
			x += EACH_WIDTH;
			DrawLine (j, i, x, False);
		}
	}
	SetTimeDateInterval (last - 1);
}

/******************************************************************
	Set the time/date/interval values
 ******************************************************************/
void Play::SetTimeDateInterval (int i)
{
	XtVaSetValues(_datestring, XmNlabelString, _timestr[i],0);
	XtVaSetValues(_interval, XmNlabelString, _intstr[i],0);
	XtVaSetValues(_time, XmNlabelString, _datestr[i],0);
}

/******************************************************************
 * Get the line requested from the log file into the buffer passed
 ******************************************************************/
void Play::GetLine (int thisline, char *s)
{
	static FILE	*fp;
	
	if (thisline == 1) {
		fp = fopen (playfilename, "r");
		fgets (s, BUFSIZ, fp);
	}
	
	fgets (s, BUFSIZ, fp);
	if (thisline == _lineno) {
		fclose (fp);
	}
}
