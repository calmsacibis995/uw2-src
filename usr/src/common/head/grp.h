/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _GRP_H
#define _GRP_H
#ident	"@(#)sgs-head:common/head/grp.h	1.3.5.6"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct group
{
	char	*gr_name;
	char	*gr_passwd;
	gid_t	gr_gid;
	char	**gr_mem;
};

#ifdef __STDC__

struct group	*getgrgid(gid_t);
struct group	*getgrnam(const char *);

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
struct _FILE_;
extern void	endgrent(void);
struct group	*fgetgrent(struct _FILE_ *);
struct group	*getgrent(void);
extern void	setgrent(void);
extern int	initgroups(const char *, gid_t);
#endif

#else /*!__STDC__*/

struct group	*getgrgid();
struct group	*getgrnam();

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
extern void	endgrent();
struct group	*fgetgrent();
struct group	*getgrent();
extern void	setgrent();
extern int	initgroups();
#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_GRP_H*/
