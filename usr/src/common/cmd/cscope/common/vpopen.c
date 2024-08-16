/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/vpopen.c	1.2"

/* vpopen - view path version of the open system call */

#include <stdio.h>
#include <fcntl.h>
#include "vp.h"

#define READ	0

vpopen(path, oflag)
char	*path;
int	oflag;
{
	char	buf[MAXPATH + 1];
	int	returncode;
	int	i;

	if ((returncode = myopen(path, oflag, 0666)) == -1 && path[0] != '/' &&
	    oflag == READ) {
		vpinit((char *) 0);
		for (i = 1; i < vpndirs; i++) {
			(void) sprintf(buf, "%s/%s", vpdirs[i], path);
			if ((returncode = myopen(buf, oflag, 0666)) != -1) {
				break;
			}
		}
	}
	return(returncode);
}
