/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/optfgfns.c	1.6"

/* 
 * 	XPG4 Feature Group Optional Functionality not implemented.
 *
 * 	POSIX 1003.2 C Language Bindings
 * 	OPTIONAL FEATURE GROUP
 * 	Not implemented: wordexp, wordfree
 *
 * 	ENHANCED I18N 
 * 	OPTIONAL FEATURE GROUP
 * 	Not implemented: strfmon
 *
 *	ASSUMPTIONS:
 * 	Error returns where required are defined as -1.
 * 	wordexp(): WRDE_NOSYS
 *
 *   NOTE: This implementation assumes NO function arguments
 */

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

#ifdef	__STDC__
			/* set errno to ENOSYS, return -1 */
	#pragma	weak	wordexp = _wordexp
	#pragma	weak	wordfree = _wordfree

	#pragma	weak	strfmon = _wordexp

	#pragma	weak	_strfmon = _wordexp
	
#endif

/* set errno to ENOSYS and returns -1 */

int
_wordexp()
{
	errno=ENOSYS;
	return (-1); 
}

/* set errno to ENOSYS and returns void */
void 
_wordfree()
{
	errno=ENOSYS;
	return; /* void*/
}
