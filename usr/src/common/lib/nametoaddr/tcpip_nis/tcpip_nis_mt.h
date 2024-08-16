/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip_nis/tcpip_nis_mt.h	1.1"
#ident  "$Header: $"

/*
 * tcpip_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _ntoa_nis_tsd {
	void *inet_ntoa_p;
	void *hostdata_p;
	void *servdata_p;
};

#define _NTOA_NIS_KEYTBL_SIZE \
			( sizeof(struct _ntoa_nis_tsd) / sizeof(void *) )
#ifdef __STDC__

extern void _free_ntoa_nis_keytbl(void *);
extern void _free_ntoa_nis_hostdata(void *);
extern void _free_ntoa_nis_servdata(void *);
extern void _free_ntoa_nis_inet_ntoa(void *);
	
#else /* ! __STDC__ */

extern void _free_ntoa_nis_keytbl();
extern void _free_ntoa_nis_hostdata();
extern void _free_ntoa_nis_servdata();
extern void _free_ntoa_nis_inet_ntoa();

#endif /* __STDC__ */

extern THREAD_KEY_T	_ntoa_nis_key;
extern MUTEX_T		_ntoa_tcpip_port_lock;
extern RWLOCK_T		_ntoa_tcpip_addr_lock;
extern MUTEX_T	 	_ntoa_ypbind_lock;

#endif /* _REENTRANT */
