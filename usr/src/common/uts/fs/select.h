/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SELECT_H	/* wrapper symbol for kernel use */
#define _FS_SELECT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/select.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields.
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	1024
#endif

#ifndef NBBY		/* number of bits per byte */
#define NBBY 8
#endif

typedef	long	fd_mask;
#define	NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef	howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#if defined(_KERNEL)
#define	FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#else
#include <string.h> 	/* for definition of memset */
#define	FD_ZERO(p)	memset((char *)(p), 0, sizeof(*(p)))
#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#if !defined(_KERNEL)
#if defined(__STDC__)
struct timeval;
int select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#else
int select();
#endif
#endif /* !defined(_KERNEL) */

#endif	/* _FS_SELECT_H */
