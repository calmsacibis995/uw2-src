/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:filter/postscript/common/tempnam.c	1.1.5.3"
#ident	"$Header: $"
#include <stdio.h>
#include <errno.h>

#if defined(V9) || defined(BSD4_2)
char *tempnam(dir, pfx)
char *dir, *pfx;
{
	int pid;
	unsigned int len;
	char *tnm, *malloc();
	static int seq = 0;

	pid = getpid();
	len = strlen(dir) + strlen(pfx) + 10;
	if ((tnm = malloc(len)) != NULL) {
		sprintf(tnm, "%s", dir);
		if (access(tnm, 7) == -1)
			return(NULL);
		do {
			sprintf(tnm, "%s/%s%d%d", dir, pfx, pid, seq++);
			errno = 0;
			if (access(tnm, 7) == -1)
				if (errno == ENOENT)
					return(tnm);
		} while (1);
	}
	return(tnm);
}
#endif
