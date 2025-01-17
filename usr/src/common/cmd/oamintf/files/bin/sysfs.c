/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/sysfs.c	1.1.4.2"
#ident  "$Header: sysfs.c 2.0 91/07/12 $"
#include <sys/fstyp.h>

extern int errno;

main()
{
	register int nfstyp = sysfs(GETNFSTYP);
	register int ifstyp;
	char fsname[FSTYPSZ];

	if (nfstyp < 1) {
		exit(errno);
	}
	for (ifstyp = 1; ifstyp <= nfstyp; ifstyp++) {
		if (sysfs(GETFSTYP, ifstyp, fsname) < 0) {
			exit(errno);
		}
		printf("%s\n", fsname );	
	}
	exit(0);
}
