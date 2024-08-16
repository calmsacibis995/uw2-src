/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:MenuBar.h	1.2"
/////////////////////////////////////////////////////////
// MenuBar.h:
/////////////////////////////////////////////////////////
#ifndef MENUBAR_H
#define MENUBAR_H
#include "BasicComponent.h"
#include <iostream.h>
#include <fstream.h>

#define		LOGQ		1
#define		STOPLOGQ	2

class WorkArea;

class MenuBar : public BasicComponent {
    
private:
    
    	Widget 		_FilePullDown,_helpbutton[3],_HelpPullDown, _helpw;
	Widget		_action_button[6], _view_button[2], _ViewPullDown;
	WorkArea	*_workArea;
	ofstream	_logFile;
	char		*_filename;

	static void	ActionsCB (Widget, XtPointer, XtPointer);
	static void	HelpCB (Widget, XtPointer, XtPointer);

protected:

	virtual void 	actions (Widget);

public:
    
    	MenuBar ( Widget, char * , WorkArea *);
    	virtual ~MenuBar ();
	int	WriteLogFile (char *);	
	void  	OpenLogFile (int);
	void 	LogData(char *);
	void 	help (Widget); 
	void 	StopLoggingFile();
	Widget  MenuAlarmWidget () const { return _view_button[0]; }
};
#endif
