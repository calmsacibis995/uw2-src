/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/open_tc.c	1.13.5.2"
#ident  "$Header: open_tc.c 1.2 91/06/21 $"

#include	<limits.h> 	/* get PATH_MAX from here, not stdio */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<backup.h>
#include	<fcntl.h>
#include	<method.h>
#include	<signal.h>
#include	<string.h>
#include	<errno.h>

extern int	br_toc_open();
extern int	br_toc_write();
extern int	brlog();      
extern int	getpid();
extern int	strfind();

extern int	bklevels;

int
backup_toc(mp, tname)
m_info_t	*mp;
char		*tname;
{
	int	toc_f;
	char	buffer[21];
	char	*ftmp;
	char	*fname1;
	int	i;
	struct stat	st;

	(void) sprintf(buffer, "%s", mp->jobid);

	toc_f = br_toc_open(1, buffer, tname);

	if (toc_f < 0 ) {
		sprintf(ME(mp), "Job ID %s: -T cannot open toc", mp->jobid);
		return(-1);
	}
	(void) sprintf(buffer,"PATHLENGTH=%d",(strlen(mp->tocfname)) + 1);
	i = br_toc_write(0, NULL, NULL, -1, buffer);

	if (i = br_toc_write(1, NULL, mp->tocfname, 1, 0)) {
		brlog("error writing tmp table of contents %s",SE);
		return(-1);
	}
	return(toc_f);
} /* backup_toc() */
