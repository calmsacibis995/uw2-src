/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/inc/nis_mt.h	1.1"
#ident  "$Header: $"

/*
 * nis_mt.h
 */

#ifndef _LNIS_MT_H
#define _LNIS_MT_H

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _nis_tsd {
	void *net_info_p;
	void *proto_info_p;
	void *serv_info_p;
	void *host_info_p;
	void *rpc_info_p;
	void *pwd_info_p;
	void *gr_info_p;
	void *nis_errno_p;
};

#define _NIS_KEYTBL_SIZE		( sizeof (struct _nis_tsd) / sizeof(void *) )

#ifdef __STDC__

extern void _free_nis_keytbl(void *);

extern void _free_nis_pwd_info(void *);
extern void _free_nis_gr_info(void *);
extern void _free_nis_net_info(void *);
extern void _free_nis_proto_info(void *);
extern void _free_nis_serv_info(void *);
extern void _free_nis_host_info(void *);
extern void _free_nis_rpc_info(void *);
extern void _free_nis_errno(void *);

#else /* ! __STDC__ */

extern void _free_s_keytbl();

extern void _free_nis_pwd_info();
extern void _free_nis_gr_info();
extern void _free_nis_net_info();
extern void _free_nis_proto_info();
extern void _free_nis_serv_info();
extern void _free_nis_host_info();
extern void _free_nis_rpc_info();
extern void _free_nis_errno();

#endif /* __STDC__ */

extern THREAD_KEY_T	_nis_key;
extern MUTEX_T      _nis_domain_lock;

#endif /* _REENTRANT */
#endif /* _LNIS_MT_H */
