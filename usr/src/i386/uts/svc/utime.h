/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UTIME_H	/* wrapper symbol for kernel use */
#define _SVC_UTIME_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/utime.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* utimbuf is used by utime(2) */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

struct utimbuf {
	time_t actime;		/* access time */
	time_t modtime;		/* modification time */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_UTIME_H */
