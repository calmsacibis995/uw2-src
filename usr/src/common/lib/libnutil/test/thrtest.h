/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/thrtest.h	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/test/thrtest.h,v 1.1 1994/02/28 22:24:24 mark Exp $"

/*
 * novell_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

#ifdef __STDC__

extern void _free_ntoa_novell_keytbl(void *);
	
#else /* ! __STDC__ */

extern void _free_ntoa_novell_keytbl();

#endif /* __STDC__ */

/* thread-specific data structure definition */
struct _doit_keys {
	int counter;
};

/* key for thread specific data */
extern THREAD_KEY_T	_doit;

#endif /* _REENTRANT */
