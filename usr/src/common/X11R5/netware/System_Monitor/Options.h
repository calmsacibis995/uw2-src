/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:Options.h	1.2"
/////////////////////////////////////////////////////////
// Options.h:
/////////////////////////////////////////////////////////
#ifndef OPTIONS_H 
#define  OPTIONS_H

#include <Xm/Xm.h>

class WorkArea;

class Options {
    
private:
    
	int		_which_proc, _incr, _range;
	Widget		_w, _timerc, _arrowdown, _interval, _arrowup, _seconds,
			_gridrc, _form, _actionbuttons[3], _timevalue, 	
			_vertical, _horizontal, _pane, _control, _pulldown, 
			_proc_menu, _procbutton[32], _proc_button;
	WorkArea	*_workarea;
	XtIntervalId	_arrow_timer_id;
	Boolean		_horizontal_set, _vertical_set, _proc_set;

	static void	okCB (Widget, XtPointer, XtPointer);
	static void	helpCB (Widget, XtPointer, XtPointer);
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	destroyCB (Widget, XtPointer, XtPointer);
	static void	StartStopCB(Widget, XtPointer, XtPointer);
	static void	HorizontalCB(Widget, XtPointer, XtPointer);
	static void	VerticalCB(Widget, XtPointer, XtPointer);
	static void	ProcCB (Widget, XtPointer, XtPointer);
	static void	ValueCB (Widget, XtPointer, XtPointer);

	static void	change_value(XtPointer, XtIntervalId );

protected:

	virtual void	ok (Widget);
	virtual void	help (Widget);
	virtual void	cancel (Widget);
	virtual void	destroy ();
	virtual void	start_stop(Widget, XmArrowButtonCallbackStruct *);
	virtual void	GetTimeValue ();
	virtual void	horizontal ();
	virtual void	vertical ();
	virtual void	processor(Widget);
	virtual void	value (Widget);

public:
    
    	Options ();
    	virtual ~Options ();
    	virtual void postDialog ( Widget, char *, WorkArea *);
    	void 	SetProc();
	Widget	baseWidget() const { return _w; }
};

extern Options *theOptionsMgr;

#endif
