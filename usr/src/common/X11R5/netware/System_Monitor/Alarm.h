/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// Alarm.h:
/////////////////////////////////////////////////////////
#ifndef ALARM_H 
#define ALARM_H

#include <Xm/Xm.h>

class WorkArea;

class Alarm {
    
private:
    
	Widget		_w, _alarmrc, _actionform, _pane, _listlabel, 
			_alarmlist, _abovetext, _abovelabel, _belowtext,  
			_belowlabel, _form, _actionbuttons[3], _sep, 
			_beep, _deletealarm, _setalarm, _flash;
	WorkArea	*_workarea;
	int		_list_position;
	Boolean		_selected;

	static void	okCB (Widget, XtPointer, XtPointer);
	static void	helpCB (Widget, XtPointer, XtPointer);
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	destroyCB (Widget, XtPointer, XtPointer);
	static void	ListCB(Widget, XtPointer, XtPointer);
	static void	DeleteAlarmCB(Widget, XtPointer, XtPointer);
	static void	SetAlarmCB(Widget, XtPointer, XtPointer);

protected:

	virtual void	ok (Widget);
	virtual void	help (Widget);
	virtual void	cancel ();
	virtual void	destroy ();
	virtual void	list (XmListCallbackStruct *);
	virtual void	deletealarm();
	virtual int	setalarm();
	virtual int	ErrorChecker(char *);

public:
    
    	Alarm ();
    	virtual ~Alarm ();
    	virtual void postDialog ( Widget, char *, WorkArea *);
};

extern Alarm *theAlarmManager;

#endif
