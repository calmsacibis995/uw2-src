/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_application.h	1.4"
/*----------------------------------------------------------------------------
 *	ps_application.h
 *
 *	This file is the definition file for the Application class.
 */
#ifndef APPLICATION_H
#define APPLICATION_H

#include <X11/StringDefs.h>
#include <X11/Xlib.h>

#include "BasicComponent.h"

/*----------------------------------------------------------------------------
 *
 */
typedef struct { 
	char*						dfltPrinter;
} resStruct, *res;

static XtResource				moreResources[] = {
	{	"defaultPrinter",
		"DefaultPrinter",
		XtRString,
		sizeof (String),
    	XtOffset (res, dfltPrinter),
		XtRString,
		NULL },
};

/*----------------------------------------------------------------------------
 *
 */
class Application : public BasicComponent {
public:
								Application (int, char**, char*);

private:
	XtAppContext				d_appContext;
	resStruct					d_resData;	

public:
	void 						RealizeLoop (DispInfo* di);
	inline XtAppContext 		appContext ();
	inline char*				GetDfltPrinter ();
};

/*----------------------------------------------------------------------------
 *
 */
XtAppContext
Application::appContext ()
{
	return (d_appContext);
}

char*
Application::GetDfltPrinter ()
{
	return (d_resData.dfltPrinter);
}

#endif		// APPLICATION_H
