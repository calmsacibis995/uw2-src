/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PWD_H
#define _PWD_H
#ident	"@(#)sgs-head:common/head/pwd.h	1.3.5.8"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct passwd
{
	char	*pw_name;
	char	*pw_passwd;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_age;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

struct comment
{
	char	*c_dept;
	char	*c_name;
	char	*c_acct;
	char	*c_bin;
};

#endif

#ifdef __STDC__

struct passwd	*getpwuid(uid_t);
struct passwd	*getpwnam(const char *);

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

struct passwd	*getpwent(void);
extern void	setpwent(void);
extern void	endpwent(void);
struct _FILE_;
struct passwd	*fgetpwent(struct _FILE_ *);
extern int	putpwent(const struct passwd *, struct _FILE_ *);

#endif

#else /*!__STDC__*/

struct passwd	*getpwuid();
struct passwd	*getpwnam();

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

struct passwd	*getpwent();
extern void	setpwent();
extern void	endpwent();
struct passwd	*fgetpwent();
extern int	putpwent();

#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_PWD_H*/
