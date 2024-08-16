/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/////////////////////////////////////////////////////////
// InfoDialog.h:
/////////////////////////////////////////////////////////
#ifndef INFODIALOG_H 
#define  INFODIALOG_H
#include <Xm/Xm.h>
#include "Dialog.h"

class InfoDialog : public Dialog {
    
private:
	static void	OkCB (Widget, XtPointer, XtPointer);
    
protected:

	virtual void	ok (Widget);

public:
    
    	InfoDialog (char *);
    	virtual void 	postDialog ( Widget, char *, char * );
};

extern InfoDialog *theInfoDialogMgr;

#endif

