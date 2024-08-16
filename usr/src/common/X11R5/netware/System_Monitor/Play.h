/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:Play.h	1.2"
/////////////////////////////////////////////////////////
// Play.h:
/////////////////////////////////////////////////////////
#ifndef PLAY_H 
#define PLAY_H

#include <Xm/Xm.h>
#include <stdio.h>
#include "main.h" 

class WorkArea;

typedef struct {
	double	*value;
	int  	*y;
	Boolean	draw;
} PlayData;

class Play {
    
private:
    
	Widget		_w, _buttonrc, _daterc, _pane, _interval, _actionform,
			_lastupdate, _date, _canvas, _toggle, _scale,
			_actionbuttons[2], _action, _time, _shellForm, 
			_speed, _intervallabel, _datestring, _rc, _sw;
	WorkArea	*_workarea;
	int		_playwidth, _playheight, _maxx, _maxy, _total, 
			_scaleval, _lineno, _previous, _lastvalue;
	double		_lasty, _mark;
	XtIntervalId	_timeid;
	GC		_gc;
	PlayData	_playdata[SAR_TOTAL];
	Boolean		_stopflag, _rewind;
	XmString	*_timestr, *_intstr, *_datestr;
	
	static void	SliderCB (Widget, XtPointer, XtPointer);
	static void	helpCB (Widget, XtPointer, XtPointer);
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	destroyCB (Widget, XtPointer, XtPointer);
	static void	ExposeCB(Widget, XtPointer, XtPointer);
	static void	ResizeCB(Widget, XtPointer, XtPointer);
	static void	toggleCB (Widget, XtPointer, XtPointer);
	static void	actionCB (Widget, XtPointer, XtPointer);

	static void 	playback (XtPointer, XtIntervalId *); 

protected:

	virtual void	help (Widget);
	virtual void	cancel ();
	virtual void	destroy ();
	virtual void	resize ();
	virtual void	expose ();
	virtual void	action(Widget);
	virtual void	toggle (Widget, XmToggleButtonCallbackStruct *);
	virtual void	slider (XmScaleCallbackStruct *);
	virtual void	DrawXandYAxis (Widget); 
	virtual void	DrawLine (int, int, int, Boolean); 
	virtual void	GetLine (int, char *); 
	virtual void	SetTimeDateInterval(int); 
	virtual void	DrawAllLines(int,int); 

public:
    
    	Play ();
    	virtual ~Play ();
    	virtual void postDialog (Widget, char *, char *, WorkArea *);
};

extern Play *thePlayManager;

#endif


