/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/raise.c	1.4"
/*LINTLIBRARY*/
#include <sys/types.h>
#include "synonyms.h"
#include <signal.h>
#include <unistd.h>


int
raise(sig)
int sig;
{
	return( kill(getpid(), sig) );
}
