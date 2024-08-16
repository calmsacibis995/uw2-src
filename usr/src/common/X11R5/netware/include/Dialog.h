/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/Dialog.h	1.2"
/////////////////////////////////////////////////////////
// Dialog.h:
/////////////////////////////////////////////////////////
#ifndef DIALOG_H 
#define  DIALOG_H
#include <Xm/Xm.h>
#include "BasicComponent.h"
#include "mnem.h"

static const int INFORMATION = 0;
static const int PROMPT = 1;
static const int WARNING = 2;
static const int QUESTION = 3;
static const int WORKING = 4;
static const int ERROR = 5;

class Dialog : public BasicComponent {
    
private:
    
	Widget parentWidget;
	unsigned int	_i, _ac;
	Arg		_al[20];
	Widget		_dialog_form;
	Widget		okWidget;
	Widget		cancelWidget;
	Widget		helpWidget;

	static void	destroyCB (Widget, XtPointer, XtPointer);

protected:

	/* the type determines which type of dialog to post
	 * Set it before calling postDialog in the derived
	 * class
	 */
	int		_type;

	/* destroy the callback when XtDestroyWidget is called
	 * from destroyCB
	 */
	virtual void	destroy ();

	virtual void	manage ();

	/* Register callbacks for OK, HELP AND CANCEL
	 * if needed in your derived class. Pass any
	 * parameters with the function. 
	 */
	virtual void 	registerOkCallback (XtCallbackProc, XtPointer);
	virtual void 	registerCancelCallback (XtCallbackProc, XtPointer);
	virtual void 	registerHelpCallback (XtCallbackProc, XtPointer);

	/* If some of the buttons in the message dialog are not
	 * needed then unmanage them
	 */
	virtual void 	unmanageOk ();
	virtual void 	unmanageCancel ();
	virtual void 	unmanageHelp ();

	/* set the label strings for the OK, CANCEL AND HELP
	 * if you want them to be anything other than ok
	 * cancel help
	 */
	virtual void 	setOkString (char *,char * mnem = NULL);
	virtual void 	setHelpString (char *,char * mnem = NULL);
	virtual void 	setCancelString (char *,char * mnem = NULL);
	
	/* Post the dialog to the screen. In this derived
	 * function, determine the type so the appropriate
	 * message dialog can be posted and also unmanage
	 * the requisite buttons and register your callbacks
	 */
    	virtual	void 	postDialog ( Widget, char *, char *);

public:
    
    	Dialog (char *);
    	virtual ~Dialog ();
};

#endif

