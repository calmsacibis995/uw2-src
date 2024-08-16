/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ibrow:ibrowlib.c	1.3"

#include <Dt/Desktop.h>
#include "../dtamlib/owner.h"	/* for OWN_PACKAGE */

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

void
DisplayHelp(Widget widget, HelpText * help)
{
 	DtRequest	request;

        memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	if (help) {
                request.display_help.source_type =
			help->section ? DT_SECTION_HELP : DT_TOC_HELP;
		request.display_help.app_name = "Get Inet Browser";
		request.display_help.app_title = "Get Internet Browser";
		request.display_help.title = "Get Internet Browser";
		request.display_help.help_dir = NULL;
		request.display_help.file_name = "ibrow/browser.hlp";
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
ibrow_help(Widget wid)
{
	static HelpText phelp;
	phelp.section = strdup("10");
        DisplayHelp(wid, &phelp);
	return(0);

}

/* 
 * have to reverse return from _DtamIsOwner, since we want 
 * 0 -> success, !0 ->failure.  this ensures that if libibrow.so
 * is missing, install/uninstall will not be sensitive.
 */
int
ok_to_install()
{
	return !_DtamIsOwner(OWN_PACKAGE);
}
