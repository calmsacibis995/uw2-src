/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


///////////////////////////////////////////////////////////////
// ButtonItem.h: 
///////////////////////////////////////////////////////////////
#ifndef BUTTONITEM_H
#define BUTTONITEM_H
#include <Xm/Xm.h>


typedef void (*CreateProc) (Widget, XtPointer, XtPointer);

typedef struct {
	XtArgVal 	_label;
	XtArgVal	_mnem;
	XtArgVal	_userdata;
	XtArgVal	_sensitive;
	XtArgVal	_proc_ptr;
} ButtonItem;


class Buttons {
	
private:
	
public:
	static void SetLabels (ButtonItem *, int);
};

#endif
	
