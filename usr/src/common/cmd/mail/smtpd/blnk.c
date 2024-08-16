/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/blnk.c	1.1"
#include	<stdio.h>
#include	<mail/list.h>

#include	"smtp.h"

smtpServerState_t
    blnkFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	char
	    buffer[512];
	
	(void) sprintf
	    (
	    buffer,
	    "502 %s is unimplemented",
	    name
	    );
	
	smtpdConnSend(connData_p, buffer);
	return(state);
	}

smtpServerState_t
    blnkError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}

smtpServerState_t
    unrcFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	char
	    buffer[512];
	
	(void) sprintf
	    (
	    buffer,
	    "502 %s is unrecognized",
	    name
	    );
	
	smtpdConnSend(connData_p, buffer);
	return(state);
	}
