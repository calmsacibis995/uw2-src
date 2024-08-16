/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

///////////////////////////////////////////////////////// 
// QDialog.h:
/////////////////////////////////////////////////////////
#ifndef QDIALOG_H 
#define  QDIALOG_H
#include <Xm/Xm.h>
#include "Dialog.h"

class QDialog : public Dialog {
    
private:
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	okCB (Widget, XtPointer, XtPointer);
    
protected:

	virtual void	cancel () = 0;
	virtual void	ok () = 0;

public:
    
    	QDialog (char *);
    	virtual void 	postDialog ( Widget, char *, char * );
};

#endif

