/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/gettxt.c	1.3"

/*	gettxt()
 *	returns the default message
 */
#ifdef __STDC__
	#pragma weak gettxt = _gettxt
#endif
#include "synonyms.h"

/*ARGSUSED*/
char *
gettxt(msgid, dflt)
const char *msgid, *dflt;
{
	return((char *)dflt);
}
