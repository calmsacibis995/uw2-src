/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:fp/fpgetsticky.c	1.4"

#include <ieeefp.h>
#include "fp.h"
#include "synonyms.h"

#ifdef __STDC__
	#pragma weak fpgetsticky = _fpgetsticky
#endif

fp_except
fpgetsticky()
{
	struct _sw87 sw;

	_getsw(&sw);
	return (fp_except)sw.excp;
}
