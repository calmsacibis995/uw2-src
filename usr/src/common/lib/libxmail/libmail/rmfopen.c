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

#ident	"@(#)libmail:libmail/rmfopen.c	1.1"
#ident	"@(#)libmail:libmail/rmfopen.c	1.1"
#include "stdc.h"
#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
#endif /* SVR4_1 */
#include "lpriv.h"

/*
    NAME
	realmode_fopen - open a file in real mode

    SYNOPSIS
	FILE *realmode_fopen(const char *dir, const char *mode)

    DESCRIPTION
	Realmode_fopen opens the given file in real mode, using
	mldmode(MLD_REAL).

*/

FILE *realmode_fopen(f, mode)
const char *f;
const char *mode;
{
#ifdef SVR4_1
    FILE *fp;
    int sverrno;
    if (mldmode(MLD_REAL) == -1)
	errexit(2, errno, ":356:Cannot set real mode!\n");
    fp = fopen(f, mode);
    sverrno = errno;
    if (mldmode(MLD_VIRT) == -1)
	errexit(2, errno, ":348:Cannot set virtual mode!\n");
    errno = sverrno;
    return fp;
#else /* SVR4_1 */
    return fopen(f, mode);
#endif /* SVR4_1 */
}
