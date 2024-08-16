/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_KSYM_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_KSYM_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/ksym.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>         /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>          /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* info for ioctl on /dev/kmem (driver mm) */
#define MIOC_READKSYM	(('M'<<8)|4)	/* ioctl command to read kernel space
					based on symbol name */
#define MIOC_IREADKSYM 	(('M'<<8)|5)	/* treat symbol as pointer to object to be read */

#define MIOC_WRITEKSYM  (('M'<<8)|6)	/* ioctl to write to kernel space based on symbol name */
#define MIOC_IWRITEKSYM  (('M'<<8)|7)

struct mioc_rksym {
	char *mirk_symname;	/* symbol at whose address read will start */
	void *mirk_buf;		/* buffer into which data will be written */
	size_t mirk_buflen;	/* length of read buffer */
};


/* info for getksym */
#define MAXSYMNMLEN	1024	/* max number of characters (incl. '\0') in
				   symbol name returned through getksym */
#ifndef _KERNEL

#ifdef __STDC__

extern int getksym(char *, unsigned long *, unsigned long *);

#else

extern int getksym();

#endif /* __STDC__ */

#endif /* ! _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_KSYM_H */
