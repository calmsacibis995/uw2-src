/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:Graph.h	1.3"
/////////////////////////////////////////////////////////
// Graph.h:
/////////////////////////////////////////////////////////
#ifndef GRAPH_H
#define GRAPH_H
#include "BasicComponent.h"

class WorkArea;

class Graph : public BasicComponent {
private:
    
	int		_graph_height, _graph_width;
	WorkArea	*_workArea;
	int		_total, _maxx, _maxy, _ascent, _descent, _eachx;
	double		_eachy, _lasty, _mark;
	XtIntervalId	_procid;
	Boolean		_drew_number;
	GC		_namegc, _cleargc;
	XGCValues	_nameval;
	time_t		_clock;
	int		_fonttype;
	XtPointer	_fontptr;
	unsigned long  	_gcmask;
	
	static void	graph_resizeCB (Widget, XtPointer, XtPointer);
	static void	graph_exposeCB (Widget, XtPointer, XtPointer);
	static void	graph_inputCB (Widget, XtPointer, XtPointer);
	static void 	plot_graph (XtPointer, XtIntervalId *); 
	
protected:
	virtual void	graph_expose (XmDrawingAreaCallbackStruct *) ;
	virtual void	graph_resize ();
	virtual void	graph_input (XEvent *);
	virtual void	graph_buttonpress (XEvent *);
	virtual void	DrawTimeString (); 
	virtual void	DrawUpdateTime(Boolean); 
	virtual void	DrawLine (int, int, int); 
	virtual void 	SetAlarm (double,int); 
	virtual void 	MovePointsOver(); 
	virtual void	GetFontType (XmFontList);
	virtual void	DrawYValues(char *, int);

public:
    
    	Graph ( Widget, WorkArea *, char * );
	int		Width () const { return _graph_width; }
	int		Height () const { return _graph_height; }
	int		YOrigin() const { return _maxy; }
	void 		RemoveTimeOut(); 
	void 		ResetTimeOut(); 
	void		DrawXandYAxis (); 
};

#endif

