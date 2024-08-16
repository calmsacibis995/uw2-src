/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libcmd/common/Shell.h	1.2"

#ifndef	Shell_h
#define	Shell_h

#include <sys/types.h>

class PtyInfo;

// Invoke the shell to run a UNIX system command
extern	pid_t	Shell( char * cmd, int redir, PtyInfo *& );

// for termination message ---
extern	char *	sig_message[];

#endif	/* Shell_h */
