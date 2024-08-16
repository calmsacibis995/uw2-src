/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ERRNO_H
#define _ERRNO_H
#ident	"@(#)sgs-head:common/head/errno.h	1.4.4.2"

#include <sys/errno.h>

#ifdef _REENTRANT

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
extern int	*__thr_errno(void);
#else
extern int	*__thr_errno();
#endif

#ifdef __cplusplus
}
#endif

#define errno	(*__thr_errno())

#else /*!_REENTRANT*/

extern int	errno;

#endif /*_REENTRANT*/

#endif /*_ERRNO_H*/
