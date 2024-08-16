/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/installf/removef.c	1.8.6.11"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include "install.h"

#define MALSIZ	64
#define ERR_MEMORY	":6:memory allocation failure, errno=%d"
#define ERR_RELPATH	":43:ERROR:relative pathname <%s> ignored"
#define ERR_NOPATH      ":768:no pathname specified"

extern struct cfent 
		**eptlist;
extern int	errno,
		eptnum,
		warnflag;
extern void	*calloc(),
		*realloc();
extern void	progerr(),
		logerr(),
		quit(),
		canonize(),
		free(),
		qsort();
extern int	cfentcmp();
extern char	*pathdup();

void
removef(argc, argv)
int argc;
char *argv[];
{
	struct cfent *new;
	char	line[PATH_MAX];
	char	*path;
	int	flag;

	flag = !strcmp(argv[0], "-");

	/* read stdin to obtain entries, which need to be sorted */
	eptlist = (struct cfent **) calloc(MALSIZ, 
		sizeof(struct cfent *));

	eptnum = 0;
	new = NULL;
	for(;;) {
		if(flag) {
			if(fgets(line, PATH_MAX, stdin) != NULL){
				if ((path = strtok(line, "\n")) == NULL)
					continue;
			}
			else
				break;
		} else {
			if(argc-- <= 0)
				break;
			path = argv[argc];
		}
		canonize(path);
		if(path[0] != '/') {
			logerr(ERR_RELPATH, path);
			if(new)
				free(new);
			warnflag++;
			continue;
		}
		new = (struct cfent *) calloc(1, sizeof(struct cfent));
		if(new == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
		new->ftype = '-';
		new->path = pathdup(path);
		eptlist[eptnum] = new;
		if((++eptnum % MALSIZ) == 0) { 
			eptlist = (struct cfent **) realloc((void *)eptlist, 
			   (unsigned) (sizeof(struct cfent)*(eptnum+MALSIZ)));
			if(!eptlist) {
				progerr(ERR_MEMORY, errno);
				quit(99);
			}
		}
	}
	if (!eptnum){
		progerr(ERR_NOPATH);
		quit(1);
	}
	eptlist[eptnum] = (struct cfent *)NULL;

	qsort((char *)eptlist, 
		(unsigned)eptnum, sizeof(struct cfent *), cfentcmp);
}
