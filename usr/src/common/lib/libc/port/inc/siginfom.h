/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/siginfom.h	1.1"

#include <siginfo.h>

struct siginfolist
{
	int nsiginfo;
	const char *const *vsiginfo;
};

extern const struct siginfolist	_sys_siginfolist[];
extern const int		_siginfo_msg_offset[];
extern const char *const	_sys_siglist[];
extern const int		_sys_nsig;
