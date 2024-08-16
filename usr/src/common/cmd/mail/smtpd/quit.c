/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/quit.c	1.1"
#include	<stdio.h>
#include	<mail/list.h>

#include	"smtp.h"

smtpServerState_t
    quitFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	char
	    buffer[512];
	
	(void) sprintf
	    (
	    buffer,
	    "221 %s closing connection",
	    SystemName
	    );
	
	smtpdConnSend(connData_p, buffer);
	smtpdConnTerminate(connData_p);
	return(sss_quit);
	}

smtpServerState_t
    quitError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}
