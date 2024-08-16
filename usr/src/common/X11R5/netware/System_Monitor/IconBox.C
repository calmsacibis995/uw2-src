#ident	"@(#)systemmon:IconBox.C	1.4"
////////////////////////////////////////////////////////////////////
// IconBox.C: Holds the icon buttons for the menu actions 
/////////////////////////////////////////////////////////////////////
#include "Application.h"
#include "main.h"
#include "IconBox.h"
#include "WorkArea.h"
#include "Options.h"
#include "Select.h"
#include "Alarm.h"
#include "i18n.h"
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////
//	These are the pretty icons used in the bar for each
//	functionality of SYSTEM_MONITOR.
////////////////////////////////////////////////////////////////////
static char *pixmaps [] = { 
	 "/usr/X/lib/pixmaps/options24.icon",
	 "/usr/X/lib/pixmaps/setalarm24.icon",
	 "/usr/X/lib/pixmaps/playback24.icon",
};

////////////////////////////////////////////////////////////////////
//	IconBox widget class:	contains the bar for holding all 
//	the buttons. The message area is tagged at the end.
////////////////////////////////////////////////////////////////////
IconBox::IconBox (Widget parent, char *name, WorkArea *workarea) 
			: BasicComponent (name)
{
	XtWidgetGeometry		size;
	int				argCnt, i, maxwidth;
	Arg				args[20];

	_workArea = workarea;	
	
	/* Create rowcolumn for pixmaps 
	 */
    	_w = XtVaCreateWidget ( _name, xmRowColumnWidgetClass, parent,
				XmNorientation, XmHORIZONTAL,
				0);

	/* set the function ptr to the respective callbacks
	 */
	_PixmapItems[0]._func_ptr = IconBox::optionsCB;
	_PixmapItems[1]._func_ptr = IconBox::alarmCB;
	_PixmapItems[2]._func_ptr = IconBox::playbackCB;

	/* set the event function  ptr to the respective arm procedures
	 * to change the message text when the mouse moves over the 
	 * button 
	 */
	_PixmapItems[0]._event_ptr = IconBox::optionsarmCB;
	_PixmapItems[1]._event_ptr = IconBox::alarmarmCB;
	_PixmapItems[2]._event_ptr = IconBox::playbackarmCB;
	
	/*    Create buttons.	*/
	for (i = 0; i < 3; i++) { 

		_workArea->CreatePixmap (&_PixmapItems[i]._pixmap, pixmaps[i], 
					theApplication->Background());
		_icon_button[i] = XtVaCreateManagedWidget( "_icon_button", 
			xmPushButtonWidgetClass,_w, 
			XmNlabelType,		XmPIXMAP,
			XmNlabelPixmap,		_PixmapItems[i]._pixmap,
			0);
		XtAddEventHandler (_icon_button[i], 
				EnterWindowMask|LeaveWindowMask, False, 
				(XtEventHandler) _PixmapItems[i]._event_ptr,
				(XtPointer)this);

		XtAddCallback (_icon_button[i], XmNactivateCallback, 
				(XtCallbackProc) _PixmapItems[i]._func_ptr,
				(XtPointer)this);
	}

	maxwidth = strlen (I18n::GetStr (TXT_PlaybackCtrl));
   	argCnt = 0;
   	XtSetArg( args[argCnt], XmNeditable, False ); argCnt++;
   	XtSetArg( args[argCnt], XmNmaxLength, maxwidth ); argCnt++;
   	XtSetArg( args[argCnt], XmNscrollHorizontal, False ); argCnt++;
   	XtSetArg( args[argCnt], XmNcursorPositionVisible, False ); argCnt++;
   	XtSetArg( args[argCnt], XmNwordWrap, True ); argCnt++;
   	_msg_area = XmCreateScrolledText( _w, "PromptText", args, argCnt ); 
	XtManageChild (_msg_area);

	_proc_msg_area = XtVaCreateManagedWidget ("proc_msg_area",
					xmLabelWidgetClass,	_w,
					0);
	
	size.request_mode = CWHeight;

	XtQueryGeometry (_w, NULL, &size);
	XtVaSetValues (_w,		/* set max. and min. size for pane */
			XmNpaneMinimum, size.height,
			XmNpaneMaximum, size.height,
			0);
}

IconBox::~IconBox()
{
	int i;

	_workArea = NULL;
	for (i = 0; i < 6; i++)  {
		XmDestroyPixmap (theApplication->screen(), 
				_PixmapItems[i]._pixmap);
	}
}

void IconBox::optionsCB (Widget, XtPointer clientData, XtPointer)
{
	IconBox *obj	=  (IconBox *) clientData;

	obj->options();
}


void IconBox::options()
{
	theOptionsMgr->postDialog (_w, "Options", _workArea);
}

////////////////////////////////////////////////////////////////////
//	The Program is finito!
////////////////////////////////////////////////////////////////////
void IconBox::playbackCB (Widget, XtPointer clientData, XtPointer)
{
	IconBox *obj	=  (IconBox *) clientData;

	obj->playback ();
}

void IconBox::playback()
{
	theSelectManager->postDialog (_w,  "Select", _workArea);
}

////////////////////////////////////////////////////////////////////
//	Alarm Callback - When the Alarm button is pushed 
//	call the ALarmmanager to popup the setup menu 
////////////////////////////////////////////////////////////////////
void IconBox::alarmCB (Widget, XtPointer clientData, XtPointer)
{
	IconBox *obj	=  (IconBox *) clientData;
	
	obj->alarm ();
}

void IconBox::alarm()
{
	theAlarmManager->postDialog (_w, "Alarms", _workArea);
}

void IconBox::optionsarmCB (Widget, XtPointer clientData, XEvent *event, 
				Boolean *)
{
	IconBox *obj	=  (IconBox *) clientData;

	obj->optionsarm (event->type);
}

void IconBox::optionsarm(int type)
{
	if (type == EnterNotify)
		SetMsgArea(TXT_OptionsCtrl);
	else
		SetMsgArea("");
}

void IconBox::playbackarmCB (Widget, XtPointer clientData, XEvent *event, 
				Boolean *)
{
	IconBox *obj	=  (IconBox *) clientData;

	obj->playbackarm (event->type);
}

void IconBox::playbackarm (int type)
{
	if (type == EnterNotify)
		SetMsgArea(TXT_PlaybackCtrl);
	else
		SetMsgArea("");
}

void IconBox::alarmarmCB (Widget, XtPointer clientData, XEvent *event,
			 Boolean *)
{
	IconBox *obj	=  (IconBox *) clientData;

	obj->alarmarm (event->type);
}

////////////////////////////////////////////////////////////////////
//		Invoke alarm set up menu
////////////////////////////////////////////////////////////////////
void IconBox::alarmarm (int type)
{
	if (type == EnterNotify)
		SetMsgArea(TXT_AlarmCtrl);
	else
		SetMsgArea("");
}

/*********************************************************************
	Set the message area - Set the label string to be a value
	reflecting the icon that the mouse is moved over. 
*********************************************************************/
void IconBox::SetMsgArea (char *msg) 
{
	XtVaSetValues (_msg_area, 
			XmNvalue, 	strlen (msg) == 0 ? "" : I18n::GetStr(msg),
			0);
}
