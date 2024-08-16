/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_dummy.c	1.2.5.2"
#ident	"$Header: $"

#include "errno.h"
#include "sys/xti.h"
#include "_import.h"

extern int t_errno;

_dummy()
{
	t_errno = TSYSERR;
	errno = ENXIO;
	return(-1);
}
