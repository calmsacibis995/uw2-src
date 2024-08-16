/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// Select.h:
/////////////////////////////////////////////////////////
#ifndef SELECT_H 
#define SELECT_H

#include <Xm/Xm.h>

class WorkArea;

class Select {
    
private:
    
	Widget		_selection;
	WorkArea	*_workarea;

	static void	helpCB (Widget, XtPointer, XtPointer);
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	destroyCB (Widget, XtPointer, XtPointer);
	static void	okCB (Widget, XtPointer, XtPointer);

protected:

	virtual void	help (Widget);
	virtual void	ok (XmFileSelectionBoxCallbackStruct *);
	virtual void	cancel ();
	virtual void	destroy ();

public:
    
    	Select ();
    	virtual ~Select ();
    	virtual void postDialog ( Widget, char *, WorkArea *);
};

extern Select *theSelectManager;

#endif
