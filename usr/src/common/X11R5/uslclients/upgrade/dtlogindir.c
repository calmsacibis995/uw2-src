/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtupgrade:dtlogindir.c	1.1"

/******************************file*header********************************
 *
 * dtuser_home - identify home directory from given user(s).
 */

#include <stdio.h>
#include <pwd.h>
#include <dirent.h>

int
main(int argc, char ** argv)
{
	register int	i;
	struct passwd *	stuff;
	int		exit_code = 0;
	DIR *		dirp;

	for (i = 1; i < argc; i++)
	{
		if (i != 1)
			printf(" ");

		if ((stuff = getpwnam(argv[i])) != NULL &&
		    (dirp = opendir(stuff->pw_dir)) != NULL)
		{
			printf("%s", stuff->pw_dir);
			closedir(dirp);
		}
		else
		{
			printf("NO-HOME");
			exit_code = 1;
		}
		fflush(stdout);
	}
	return(exit_code);
} /* end of main */
