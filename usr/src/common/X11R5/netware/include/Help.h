/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/Help.h	1.1"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/include/Help.h,v 1.1 1994/01/24 19:38:02 renu Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
///////////////////////////////////////////////////////////////
// HelpComponent.h: 
///////////////////////////////////////////////////////////////
#ifndef HELP_H
#define HELP_H

#include <Xm/Xm.h>

typedef struct {
	char *file;
	char *title;
	char *section;
	char *apptitle;
} HelpText;

class Help {
    
  private:  

  public:
	Help();
	virtual ~Help ();
	void	DisplayHelp (Widget, HelpText *);
	void	SetHelpLabels (HelpText *);
};

extern Help *theHelpManager;

#endif

	
