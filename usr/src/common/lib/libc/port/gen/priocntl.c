/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/priocntl.c	1.2"

#ifdef __STDC__
	#pragma weak priocntl = __priocntl
#endif

#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>

long
__priocntl(pc_version, idtype, id, cmd, arg)
int		pc_version;
idtype_t	idtype;
id_t		id;
int		cmd;
caddr_t		arg;
{
	procset_t	procset;

	setprocset(&procset, POP_AND, idtype, id, P_ALL, 0);

	return(__priocntlset(pc_version, &procset, cmd, arg));
}
