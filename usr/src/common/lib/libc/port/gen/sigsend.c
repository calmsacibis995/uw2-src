/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/sigsend.c	1.2"

#ifdef __STDC__
	#pragma weak sigsend = _sigsend
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/procset.h>
#include <signal.h>

sigsend(idtype, id, sig)
idtype_t idtype;
id_t	 id;
int	 sig;
{
	procset_t set;
	setprocset(&set, POP_AND, idtype, id, P_ALL, P_MYID);
	return sigsendset(&set, sig);
}
