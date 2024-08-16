#ident	"@(#)libclass:Help.C	1.4 libclass:Help.c"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libclasses/Help.C,v 1.4 1994/07/11 22:07:15 rv Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
///////////////////////////////////////////////////////////
// Help.C: 
// The Desktop Help routine that sends a request to the dtm
// to display help
///////////////////////////////////////////////////////////
#include <Dt/Desktop.h>
#include "Help.h"
#include "i18n.h"
#include <stdio.h> 

Help	*theHelpManager = new Help ();

Help::Help()
{
}

Help::~Help()
{
}

void Help::DisplayHelp (Widget widget, HelpText *help)
{
    	DtRequest		*req;
    	DtDisplayHelpRequest	displayHelpReq;
    	Display			*display = XtDisplay (widget);
	Widget			shell = XtParent (widget);
    	Window			win;

	while (!XtIsShell (shell))
		shell = XtParent (shell);

	win  = XtWindow (shell);

	memset(&displayHelpReq,0,sizeof(displayHelpReq));
	req = (DtRequest *) &displayHelpReq;
    	displayHelpReq.rqtype = DT_DISPLAY_HELP;
    	displayHelpReq.serial = 0;
    	displayHelpReq.version = 1;
    	displayHelpReq.client = win;
    	displayHelpReq.nodename = NULL;

	if (help) {
		displayHelpReq.source_type = help->section ? 
					DT_SECTION_HELP : DT_TOC_HELP;

		displayHelpReq.app_name = help->apptitle;
		displayHelpReq.app_title = help->apptitle;
		displayHelpReq.title = help->title;
		displayHelpReq.help_dir = NULL;
		displayHelpReq.file_name = help->file;
		displayHelpReq.sect_tag = help->section;
	}
	else {
		displayHelpReq.source_type = DT_OPEN_HELPDESK;
	}

    	(void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   	_HELP_QUEUE (display), win, req);

}	/* End of DisplayHelp () */

/* SetHelpLabels
 *
 * Set strings for help text.
 */
void Help::SetHelpLabels (HelpText *help)
{
	help->title = I18n::GetStr (help->title);
/*
	#####Help section not to be internationalized
	if (help->section) 
		help->section = I18n::GetStr (help->section);
	else
		help->section =  NULL;
*/
	help->apptitle = I18n::GetStr (help->apptitle);
	
}	/* End of SetHelpLabels */
