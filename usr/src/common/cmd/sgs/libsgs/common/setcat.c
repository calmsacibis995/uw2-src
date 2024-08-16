/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/setcat.c	1.2"

#ifdef __STDC__	
	#pragma weak setcat = _setcat
#endif
#include "synonyms.h"
#include <stdio.h>

/* setcat(cat): dummy function.  Returns NULL since there is no
 * message catalog in the cross environment
 */
/*ARGSUSED*/
const char *
setcat(cat)
const char *cat;
{
	return NULL;
}
