/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/pfmtm.h	1.1"

#include <pfmt.h>
#include <stdlock.h>

struct sev_tab
{
	int		level;
	const char	*string;
};

extern const char	*_pfmt_label;
extern struct sev_tab	*_pfmt_sevtab;
extern int		_pfmt_nsev;
#ifdef _REENTRANT
extern StdLock		_pfmt_lock_setlabel;
#endif

#ifdef __STDC__
struct _FILE_;
		/*PRINTFLIKE3*/
extern int	_ipfmt(struct _FILE_ *, long, const char *, _VA_LIST);
#else
extern int	_ipfmt();
#endif
