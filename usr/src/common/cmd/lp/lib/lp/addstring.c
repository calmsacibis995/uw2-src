/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lp/addstring.c	1.3.6.3"
#ident	"$Header: $"

#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** addstring() - ADD ONE STRING TO ANOTHER, ALLOCATING SPACE AS NEEDED
 **/

int
#if	defined(__STDC__)
addstring (
	char **			dst,
	char *			src
)
#else
addstring (dst, src)
	char			**dst;
	char			*src;
#endif
{
	size_t			len;

	if (!dst || !src) {
		errno = EINVAL;
		return (-1);
	}

	len = strlen(src) + 1;
    
	if (*dst) {
		if (!(*dst = Realloc(*dst, strlen(*dst) + len))) {
			errno = ENOMEM;
			return (-1);
		}
	} else {
		if (!(*dst = Malloc(len))) {
			errno = ENOMEM;
			return (-1);
		}
		(*dst)[0] = '\0';
	}

	(void) strcat(*dst, src);
	return (0);
}
