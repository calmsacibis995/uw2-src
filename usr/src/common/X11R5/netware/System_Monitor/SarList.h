/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// SarList.h:
/////////////////////////////////////////////////////////
#ifndef SARLIST_H
#define SARLIST_H
#include "BasicComponent.h"

class WorkArea;
class Graph;

class SarList : public BasicComponent {
	
private:
	WorkArea	*_workArea;
	Graph		*_graph_area;
	Widget		_sw, _colorsw, _rc, _colorrc, _optionbuttons, _tform;
	Widget		_colorlabel, _listlabel, _scalelabel;
	Widget		_scalesw;
	int	 	_toggle_set, _toggle_num;
	char		*_scalebuf;

	static void 	toggleCB (Widget, XtPointer, XtPointer);
	static void 	colorCB (Widget, XtPointer, XtPointer);
	static void 	SelectCB(Widget, XtPointer, XtPointer);
	static void 	colorList(XtPointer, XtIntervalId *);

protected:
	virtual void  	toggle (Widget, XmToggleButtonCallbackStruct *);
	virtual void  	color (Widget, XmToggleButtonCallbackStruct *);
	virtual void  	select(Widget, XmListCallbackStruct *);

public:
    
    	SarList ( Widget, WorkArea *, char *, Graph *);
	virtual	~SarList ();
};

#endif

