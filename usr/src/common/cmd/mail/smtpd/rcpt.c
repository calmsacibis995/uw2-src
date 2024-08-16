/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/rcpt.c	1.1"
#include	<stdio.h>
#include	<mail/list.h>

#include	"smtp.h"

smtpServerState_t
    rcptFunc(char *cmd, smtpServerState_t state, void *connData_p, list_t *args)
	{
	message_t
	    *msg_p;

	char
	    buffer[512],
	    *name;
	
	if((msg_p = connectionMessage(connData_p)) == NULL)
	    {
	    (void) sprintf(buffer, "503 Bad sequence of commands.");
	    }
	else if(listGetNext(args, &name))
	    {
	    (void) sprintf(buffer, "501 No recipient specified.");
	    }
	else if(messageRecipientAdd(msg_p, name))
	    {
	    (void) sprintf(buffer, "452 Out of memory");
	    }
	else
	    {
	    (void) sprintf(buffer, "250 %s...Recipient OK.", name);
	    state = sss_rcpt;
	    }

	smtpdConnSend(connData_p, buffer);
	return(state);
	}

smtpServerState_t
    rcptError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}
