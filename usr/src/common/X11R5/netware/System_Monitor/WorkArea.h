/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:WorkArea.h	1.4"
/////////////////////////////////////////////////////////
// WorkArea.h:
/////////////////////////////////////////////////////////
#ifndef WORKAREA_H
#define WORKAREA_H
#include "BasicComponent.h"
#include "MenuBar.h"

class SarList;
class Graph;
class IconBox;
class Alarm;
class MenuBar;
class Play;

typedef struct {
	char 		*title;
	char 		*scale;
} SarDetail;

typedef struct {
	char 		*title;
	GC 		gc;
	double		value[1280];
	Boolean		is_alarm;
	int		x[1280], y[1280], selected, color, def_scale, scale;
	int		 alarm_over, alarm_under, beep, flash;
	Widget		toggle, scaletext, alarmicon;
	int		background, foreground;
	Boolean		flashed;
} SarTable;

class WorkArea : public BasicComponent {
    
	friend  SarList;
	friend  Graph;
	friend  Alarm;
	friend  Play;

private:
    
	Boolean		_draw_HZ, _draw_VT, _log;
	IconBox		*_iconbox;
	MenuBar	 	*_menubar;
	SarList	 	*_list;
	Graph		*_graph;
	int		_timer, _cpus, _which_cpu;
	SarTable	_sar[20];
	char		*_logfilename, *_savefilename, *_alarmfilename,
			*_homedir;

	static 	void 	HelpCB (Widget, XtPointer, XtPointer);

protected:

	virtual GC		GetColorGC (Widget, int);
	virtual GC		GetBlackGC (Widget);
	virtual GC		GetClearGC (Widget);
	virtual GC		SetForegroundGC (Widget, int);
	virtual void 		WriteFile (char *s) 
					{_menubar->WriteLogFile(s);}
	virtual void		SetSarTable (Widget);
	virtual void		ResetSarToggle (int);
	virtual void		help (Widget);
	virtual void		SetAlarmButton ();

public:
    
    	WorkArea 	( Widget, char * );
    	virtual 	~WorkArea ();

	/* These functions manipulate the graph area in the 
	 * drawing area widget
	 */

	void		SetTimer (int i) { _timer = i; }
	int		GetTimer () const  { return _timer; }
	int		NoCpus () const  { return _cpus; }
	void		SetCpu (int i) { _which_cpu = i; }
	int		GetCpu () { return _which_cpu; }
	void		ClearGraph ();
	void		PlotGraph ();
	void		RemovePlotTimeout();
	void		HorizontalLines (Boolean);
	void		VerticalLines (Boolean);
	Boolean		DrawVT () const { return _draw_VT; }
	Boolean		DrawHZ () const { return _draw_HZ; }

	void		SetLog (Boolean i) { _log = i; }
	char 		*GetLogFileName()  const { return _logfilename; }
	void		SaveSettings (Widget);
	void		CreatePixmap (Pixmap *, char *, long);
	char		*HomeDirectory () const { return _homedir; }
	void		IfLog (Widget) ;
	void		SetProcLabel () ;
};

#endif
