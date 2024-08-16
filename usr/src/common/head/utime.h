/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIME_H
#define _UTIME_H
#ident	"@(#)sgs-head:common/head/utime.h	1.4"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/utime.h>

#ifdef __STDC__
extern int	utime(const char *, const struct utimbuf *);
#else
extern int	utime();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_UTIME_H*/
