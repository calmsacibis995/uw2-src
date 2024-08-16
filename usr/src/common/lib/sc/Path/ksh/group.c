/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident "@(#)sc:Path/ksh/group.c	3.1" */
#include <stdio.h>

main()
{
	system("groups");
	info();
	setegid(4);
	info();
	setrgid(22);
	info();
	setrgid(13);
	info();
}

info()
{
	int groups[10];
	register int n;
	printf ("real gid is %d, effective gid is %d, groups are:", getgid(), getegid());
	n = getgroups(10,groups);
	while(--n >= 0)
		printf ("\t%d", groups[n]);
	printf ("\n");
}
