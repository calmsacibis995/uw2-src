/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtupgrade:dtgetusers.c	1.1"

/*********************************************************************
 *
 * dtgetusers - return all users along with their home directories (if exists)
 *	from the system, the output format is:
 *
 *		login-id-1:login-dir-1
 *		login-id-2:login-dir-2
 *		...
 *	`login-dir-?' can be NULL if opendir() is failed.
 */

#include <stdio.h>
#include <pwd.h>
#include <dirent.h>

main()
{
        struct passwd *	pwd;
	DIR *		dirp;

        while (pwd = getpwent())
	{
			/* Use `:' as delimiter... */
		if ((dirp = opendir(pwd->pw_dir)) != NULL)
			printf("%s:%s\n", pwd->pw_name, pwd->pw_dir);
		else
			printf("%s:\n", pwd->pw_name);
	}
}
