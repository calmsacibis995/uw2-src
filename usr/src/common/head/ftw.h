/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FTW_H
#define _FTW_H
#ident	"@(#)sgs-head:common/head/ftw.h	1.3.1.19"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/stat.h>

#define	FTW_F	0	/* file */
#define	FTW_D	1	/* directory */
#define	FTW_DNR	2	/* directory without read permission */
#define	FTW_NS	3	/* unknown type, stat failed */
#define FTW_SL	4	/* symbolic link */
#define	FTW_DP	6	/* directory */
#define FTW_SLN	7	/* symbolic link that points to nonexistent file */

#define FTW_PHYS	01	/* use lstat instead of stat */
#define FTW_MOUNT	02	/* do not cross a mount point */
#define FTW_CHDIR	04	/* chdir to each directory before reading */
#define FTW_DEPTH	010	/* call descendents before calling the parent */

#define FTW_SKD		1
#define FTW_FOLLOW	2
#define FTW_PRUNE	4

#ifndef _XOPEN_SOURCE
struct FTW
{
	int	quit;
	int	base;
	int	level;
};
#endif

#define _XFTWVER	2

#ifdef _STYPES

#include <errno.h>

#if defined(__STDC__) || defined(__cplusplus)
static int nftw(const char *__p,
		int (*__f)(const char *, const struct stat *, int, struct FTW *),
		int __d,
		int __i)
#else
static int nftw(__p, __f, __d, __i)const char *__p; int (*__f)(); int __d, __i;
#endif
{
	errno = EINVAL;
	return -1;
}

#else /*!_STYPES*/

#ifdef __STDC__
extern int _xftw(int, const char *,
	int (*)(const char *, const struct stat *, int), int);
#else
extern int _xftw();
#endif

#ifndef _EFTSAFE
#if defined(__STDC__) || defined(__cplusplus)
static int ftw(const char *__p, int (*__f)(const char *, const struct stat *, int), int __d)
#else
static int ftw(__p, __f, __d)const char *__p; int (*__f)(); int __d;
#endif
{
	return _xftw(_XFTWVER, __p, __f, __d);
}
#endif

#endif /*_STYPES*/

#ifdef __STDC__

extern int ftw(const char *,
	int (*)(const char *, const struct stat *, int), int);

#ifndef _XOPEN_SOURCE
extern int nftw(const char *,
	int (*)(const char *, const struct stat *, int, struct FTW *),
	int, int);
#endif

#else /*!__STDC__*/

extern int ftw();

#ifndef _XOPEN_SOURCE
extern int nftw();
#endif

#endif /*__STDC__*/

#if defined(_EFTSAFE) && !defined(_STYPES)
#undef ftw
#define	ftw(p, f, d)	_xftw(_XFTWVER, p, f, d)
#endif

#ifdef __cplusplus
}
#endif

#endif /*_FTW_H*/
