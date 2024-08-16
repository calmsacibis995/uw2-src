/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/***
 *
 *  name	IS_lib.c - C function for Install_Server wksh script
 *		@(#)inetinst:IS_lib.c	1.2	8/22/94
 *
 ***/

#ident	"@(#)inetinst:IS_lib.c	1.2"

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/RowColumn.h>
#include <Xm/MwmUtil.h>

#include "lookup.h"
#include "lookupG.h"

#include <Dt/Desktop.h>

#include <unistd.h>

static	Widget statusWid;

void
DisplayHelp(Widget widget, HelpText * help)
{

	DtRequest	request;

	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	if (help) {
		request.display_help.source_type =
			help->section ? DT_SECTION_HELP : DT_TOC_HELP;
		request.display_help.app_name = "Install_Server";
		request.display_help.app_title = "Network Install Server Setup";
		request.display_help.title = "Network Install Server Setup";
		request.display_help.title = gettxt("iserver:13", "Network Install Server Setup") ;
		request.display_help.help_dir = NULL;
		request.display_help.file_name = "Install_Server/Install_Server.hlp";
		request.display_help.sect_tag = help->section;
	} else {
		request.display_help.source_type = DT_OPEN_HELPDESK;
	}
	(void)DtEnqueueRequest(
		XtScreen(widget),
		_HELP_QUEUE(XtDisplay(widget)),
		XInternAtom(
			XtDisplay(widget),
			"_DT_QUEUE",
			False
		),
		XtWindow(widget),
		&request
	);
}

int
access_help( Widget wid )
{
static HelpText ahelp;

	ahelp.section = strdup("10");

	DisplayHelp(wid, &ahelp);
	return(0);	
}

int
load_help( Widget wid )
{
static HelpText lhelp;

	lhelp.section = strdup("20");

	DisplayHelp(wid, &lhelp);
	return(0);	
}

int
hdesk_help( Widget wid )
{
	DisplayHelp(wid, NULL) ;
	return(0);
}

int
copy_help(Widget wid)
{
static HelpText chelp;

	chelp.section = strdup("70");

	DisplayHelp(wid, &chelp);
	return(0);	
}

int
getlen(Widget wid)
{
	XtWidgetGeometry	size;
	size.request_mode = CWWidth;
	XtQueryGeometry(wid, NULL, &size);
	return(size.width);	
}

setFocus(Widget wid, int iterations)
{
	int	i;
	/*altprintf("iterations is %d\n", iterations); */
	for (i=0; i < iterations; i++) {
		XmProcessTraversal(wid, XmTRAVERSE_NEXT_TAB_GROUP);
	} 
	return(0);
}
