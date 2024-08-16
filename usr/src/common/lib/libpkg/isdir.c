/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/isdir.c	1.3.5.4"
#ident "$Header: $"

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

extern int	sprintf();

isdir(path)
char *path;
{
	struct stat statbuf;

	if (path != NULL)
		if(!stat(path, &statbuf) && ((statbuf.st_mode & S_IFMT) == S_IFDIR))
			return(0);
	return(1);
}

ispipe(path)
char *path;
{
	struct stat statbuf;

	if (path != NULL)
		if(!stat(path, &statbuf) && ((statbuf.st_mode & S_IFMT) == S_IFIFO))
			return(0);
	return(1);
}

isfile(dir, file)
char *dir;
char *file;
{
	struct stat statbuf;
	char	path[PATH_MAX];

	if(dir != NULL) {
		if (file != NULL)
			(void) sprintf(path, "%s/%s", dir,file);
		else
			(void) sprintf(path, "%s", dir);
		file = path;
	}

	if ( file != NULL )
		if(!stat(file, &statbuf) && (statbuf.st_mode & S_IFREG))
			return(0);
	return(1);
}
