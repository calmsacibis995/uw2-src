/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _DIRENT_H
#define _DIRENT_H
#ident	"@(#)sgs-head:common/head/dirent.h	1.6.1.9"

#ifdef __cplusplus
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

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define MAXNAMLEN	512	/* maximum filename length */
#define DIRBUF		2048	/* buffer size for fs-indep. dirs */
#endif

typedef struct
{
#ifdef dd_fd
	int	__dd_fd;	/* file descriptor */
#else
	int	dd_fd;
#endif
#ifdef dd_loc
	int	__dd_loc;	/* offset in block */
#else
	int	dd_loc;
#endif
#ifdef dd_size
	int	__dd_size;	/* amount of valid data */
#else
	int	dd_size;
#endif
#ifdef dd_buf
	char	*__dd_buf;	/* directory block */
#else
	char	*dd_buf;
#endif
} DIR;

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _SYS_DIRENT_H
#include <sys/dirent.h>
#endif

#ifdef __STDC__

extern DIR	*opendir(const char *);
struct dirent	*readdir(DIR *);
extern void	rewinddir(DIR *);
extern int	closedir(DIR *);
#if defined(_XOPEN_SOURCE) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
extern long	telldir(DIR *);
extern void	seekdir(DIR *, long);
#define rewinddir(p)	seekdir(p, 0L)
#endif

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
struct dirent	*readdir_r(DIR *, struct dirent *);
#endif

#else /*!__STDC__*/

extern DIR	*opendir();
struct dirent	*readdir();
extern void	rewinddir();
extern int	closedir();
#if defined(_XOPEN_SOURCE) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
extern long	telldir();
extern void	seekdir();
#define rewinddir(p)	seekdir(p, 0L)
#endif

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
struct dirent	*readdir_r();
#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_DIRENT_H*/
