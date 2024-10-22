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

#ident	"@(#)libmail:libmail/check4mld.c	1.1"
#ident	"@(#)libmail:libmail/check4mld.c	1.1"
#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
#endif /* SVR4_1 */
#include "lpriv.h"

/*
    NAME
	check4mld - check directory name to see if it is an MLD

    SYNOPSIS
	int check4mld(const char *dir)

    DESCRIPTION
	Tests the given directory to see if it is an MLD. It goes
	into real mode and uses tstmld().

    RETURNS
	 0 if an MLD
	 1 if not an MLD or package is not installed
	-1 on error
*/

#ifndef SVR4_1
/* ARGSUSED */
#endif
int check4mld(dir)
const char *dir;
{
#ifdef SVR4_1
    struct stat statbuf;

    if (stat(dir, &statbuf) == -1)
	return -1;

    /* if /var/mail is a MLD and mldmode() works, return MLD */
    if ((S_ISMLD & statbuf.st_flags) && (mldmode(MLD_QUERY) != -1))
        return 0;
    return 1;
#else /* SVR4_1 */
    return 1;
#endif /* SVR4_1 */
}
