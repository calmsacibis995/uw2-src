/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/setlist.c	1.3.7.5"
#ident  "$Header: setlist.c 1.2 91/06/27 $"

#include <stdio.h>
#include <string.h>

extern int	errno;
extern void	*calloc(), 
		*realloc();
extern void	progerr(),
		quit();

#define MALSIZ	64
#define ERR_MEMORY	"uxpkgtools:6:memory allocation failure, errno=%d"

int
setlist(plist, value)
char ***plist;
char *value;
{
	char	**list;
	char	*pt;
	int	n;

	if ((list = (char **)calloc(MALSIZ, sizeof(char *))) == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}

	n = 0;
	pt = strtok(value, " \t\n");
	while (pt) {
		list[n++] = pt;
		if ((n % MALSIZ) == 0) {
			list = (char **)realloc((void *)list, 
				(n + MALSIZ) * sizeof(char *));
			if (list == NULL) {
				progerr(ERR_MEMORY, errno);
				quit(99);
			}
		}
		pt = strtok(NULL, " \t\n");
	}
	list[n] = NULL;
	*plist = list;
	return(n);
}
