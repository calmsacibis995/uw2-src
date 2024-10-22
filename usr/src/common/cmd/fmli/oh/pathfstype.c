/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)fmli:oh/pathfstype.c	1.2.3.3"

/*
 *	This function returns the identifier of the filesystem that
 *	the path arguement resides on.  If any errors occur, it
 *	return s5 as a default.
 */

#include "inc.types.h"		/* abs s14 */
#include <sys/statfs.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>

static char fs_buf[FSTYPSZ];
static char fs_default[]="s5";

char *
path_to_fstype(path)
char *path;
{
	struct statfs stat_buf;

	if ( statfs(path,&stat_buf,sizeof(struct statfs),0) ) {
		return(fs_default);
	}

	if ( sysfs(GETFSTYP,stat_buf.f_fstyp,fs_buf) ) {
		return(fs_default);
	}

	return(fs_buf);
}
