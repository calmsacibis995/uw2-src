/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/backup.c	1.3.5.3"
#ident  "$Header: backup.c 1.2 91/06/27 $"
#include <stdio.h>

#include <pfmt.h>

extern char	savlog[];
extern int	warnflag;

extern void	logerr();

void
backup(path, mode)
char	*path;
int	mode;
{
	static int	count = 0;
	static FILE	*fp;

	/* mode probably used in the future */
	if(count++ == 0) {
		if((fp = fopen(savlog, "w")) == NULL) {
			logerr(":278:WARNING:unable to open logfile <%s>", savlog);
			warnflag++;
		}
	}

	if(fp == NULL)
		return;

	(void) fprintf(fp, "%s%s", path, mode ? "\n" : gettxt(":279", " <attributes only>\n"));
	/* we don't really back anything up; we just log the pathname */
}
