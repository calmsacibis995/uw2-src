/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/rmopendir.c	1.1"
#ident	"@(#)libmail:libmail/rmopendir.c	1.1"
#include "stdc.h"
#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
#endif /* SVR4_1 */
#include "lpriv.h"

/*
    NAME
	realmode_opendir - open a directory in real mode

    SYNOPSIS
	DIR *realmode_opendir(const char *dir)

    DESCRIPTION
	Realmode_opendir opens the given directory in real mode, using
	mldmode(MLD_REAL).

    RETURNS
	Returns a pointer to the directory, or NULL on error.
*/

DIR *realmode_opendir(dir)
const char *dir;
{
#ifdef SVR4_1
    DIR *dirp;
    int sverrno;
    if (mldmode(MLD_REAL) == -1)
	errexit(2, errno, ":356:Cannot set real mode!\n");
    dirp = opendir(dir);
    sverrno = errno;
    if (mldmode(MLD_VIRT) == -1)
	errexit(2, errno, ":348:Cannot set virtual mode!\n");
    if (!dirp)
	errexit(2, errno, ":2:Cannot open %s: %s\n", dir, Strerror(errno));
    errno = sverrno;
    return dirp;
#else /* SVR4_1 */
    return opendir(dir);
#endif /* SVR4_1 */
}
