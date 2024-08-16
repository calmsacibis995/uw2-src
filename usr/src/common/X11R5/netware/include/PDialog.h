/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// PDialog.h:
/////////////////////////////////////////////////////////
#ifndef PDIALOG_H
#define PDIALOG_H
#include "PDialog.h"
#include "Dialog.h"
#include <Xm/SelectioB.h> 

class PDialog : public Dialog{
    
private:

	static void cancelCB (Widget, XtPointer, XtPointer);
	static void helpCB (Widget, XtPointer, XtPointer);
	static void okCB (Widget, XtPointer, XtPointer);
	
protected:

	virtual void 	ok (XmSelectionBoxCallbackStruct *) = 0;
	virtual void 	cancel () = 0;
	virtual void 	help (Widget) = 0;
	virtual void	postPDialog (Widget, char *, char *, char *);

public:
    
    	PDialog (char *);
};

#endif
