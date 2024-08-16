/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/help.c	1.1"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<mail/list.h>
#include	<mail/link.h>

#include	"smtp.h"

smtpServerState_t
    helpFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	void
	    *curLink_p,
	    *helpMsg_p;

	char
	    buffer[512],
	    *curLine_p,
	    *nextLine_p,
	    *cmdName;
	
	if(listGetNext(args, &cmdName))
	    {
	    /*
		General help, no specific command.
	    */
	    cmdName = strdup("HELP");
	    }

	if((helpMsg_p = cmdHelp(cmdName)) == NULL)
	    {
	    (void) sprintf(buffer, "504 No help for %s.", cmdName);
	    smtpdConnSend(connData_p, buffer);
	    free(cmdName);
	    }
	else
	    {
	    /*
		requested connamd cmdName.
	    */
	    for
		(
		curLine_p = (char *)linkOwner(curLink_p = linkNext(helpMsg_p)),
		    nextLine_p = (char *)linkOwner
			(
			curLink_p = linkNext(curLink_p)
			);
		curLine_p != NULL;
		curLine_p = nextLine_p,
		     nextLine_p = (char *)linkOwner
			(
			curLink_p = linkNext(curLink_p)
			)
		)
		{
		(void) strcpy(buffer, (nextLine_p == NULL)? "214 ": "214-");
		(void) strncat(buffer, curLine_p, sizeof(buffer));
		smtpdConnSend(connData_p, buffer);
		}

	    free(cmdName);
	    }

	return(state);
	}

smtpServerState_t
    helpError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}

