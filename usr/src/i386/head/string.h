/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STRING_H
#define _STRING_H
#ident	"@(#)sgs-head:i386/head/string.h	1.7.4.11"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifndef NULL
#define NULL	0
#endif

#ifdef __STDC__

extern void	*memchr(const void *, int, size_t);
extern void	*memcpy(void *, const void *, size_t);
extern void	*memccpy(void *, const void *, int, size_t);
extern void	*memmove(void *, const void *, size_t);
extern void	*memset(void *, int, size_t);

extern char	*strchr(const char *, int);
extern char	*strcpy(char *, const char *);
extern char	*strdup(const char *);
extern char	*strncpy(char *, const char *, size_t);
extern char	*strcat(char *, const char *);
extern char	*strncat(char *, const char *, size_t);
extern char	*strpbrk(const char *, const char *);
extern char	*strrchr(const char *, int);
extern char	*strstr(const char *, const char *);
extern char	*strtok(char *, const char *);
extern char	*strtok_r(char *, const char *, char **);
extern char	*strerror(int);
extern char	*strlist(char *, const char *, ...);

extern int	memcmp(const void *, const void *, size_t);
extern int	strcmp(const char *, const char *);
extern int	strcoll(const char *, const char *);
extern int	strncmp(const char *, const char *, size_t);

extern size_t	strxfrm(char *, const char *, size_t);
extern size_t	strcspn(const char *, const char *);
extern size_t	strspn(const char *, const char *);
extern size_t	strlen(const char *);

#ifndef __cplusplus
	#pragma int_to_unsigned strcspn
	#pragma int_to_unsigned strspn
	#pragma int_to_unsigned strlen
#endif

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

extern int	ffs(int);

#endif

#else /*!__STDC__*/

extern char
	*strcpy(),
	*strncpy(),
	*strcat(),
	*strncat(),
	*strchr(),
	*strrchr(),
	*strpbrk(),
	*strtok(),
	*strtok_r(),
	*strdup(),
	*strstr(),
	*strerror(),
	*strlist();
extern int
	strcmp(),
	strncmp(),
	strcoll();
extern size_t
	strlen(),
	strspn(),
	strcspn(),
	strxfrm();
extern char
	*memccpy(),
	*memchr(),
	*memcpy(),
	*memmove(),
	*memset();
extern int memcmp();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_STRING_H*/
