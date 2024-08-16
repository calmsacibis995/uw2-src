/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PFMT_H
#define _PFMT_H
#ident	"@(#)sgs-head:common/head/pfmt.h	1.9"

#ifdef __cplusplus
extern "C" {
#endif

#define MM_STD		0
#define MM_NOSTD	0x100
#define MM_GET		0
#define MM_NOGET	0x200

#define MM_ACTION	0x400

#define MM_NOCONSOLE	0
#define MM_CONSOLE	0x800

#define MM_NULLMC	0
#define MM_HARD		0x1000
#define MM_SOFT		0x2000
#define MM_FIRM		0x4000
#define MM_APPL		0x8000
#define MM_UTIL		0x10000
#define MM_OPSYS	0x20000

#define MM_SVCMD	(MM_UTIL|MM_SOFT)	/* common combination */

#define MM_ERROR	0
#define MM_HALT		1
#define MM_WARNING	2
#define MM_INFO		3

#define DB_NAME_LEN	15	/* includes terminating \0 */
#define MAXLABEL	25	/* obsolete */

#ifndef _VA_LIST
#   if #machine(i860)
	struct _va_list
	{
		unsigned _ireg_used, _freg_used;
		long	*_reg_base, *_mem_ptr;
	};
#	define _VA_LIST struct _va_list
#   else
#	define _VA_LIST void *
#   endif
#endif

#ifdef __STDC__
struct _FILE_;
		/*PRINTFLIKE3*/
extern int	pfmt(struct _FILE_ *, long, const char *, ...);
		/*PRINTFLIKE3*/
extern int	lfmt(struct _FILE_ *, long, const char *, ...);
extern int	vpfmt(struct _FILE_ *, long, const char *, _VA_LIST);
extern int	vlfmt(struct _FILE_ *, long, const char *, _VA_LIST);
extern char	*setcat(const char *);
extern int	setlabel(const char *);
extern int	addsev(int, const char *);
#else
extern int	pfmt(), lfmt(), vpfmt(), vlfmt();
extern int	setlabel(), addsev();
extern char	*setcat();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_PFMT_H*/
