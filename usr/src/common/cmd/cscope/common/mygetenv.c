/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/mygetenv.c	1.1"
/* return the non-null environment value or the default argument */

char	*
mygetenv(variable, deflt)
char	*variable, *deflt;
{
	char	*value;
	char	*getenv();

	value = getenv(variable);
	if (value == (char *) 0 || *value == '\0') {
		return(deflt);
	}
	return(value);
}
