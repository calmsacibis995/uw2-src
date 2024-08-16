/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:fp/fpgetround.c	1.4"

#include <ieeefp.h>
#include "fp.h"
#include "synonyms.h"

#ifdef __STDC__
	#pragma	weak fpgetround = _fpgetround
#endif

fp_rnd
fpgetround()
{
	struct _cw87 cw;

	_getcw(&cw);
	return (fp_rnd)cw.rnd;
}
