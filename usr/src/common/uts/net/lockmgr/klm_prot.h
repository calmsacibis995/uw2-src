/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_LOCKMGR_KLM_PROT_H	/* wrapper symbol for kernel use */
#define _NET_LOCKMGR_KLM_PROT_H	/* subject to change without notice */

#ident	"@(#)kern:net/lockmgr/klm_prot.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	klm_prot.h, kernel lock manager protocol definitions
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h>	/* REQUIRED */
#include <net/rpc/xdr.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	KLM_PROG		((u_long)100020)
#define	KLM_VERS		((u_long)1)
#define	KLM_TEST		((u_long)1)
#define	KLM_LOCK		((u_long)2)
#define	KLM_CANCEL		((u_long)3)
#define	KLM_UNLOCK		((u_long)4)

#define	LM_MAXSTRLEN		1024

enum klm_stats {
	klm_granted = 0,
	klm_denied = 1,
	klm_denied_nolocks = 2,
	klm_working = 3,
	klm_deadlck = 5
};
typedef	enum klm_stats		klm_stats;

struct klm_lock {
	char	*server_name;
	netobj	fh;
	int	base;
	int	length;
	int	type;
	int	granted;
	int	color;
	int	LockID;
	int	pid;
	int	class;
	long	rsys;
	long	rpid;
};
typedef	struct klm_lock		klm_lock;

struct klm_holder {
	bool_t	exclusive;
	int	base;
	int	length;
	int	type;
	int	granted;
	int	color;
	int	LockID;
	int	pid;
	int	class;
	long	rsys;
	long	rpid;
};
typedef	struct klm_holder	klm_holder;

struct klm_stat {
	klm_stats stat;
};
typedef	struct klm_stat		klm_stat;

struct klm_testrply {
	klm_stats stat;
	union {
		struct klm_holder holder;
	} klm_testrply_u;
};
typedef	struct klm_testrply	klm_testrply;

struct klm_lockargs {
	bool_t	block;
	bool_t	exclusive;
	struct klm_lock alock;
};
typedef	struct klm_lockargs	klm_lockargs;

struct klm_testargs {
	bool_t	exclusive;
	struct klm_lock alock;
};
typedef	struct klm_testargs	klm_testargs;

struct klm_unlockargs {
	struct klm_lock alock;
};
typedef	struct klm_unlockargs	klm_unlockargs;

#ifdef	__STDC__

extern	bool_t		xdr_klm_stats(XDR *, klm_stats *);
extern	bool_t		xdr_klm_lock(XDR *, klm_lock *);
extern	bool_t		xdr_klm_holder(XDR *, klm_holder *);
extern	bool_t		xdr_klm_stat(XDR *, klm_stat *);
extern	bool_t		xdr_klm_testrply(XDR *, klm_testrply *);
extern	bool_t		xdr_klm_lockargs(XDR *, klm_lockargs *);
extern	bool_t		xdr_klm_testargs(XDR *, klm_testargs *);
extern	bool_t		xdr_klm_unlockargs(XDR *, klm_unlockargs *);

#else

extern	bool_t		xdr_klm_stats();
extern	bool_t		xdr_klm_lock();
extern	bool_t		xdr_klm_holder();
extern	bool_t		xdr_klm_stat();
extern	bool_t		xdr_klm_testrply();
extern	bool_t		xdr_klm_lockargs();
extern	bool_t		xdr_klm_testargs();
extern	bool_t		xdr_klm_unlockargs();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_LOCKMGR_KLM_PROT_H */
