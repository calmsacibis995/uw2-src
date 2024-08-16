/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:IconBox.h	1.3"
/////////////////////////////////////////////////////////
// IconBox.h:
/////////////////////////////////////////////////////////
#ifndef ICONBOX_H
#define ICONBOX_H
#include "BasicComponent.h"

typedef void (*CreateProc) (Widget, XtPointer, XtPointer);
typedef void (*ArmProc) (Widget, XtPointer, XEvent *, Boolean *);

typedef struct _icon_items {
	Pixmap 		_pixmap;
	CreateProc 	_func_ptr;
	ArmProc		_event_ptr;
} IconItem ;


class WorkArea;

class IconBox : public BasicComponent {
    
private:
    
	unsigned int	_which_button;
    	Widget 		_icon_button[3],_msg_area, _proc_msg_area;
	WorkArea	*_workArea;
	IconItem	_PixmapItems[3];

	static void	optionsCB (Widget, XtPointer, XtPointer);
	static void	alarmCB (Widget, XtPointer, XtPointer);
	static void	playbackCB (Widget, XtPointer, XtPointer);

	static void	optionsarmCB(Widget, XtPointer, XEvent *, Boolean *);
	static void	alarmarmCB (Widget, XtPointer, XEvent *, Boolean *);
	static void	playbackarmCB (Widget, XtPointer, XEvent *, Boolean *);

protected:

	virtual void 	optionsarm (int);
	virtual void 	alarmarm (int);
	virtual void 	playbackarm (int);

	virtual void 	options ();
	virtual void 	alarm ();
	virtual void 	playback ();
	virtual void 	SetMsgArea(char *);

public:
    
    	IconBox ( Widget, char * , WorkArea *);
    	virtual ~IconBox ();
	Widget	ProcLabelWidget () const { return _proc_msg_area; }
	Widget	IconAlarmWidget () const { return _icon_button[1]; }
};
#endif
