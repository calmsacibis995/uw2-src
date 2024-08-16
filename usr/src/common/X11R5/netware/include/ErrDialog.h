/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/ErrDialog.h	1.2"
/////////////////////////////////////////////////////////
// ErrDialog.h:
/////////////////////////////////////////////////////////
#ifndef ERRDIALOG_H 
#define  ERRDIALOG_H
#include <Xm/Xm.h>
#include "Dialog.h"

class ErrDialog : public Dialog {
    
private:
	static void	cancelCB (Widget, XtPointer, XtPointer);
    
protected:

	virtual void	cancel (Widget);

public:
    
    	ErrDialog (char *);
    	virtual void 	postDialog ( Widget, char *, char * );
	void 		setModal();
};

extern ErrDialog *theErrDialogMgr;

#endif

