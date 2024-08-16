/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// WarnDialog.h:
/////////////////////////////////////////////////////////
#ifndef WARNDIALOG_H 
#define  WARNDIALOG_H
#include <Xm/Xm.h>
#include "Dialog.h"

class WarnDialog : public Dialog {
    
private:
	static void	okCB (Widget, XtPointer, XtPointer);
    
protected:

	virtual void	ok (Widget);

public:
    
    	WarnDialog (char *);
    	virtual void 	postDialog ( Widget, char *, char * );
};

extern WarnDialog *theWarnDialogMgr;

#endif

