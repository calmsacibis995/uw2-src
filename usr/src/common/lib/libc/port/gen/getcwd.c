/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getcwd.c	1.33"


/*LINTLIBRARY*/

#ifdef __STDC__
	#pragma weak getcwd = _getcwd
#endif
#include 	"synonyms.h"
#include        <sys/types.h>
#include        <sys/stat.h>
#include        <mac.h>
#include        <errno.h>
#include        <dirent.h>
#include        <string.h>
#include        <stdlib.h>

#define MAX_PATH 1024
#define MAX_NAME 512
#define BUF_SIZE 1536 /* 3/2 of MAX_PATH for /a/a/a... case */

/* 
 * This algorithm does not use chdir.  Instead, it opens a 
 * successive strings for parent directories, i.e. .., ../..,
 * ../../.., and so forth.
 */

char *
#ifdef __STDC__
getcwd(char *str, size_t size)
#else
getcwd(str, size)
char *str;
size_t size;
#endif
{
	char dotdots[BUF_SIZE+MAX_NAME];
	struct stat		cdir;	/* current directory status */
	struct stat		tdir;
	struct stat		pdir;	/* parent directory status */
	DIR			*pdfd;	/* parent directory stream */

	struct dirent *dir;
	char *dotdot = dotdots + BUF_SIZE - 3;
	char *dotend = dotdots + BUF_SIZE - 1; 
	int i, maxpwd, alloc, saverr, ret; 
	int mld_mode = MLD_REAL;
	
	if((int) size <= 0) {
		errno = EINVAL;
		return NULL;
	}
	
	if(stat(".", &pdir) < 0)
		return NULL;
	
	alloc = 0;
	if(str == NULL)  {
		if((str = (char *)malloc(size)) == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		alloc = 1;
	}
	
	/*
	 * Save the MLD mode on entry and change to real MLD mode, if necessary.
	 */
	if ((mld_mode = mldmode(MLD_QUERY)) == MLD_VIRT)
		mldmode(MLD_REAL);
	
	*dotdot = '.';
	*(dotdot+1) = '.';
	*(dotdot+2) = '\0';
	maxpwd = size--;
	str[size] = 0;

	for(;;)
	{
		/* update current directory */
		cdir = pdir;

		/* open parent directory */
		if ((pdfd = opendir(dotdot)) == 0)
			break;

		if(fstat(pdfd->dd_fd, &pdir) < 0)
		{
			saverr = errno;
			(void)closedir(pdfd);
			errno = saverr;
			break;
		}

		/*
		 * find subdirectory of parent that matches current 
		 * directory
		 */
		if(cdir.st_dev == pdir.st_dev)
		{
			if(cdir.st_ino == pdir.st_ino)
			{
				/* at root */
				(void)closedir(pdfd);
				if (size == (maxpwd - 1))  /* pwd is '/' */
				{
					if ((int)size <= 0)
					{
						errno = ERANGE;
						goto out;
					}
					else
						str[--size] = '/';
				}

				strcpy(str, &str[size]);
				if (mld_mode == MLD_VIRT)
					mldmode(MLD_VIRT);
				return str;
			}
			do
			{
				if ((dir = readdir(pdfd)) == 0)
				{
					saverr = errno;
					(void)closedir(pdfd);
					errno = saverr;
					goto out;
				}
			}
			while (dir->d_ino != cdir.st_ino);
		}
		else
		{
			/*
			 * must determine filenames of subdirectories
			 * and do stats
			 */
			*dotend = '/';
			do
			{
				if ((dir = readdir(pdfd)) == 0)
				{
					saverr = errno;
					(void)closedir(pdfd);
					errno = saverr;
					goto out;
				}
				strcpy(dotend + 1, dir->d_name);
				/* skip over non-stat'able
				 * entries
				 */
				ret = stat(dotdot, &tdir);
				
			}		
			while(ret == -1 || tdir.st_ino != cdir.st_ino || tdir.st_dev != cdir.st_dev);
		}
		i = strlen(dir->d_name);

		if (i > (int)size - 1) {
			(void)closedir(pdfd);
			errno = ERANGE;
			break;
		} else {
			/*
			 * We copy the name of the current directory into the
			 * pathname under either of 2 conditions:
			 * (1) The parent directory isn't an MLD,
			 * (2) We were in real MLD mode when this function
			 *     was invoked.
			 * This prevents the name of an effective directory
			 * from appearing in the resulting string if there
			 * is an MLD in the path and we're in virtual MLD mode.
			 */
			if ( !(S_ISMLD & pdir.st_flags) || mld_mode == MLD_REAL) {
				size -= i;
				strncpy(&str[size], dir->d_name, i);
				str[--size] = '/';
			}
		}
		(void)closedir(pdfd);
		if (dotdot - 3 < dotdots) 
			break;
		/* update dotdot to parent directory */
		*--dotdot = '/';
		*--dotdot = '.';
		*--dotdot = '.';
		*dotend = '\0';
	}
out:
	if(alloc)
		free(str);
	/*
	 * Restore MLD mode, if necessary
	 */
	if (mld_mode == MLD_VIRT)
		mldmode(MLD_VIRT);
	return NULL;
}

