/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ULIMIT_H
#define _ULIMIT_H
#ident	"@(#)sgs-head:common/head/ulimit.h	1.5"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ulimit.h>

#ifdef __STDC__
extern long	ulimit(int, ...);
#else
extern long	ulimit();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_ULIMIT_H*/
