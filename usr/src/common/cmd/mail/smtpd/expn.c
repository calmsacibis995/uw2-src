/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/expn.c	1.1"
#include	<stdio.h>
#include	<string.h>
#include	<malloc.h>
#include	<mail/list.h>
#include	<mail/link.h>

#include	"smtp.h"

void
    *maildir_alias();

smtpServerState_t
    expnFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	void
	    *aliasList,
	    *curLink_p,
	    *nextLink_p;

	char
	    *curLine_p,
	    *nextLine_p,
	    *mailName,
	    buffer[512];
	
	if(listGetNext(args, &mailName))
	    {
	    /* No Name */
	    }
	else if((aliasList = maildir_alias(strtok(mailName, "<>"), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else
	    {
	    for
		(
		curLine_p = (char *)linkOwner(curLink_p = linkNext(aliasList)),
		    nextLine_p = (char *)linkOwner
			(
			nextLink_p = linkNext(curLink_p)
			),
		    linkFree(curLink_p);
		curLine_p != NULL;
		free(curLine_p),
		    curLine_p = nextLine_p,
		    curLink_p = nextLink_p,
		    nextLine_p = (char *)linkOwner
			(
			nextLink_p = linkNext(curLink_p)
			),
		    linkFree(curLink_p)
		)
		{
		(void) strcpy(buffer, (nextLine_p == NULL)? "250 ": "250-");
		(void) strncat(buffer, curLine_p, sizeof(buffer));
		smtpdConnSend(connData_p, buffer);
		}
	    }

	return(state);
	}

smtpServerState_t
    expnError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}
