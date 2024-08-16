/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:eaccess.c	2.1.2.2"
/*	Determine if the effective user id has the appropriate permission
	on a file.  
*/

#ifdef __STDC__
	#pragma weak eaccess = _eaccess
#endif
#include "synonyms.h"
#include <unistd.h>

extern int	access();


int
eaccess( path, amode )
const char		*path;
register int	amode;
{
	/* Use effective id bits */
	return access(path, EFF_ONLY_OK|amode); 
}
