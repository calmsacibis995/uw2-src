/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/ns/inc/ns_mt.h	1.1"
#ident  "$Header: $"

/*
 * ns_mt.h
 */

#ifndef _LNS_MT_H
#define _LNS_MT_H

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _ns_tsd {
	void *ns_action_p;
	void *ns_getent_p;
};

#define _NS_KEYTBL_SIZE		( sizeof (struct _ns_tsd) / sizeof(void *) )

#ifdef __STDC__

extern void _free_ns_keytbl(void *);
extern void _free_ns_info(void *);

#else /* ! __STDC__ */

extern void _free_ns_keytbl();
extern void _free_ns_info();

#endif /* __STDC__ */

extern THREAD_KEY_T	_ns_key;

#endif /* _REENTRANT */
#endif /* _LNS_MT_H */
