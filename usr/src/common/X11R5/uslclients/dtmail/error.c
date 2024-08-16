/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:error.c	1.4"
#endif

#define ERROR_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "mail.h"
#include <Gizmo/Gizmos.h>
#include <locale.h>

extern int _sys_num_err;
/*
 * Given the errno return a printable message in some language.
 */

char *
GetTextGivenErrno (errno)
int	errno;
{
	char	buf[10];

	/*
	 * Return error message errno from catalog UX.
	 */
	sprintf (buf, "UX:%d", errno+1);
	return (char *)gettxt (buf, strerror (errno));
}

/*
 * Given an error message produced by perror return a printable message in
 * some language.
 */

char *
GetTextGivenText (perror)
char *	perror;
{
	char *		cp;
	int		i;

	/*
	 * Look though all of the system errors for this error message
	 */

	for (i=0; (cp=strerror(i))!=NULL && i < _sys_num_err; i++) {
		if (strncmp (perror, cp, strlen(cp)) == 0) {
			/*
			 * String found
			 */
			return GetTextGivenErrno (i);
		}
	}

	/* In case we can't find the message, look for "empty file" */
	/* This fix should be move into mail.c, where the test can be */
	/* done at a more appropriate location (i.e. when receiving the */
	/* string "empty file" from mailx).  However, in this stage of */
	/* the product (near beta release), the least risky fix is here */
	/* as a special case. */

	if (i >= _sys_num_err && !strcmp (perror, NTS_EMPTY_FILE))
		return (GetGizmoText(TXT_EMPTY_FILE));

	return NULL;
}
