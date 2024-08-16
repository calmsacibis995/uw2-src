/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/readdir.c	1.15"
/*
	readdir -- C library extension routine

*/

#ifdef __STDC__
	#pragma weak readdir = _readdir
	#pragma weak readdir_r = _readdir_r
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<dirent.h>
#include	<string.h>
#include	"dirm.h"	/* includes stdlock.h */

#define NEXTDP	 ((struct dirent *) &((struct dirplus *)dirp)->u.buf[dirp->dd_loc])

struct dirent *
#ifdef __STDC__
readdir_r(DIR *dirp, struct dirent *result)
#else
readdir_r(dirp, result) DIR *dirp; struct dirent *result;
#endif
{
	register struct dirent	*dp;	/* -> directory data */
	int saveloc = 0;

	STDLOCK(&((struct dirplus *)dirp)->dirlock);
	if (dirp->dd_size != 0) {
		dp = NEXTDP;
		saveloc = dirp->dd_loc;   /* save for possible EOF */
		dirp->dd_loc += dp->d_reclen;
	}
	if (dirp->dd_loc >= dirp->dd_size)
		dirp->dd_loc = dirp->dd_size = 0;

	if (dirp->dd_size == 0 	/* refill buffer */
	  && (dirp->dd_size = getdents(dirp->dd_fd,
			&((struct dirplus *)dirp)->u.align, DIRBUF)
	     ) <= 0
	   ) {
		if (dirp->dd_size == 0)	/* This means EOF */
			dirp->dd_loc = saveloc;  /* EOF so save for telldir */
		dp = NULL;	/* error or EOF */
		goto out;
	}
	dp = NEXTDP;
	if (result != 0)
	{
		memmove(result, dp, dp->d_reclen);
		dp = result;
	}
out:;
	STDUNLOCK(&((struct dirplus *)dirp)->dirlock);
	return(dp);
}
	
struct dirent *
#ifdef __STDC__
readdir(DIR *dirp)
#else
readdir(dirp) DIR  *dirp;	/* stream from opendir() */
#endif
{
	return (readdir_r(dirp, (struct dirent *)0));
}
