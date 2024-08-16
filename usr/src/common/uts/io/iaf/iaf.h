/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_IAF_IAF_H	/* wrapper symbol for kernel use */
#define _IO_IAF_IAF_H	/* subject to change without notice */

#ident	"@(#)kern:io/iaf/iaf.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*	IAF structures and constants				*/

struct iaf {			/* used to pass AVAs to the module.	*/
	int count;		/* number of strings			*/
	int size;		/* length of strings (including NULLs)	*/
	char data[1];		/* arbitrary character array		*/
};

#if defined(_KERNEL) || defined(_KMEMUSER)

struct iafstate {
	lock_t	*iaf_mutex;	/* protection for message pointer */
	mblk_t	*iaf_mp;	/* AVA strings */
};

#endif /* _KERNEL || _KMEMUSER */

#define IAFMOD	"iaf"

/* User Level I/O Controls */
#define SETAVA	(('i' << 8) | 1)
#define GETAVA	(('i' << 8) | 2)

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_IAF_IAF_H */
