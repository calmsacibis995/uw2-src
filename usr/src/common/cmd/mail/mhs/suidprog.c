/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mhs/suidprog.c	1.4"
#include	<stdio.h>
#include	<string.h>

int
    main(int argc, char **argv)
	{
	char
	    *basename,
	    cmdbuf[256];
	
	int
	    id;
	
	id = geteuid();
	if(setuid(id))
	    {
	    perror("setuid");
	    }
	
	id = getegid();
	if(setgid(id))
	    {
	    perror("setgid");
	    }
	
	basename = strrchr(argv[0], '/');
	sprintf
	    (
	    cmdbuf,
	    "/usr/lib/mail/surrcmd/suid/%s",
	    (basename == NULL)? argv[0]: &basename[1]
	    );

	execv(cmdbuf, argv);
	}
