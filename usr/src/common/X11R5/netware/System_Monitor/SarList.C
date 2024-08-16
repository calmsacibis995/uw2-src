#ident	"@(#)systemmon:SarList.C	1.6"
/////////////////////////////////////////////////////////////////////
// SarList.C:  List of counters 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "SarList.h"
#include "WorkArea.h"
#include "Graph.h"
#include "ErrDialog.h"
#include "i18n.h"
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/ScrolledW.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define XRMFILE 		"/usr/X/lib/app-defaults/System_Monitor"

static char *scale_string [] = {
	TXT_10,
	TXT_100,
	TXT_1000,
	TXT_10000,
	TXT_100000,
};

static char *alarmpixmap = "/usr/X/lib/pixmaps/alarm16.icon";

SarList::SarList (Widget parent, WorkArea *w, char *name, Graph *g) 
				: BasicComponent (name)
{
	char 		*tmp;
	int		i, ac;
	Arg		al[5];
	XmString	*xmstring, xmstr;
	Pixmap		pixmap;
	XrmDatabase	_db;
	struct stat	buf;
	Boolean		do_not_getresource = False;

	_workArea = w;
	_graph_area = g;
	_toggle_set = _toggle_num = 0;

	/*******************************************************
	 * Create the form to hold all the gui components i.e
	 * Scrolled list of SAR options, scale options,color options
	 *******************************************************/
	_w = XtVaCreateManagedWidget (name, xmFormWidgetClass, parent, 0);

	/*******************************************************
	 * List of all sar Options that are available are in a 
	 * scrolled list as toggle buttons with the scale that 
	 * is being shown to the right of the item.
	 *******************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_list), charset);
	_listlabel = XtVaCreateManagedWidget ("listlabel", xmLabelWidgetClass, 
					 	_w, 
					XmNleftAttachment, XmATTACH_FORM,
					XmNtopAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition,  60,
					XmNalignment,  XmALIGNMENT_BEGINNING,
					XmNlabelString,  xmstr,
				 	0);
	XmStringFree (xmstr);

	_sw = XtVaCreateManagedWidget ("sw", xmScrolledWindowWidgetClass, 
					_w, 
					XmNscrollingPolicy, XmAUTOMATIC,
					XmNleftAttachment, XmATTACH_FORM,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, _listlabel,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition,  60,
					XmNbottomAttachment, XmATTACH_FORM,
				 	0);

	_rc = XtVaCreateManagedWidget ("rc", xmRowColumnWidgetClass, _sw, 0);
	
	/*******************************************************
	 * Create temporary storage for the "SCALE=" variable
	 * for later use
	 *******************************************************/
	_scalebuf  = XtMalloc (strlen (I18n::GetStr(TXT_scale)) + 
				strlen (EQUAL_TO) + 2);
	strcpy (_scalebuf, I18n::GetStr (TXT_scale));
	strcat (_scalebuf, EQUAL_TO);

	/* If the save settings file exists then that takes
	 * precedence else get the resource from the appdefaults
	 * file 
	 */
	if ((stat (SAVE_FILE, &buf)) != -1)  {
		if (buf.st_size > 0) 
			do_not_getresource = True;
	}

	if (do_not_getresource == False) 
		_db = XrmGetFileDatabase (XRMFILE);

	/*******************************************************
	 * The sar table of options is drawn here - the title 
	 * string,  scale variables and the alarm icon (not managed)
	 *******************************************************/
	for (i = 0; i < SAR_TOTAL;i++) {

		char 		buf[6], appdefaults[64];
		XColor		retrgb, closestrgb;
		XrmValue	value;
		char*		strType[20];
    	
		/*******************************************************
		 * Scale buffer is tmp -  contains the SCALE=<variable
		 * from the Sar structure>
	 	 *******************************************************/
		sprintf (buf, "%d", _workArea->_sar[i].scale);
		tmp = XtMalloc (strlen (buf) + strlen(_scalebuf) + 1);
		strcpy (tmp, _scalebuf); 
		strcat(tmp, buf);

		_tform = XtVaCreateWidget ("tform", xmFormWidgetClass, _rc, 
					XmNnavigationType, XmNONE,
					0);

		/* create the alarmicon pixmap here
		 */
		_workArea->CreatePixmap (&pixmap, alarmpixmap, 
					theApplication->Background());

		/* get the appdefault value from the appdefault file 
		 * if the save settings file is not found
		 */
		if (do_not_getresource == False) {
			sprintf (appdefaults, "%s%d%s","*SarOption", i, 
				"*foreground");
			XrmGetResource (_db, appdefaults, appdefaults, strType, 
					&value);
			if (value.addr) {
				if ((XAllocNamedColor(theApplication->display(),
							theApplication->cmap(), 
							value.addr, &retrgb, 
							&closestrgb)) != 0)
				_workArea->_sar[i].color = retrgb.pixel;
				_workArea->_sar[i].gc = _workArea->GetColorGC
							(parent, retrgb.pixel);
			}
		}
		sprintf (appdefaults, "%s%d","SarOption", i);

		xmstr = XmStringCreateLtoR(_workArea->_sar[i].title, charset);
		_workArea->_sar[i].toggle =  XtVaCreateManagedWidget (
					appdefaults, 
					xmToggleButtonWidgetClass, _tform, 
					XmNalignment,  XmALIGNMENT_BEGINNING,
					XmNlabelString, xmstr, 
					XmNforeground,_workArea->_sar[i].color,
					XmNuserData,		i,
					XmNset, _workArea->_sar[i].selected,
					XmNleftAttachment, 	XmATTACH_FORM,
					XmNtopAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition,  	60,
					0);
		XmStringFree (xmstr);

		XtVaGetValues (_workArea->_sar[i].toggle, 
				XmNforeground, &_workArea->_sar[i].foreground, 
				XmNbackground, &_workArea->_sar[i].background, 
				0);

		xmstr = XmStringCreateLtoR (tmp, charset);
		_workArea->_sar[i].scaletext = XtVaCreateManagedWidget (
					"scaletext", 
					xmLabelWidgetClass, _tform, 
					XmNforeground,_workArea->_sar[i].color,
					XmNleftAttachment,XmATTACH_WIDGET,
					XmNleftWidget,  
						_workArea->_sar[i].toggle,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition,   90,
					XmNtopAttachment, 	XmATTACH_FORM,
					XmNbottomAttachment, 	XmATTACH_FORM,
					XmNlabelString, 	xmstr,
				 	0);
		XmStringFree (xmstr);

		_workArea->_sar[i].alarmicon = XtVaCreateWidget("alarmicon", 
				xmLabelWidgetClass, _tform, 
				XmNlabelType,		XmPIXMAP,
				XmNlabelPixmap,		pixmap,
				XmNleftAttachment,	XmATTACH_WIDGET,
				XmNleftWidget,  _workArea->_sar[i].scaletext,
				XmNrightAttachment, XmATTACH_FORM,
				XmNtopAttachment, 	XmATTACH_FORM,
				0);

		XtManageChild (_tform);

		XtAddCallback (_workArea->_sar[i].toggle, 
				XmNvalueChangedCallback, &SarList::toggleCB, 
				this);
		XtFree (tmp);
	}
	XmScrolledWindowSetAreas (_sw, NULL, NULL, _rc);

	/*******************************************************
	 * Scale label and scrolled window consisting of a list
	 * of usable scales. These are selected in combination
	 * with the List items from the first list.
	 *******************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_scale), charset);
	_scalelabel = XtVaCreateManagedWidget ("scalelabel", 
					xmLabelWidgetClass, _w, 
					XmNleftAttachment, XmATTACH_POSITION,
					XmNleftPosition,   60,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition,  74,
					XmNtopAttachment, XmATTACH_FORM,
					XmNlabelString, xmstr, 
				 	0);
	XmStringFree (xmstr);

	ac = 0;
	XtSetArg (al[ac],XmNlistSizePolicy, XmCONSTANT); ac++;
	_scalesw = XmCreateScrolledList(_w, "scalesw", al, ac);
	XtManageChild (_scalesw);

	/*******************************************************
	 * The compound Strings  containing the list of all items
	 * is stored here
	 *******************************************************/
	xmstring = (XmString *) XtMalloc  (sizeof (XmString) 
			* XtNumber (scale_string));
	for ( i = 0; i < XtNumber (scale_string) ; i++) {
		scale_string[i] =  I18n::GetStr (scale_string[i]);
		xmstring[i] = XmStringCreateLtoR (scale_string[i], charset);
	}
	
	XtVaSetValues (_scalesw,
			XmNscrollingPolicy, XmAUTOMATIC,
			XmNscrollBarDisplayPolicy, XmAS_NEEDED,
			XmNitems, 	xmstring,	
			XmNitemCount,	XtNumber (scale_string),
			XmNvisibleItemCount, XtNumber (scale_string),
			0);
	for ( i = 0; i < XtNumber (scale_string) ; i++)
		XmStringFree (xmstring[i]);

	XtVaSetValues (XtParent(_scalesw),
			XmNtopAttachment,   XmATTACH_WIDGET,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition,  74,
			XmNtopWidget,       _scalelabel,
			XmNleftAttachment,  XmATTACH_POSITION,
			XmNleftPosition,    60,
			XmNbottomAttachment, XmATTACH_FORM,
			0);
	XtAddCallback (_scalesw, XmNbrowseSelectionCallback, 
			&SarList::SelectCB, this);
	
	/*******************************************************
	 * COLOR scrolled window - contains a list of all colors 
	 * that the display h/w is capable of supporting
	 * The colors are loaded in the background 
	 *******************************************************/
	xmstr = XmStringCreateLtoR (I18n::GetStr (TXT_wait), charset);
	_colorlabel = XtVaCreateManagedWidget ("colorlabel", 
					xmLabelWidgetClass, _w, 
					XmNalignment,  XmALIGNMENT_BEGINNING,
					XmNrightAttachment, XmATTACH_FORM,
					XmNtopAttachment, XmATTACH_FORM,
					XmNleftAttachment, XmATTACH_POSITION,
					XmNleftPosition,  75,
					XmNlabelString,	xmstr, 
				 	0);
	XmStringFree (xmstr);

	_colorsw = XtVaCreateManagedWidget ("colorsw", 
					xmScrolledWindowWidgetClass, _w,
					XmNscrollingPolicy, XmAUTOMATIC,
					XmNrightAttachment, XmATTACH_FORM,
					XmNtopAttachment,   XmATTACH_WIDGET,
					XmNtopWidget,       _colorlabel,
					XmNleftAttachment,  XmATTACH_POSITION,
					XmNleftPosition,    75,
					XmNbottomAttachment, XmATTACH_FORM,
					0);

	_colorrc = XtVaCreateManagedWidget( "colorrc", 
					xmRowColumnWidgetClass, _colorsw,
					XmNradioBehavior,	True,
					0);

	colorList (this, NULL);
	XmScrolledWindowSetAreas (_colorsw, NULL, NULL, _colorrc);
} 


SarList::~SarList() 
{
	delete _scalebuf;
	delete _workArea;
	delete _graph_area;
}

/*********************************************************
	Timeout callback - called periodically till all
	Widget		_pane;
	colors on the screen are shown on the screen.  	 
	It does sixteen colors at a time.
********************************************************/
void SarList::colorList (XtPointer client_data, XtIntervalId *)
{
	SarList *obj = (SarList *) client_data;
	
	XmString		xmstr;
	int 			i, limit;
	static int		index = 0;
	static XtIntervalId	procid;

	/*********************************************************
	 * create the buttons to do all colors 
	 * set the label strings, the mnemonics and the callbacks
	 ********************************************************/
	if (index >= theApplication->ScreenColors()) {
		XtRemoveTimeOut (procid);
	}
	else {
		limit = index + 16;

		/*********************************************************
		 * If the limit of colors on the screen has been reached 
	 	 * then reset the clock and take the wait message out
	 	 ********************************************************/
		if (limit >= theApplication->ScreenColors()) {
			limit =  theApplication->ScreenColors();
			xmstr = XmStringCreateLtoR (I18n::GetStr 
						(TXT_colorlist), charset);
			XtVaSetValues (obj->_colorlabel, 
					XmNalignment,  XmALIGNMENT_CENTER,
					XmNlabelString,  xmstr,
					0);
			XmStringFree (xmstr);
		}
					
		/*********************************************************
		 * If the limit has been not been reached then add the 
		 * color widgets to the scrolled window. Setup the callbacks
		 * and call the timeout procedure again. For the first set
		 * up the clock cursor 
	 	 ********************************************************/
		for (i = index; i < limit; i++) {
    			obj->_optionbuttons = XtVaCreateManagedWidget (
				"optionbuttons",
				xmToggleButtonWidgetClass, obj->_colorrc, 
				XmNforeground,  theApplication->ColorEntry(i),	
				XmNbackground,  theApplication->ColorEntry(i),	
				XmNuserData,  	theApplication->ColorEntry(i),	
				0);
			XtAddCallback (obj->_optionbuttons, 
					XmNvalueChangedCallback, obj->colorCB, 
					obj);
			index++;
		}
		if (index < theApplication->ScreenColors()) 
			procid = XtAppAddTimeOut (theApplication->appContext (),
				250, (XtTimerCallbackProc) &SarList::colorList,
				 obj);
	}
}

void SarList::toggleCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	SarList *obj = (SarList *) client_data;
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)
						call_data; 

	obj->toggle (w, cb);
}

void SarList::toggle (Widget w, XmToggleButtonCallbackStruct *cb)
{
	XtVaGetValues (w, XmNuserData, &_toggle_num, 0);
	if (cb->set) {
		_toggle_set = True;
		_workArea->_sar[_toggle_num].selected = True;
		
	}
	else {
		_toggle_set = False;
		_workArea->_sar[_toggle_num].selected = False;
		_workArea->ResetSarToggle (_toggle_num);
	}

	/* De-sensitize or sensitize alarm button depending on whether
	 * the alarm can be set or not
	 */
	_workArea->SetAlarmButton();

	/* Expose the graph so they can pick up newly selected
	 * sar options
	 */
	XClearArea(XtDisplay (_graph_area->baseWidget()),
		XtWindow (_graph_area->baseWidget()), 0, 0, 0, 0, True);
}

void SarList::colorCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	SarList *obj = (SarList *) client_data;
	XmToggleButtonCallbackStruct *cb = (XmToggleButtonCallbackStruct *)
						call_data; 

	obj->color(w, cb);
}

void SarList::color(Widget w, XmToggleButtonCallbackStruct *cb)
{
	int  	color;

	XtVaGetValues (w, XmNuserData, &color, 0);

	if (_toggle_set) {
		
		if (_workArea->_sar[_toggle_num].color != color)  
			_workArea->_sar[_toggle_num].color = color;

		if (_workArea->_sar[_toggle_num].foreground != color)  
			_workArea->_sar[_toggle_num].foreground = color;

		XtVaSetValues(_workArea->_sar[_toggle_num].toggle,
				XmNforeground, color,
				0);
		XtVaSetValues(_workArea->_sar[_toggle_num].scaletext,
				XmNforeground, color,
				0);

		_workArea->_sar[_toggle_num].gc = _workArea->GetColorGC(w,
								color);
		XClearArea(XtDisplay (_graph_area->baseWidget()),
					XtWindow (_graph_area->baseWidget()),
					0, 0, 0, 0, True);
	}
}

void SarList::SelectCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	SarList *obj = (SarList *) client_data;
	XmListCallbackStruct *cb = (XmListCallbackStruct *) call_data; 

	obj->select(w, cb);
}

void SarList::select(Widget w, XmListCallbackStruct *cb)
{
	if (_toggle_set && _workArea->_sar[_toggle_num].def_scale == 100) {
		theErrDialogMgr->postDialog (w, I18n::GetStr (TXT_scaleerr), 
					I18n::GetStr (TXT_percerror));
		theErrDialogMgr->setModal();
	}
	else {
		char 		*tmp;
		XmString	xmstr;

		/* If the item was selected then reset the scale text
		 * in the row to the one selected
		 */
		if (_toggle_set) {
			tmp = XtMalloc (strlen (_scalebuf) + strlen 
				(scale_string[cb->item_position - 1]));
			strcpy (tmp, _scalebuf);
			strcat (tmp, scale_string[cb->item_position - 1]);
			xmstr = XmStringCreateLtoR (tmp,charset);
			XtVaSetValues (_workArea->_sar[_toggle_num].scaletext,
					XmNlabelString,	xmstr, 
					0);	
			XmStringFree (xmstr);
			_workArea->_sar[_toggle_num].scale = 
				atoi (scale_string[cb->item_position - 1]);
			XtFree (tmp);
			XClearArea(XtDisplay (_graph_area->baseWidget()),
			XtWindow (_graph_area->baseWidget()), 0, 0, 0, 0, True);
		} 
	}
}
