/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netdir/netdir_mt.h	1.1.1.2"
#ident  "$Header: $"

/*
 * netdir_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

struct _netdir_tsd {
	void	*_nderror_p;
	void	*errbuf_p;
};

#define NETDIR_KEYTBL_SIZE    ( sizeof(struct _netdir_tsd) / sizeof(void *) )

extern THREAD_KEY_T _netdir_key;
extern RWLOCK_T _netdir_xlist_lock;

extern void _free_netdir_keytbl(void *);
extern void _free_netdir__nderror(void *);
extern void _free_netdir_errbuf(void *);

#endif /* _REENTRANT */
