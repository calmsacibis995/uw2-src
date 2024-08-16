/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/libsock_mt.h	1.3"
#ident	"$Header: $"

/*
 * libsock_mt.h
 */

#ifndef _LSOCK_MT_H
#define _LSOCK_MT_H

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _s_tsd {
	void *net_info_p;
	void *proto_info_p;
	void *serv_info_p;
	void *ndhost_info_p;
	void *host_info_p;
	void *ifig_info_p;
	void *token_info_p;
	void *netgr_info_p;
	void *h_errno_p;
	void *ether_s_p;
	void *ether_e_p;
};

#define _S_KEYTBL_SIZE		( sizeof (struct _s_tsd) / sizeof(void *) )

#ifdef __STDC__

extern void _free_s_keytbl(void *);

extern void _free_s_net_info(void *);
extern void _free_s_proto_info(void *);
extern void _free_s_serv_info(void *);
extern void _free_s_ndhost_info(void *);
extern void _free_s_host_info(void *);
extern void _free_s_ifig_info(void *);
extern void _free_s_token_info(void *);
extern void _free_s_netgr_info(void *);
extern void _free_s_h_errno(void *);
extern void _free_s_ether_s(void *);
extern void _free_s_ether_e(void *);

#else /* ! __STDC__ */

extern void _free_s_keytbl();

extern void _free_s_net_info();
extern void _free_s_proto_info();
extern void _free_s_serv_info();
extern void _free_s_ndhost_info();
extern void _free_s_host_info();
extern void _free_s_ifig_info();
extern void _free_s_token_info();
extern void _free_s_netgr_info();
extern void _free_s_h_errno();
extern void _free_s_ether_s();
extern void _free_s_ether_e();

#endif /* __STDC__ */

extern THREAD_KEY_T	_s_key;
extern MUTEX_T		_s_domain_lock;
extern MUTEX_T		_s_netgr_lock;
extern MUTEX_T		_s_port_lock;
extern RWLOCK_T		_s_si_user_lock;

#endif /* _REENTRANT */
#endif /* _LSOCK_MT_H */
