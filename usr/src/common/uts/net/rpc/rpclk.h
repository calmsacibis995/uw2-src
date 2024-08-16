/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_RPCLK_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPCLK_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpclk.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	rpclk.h, all the lock hierarchy and minipl information
 *	pertaining to the kernel rpc subsystem.
 */

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define PRIRPC		PRIMED
#define PLRPC		PLSTR

#define RPC_HIER_BASE	150

#define RPC_HIERCKUFLAGS	(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERXID		(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERKEYCALL		(uchar_t) (RPC_HIER_BASE + 5)

#define RPC_HIERUDFLAGS		(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERRQCRED		(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERAUTHDES		(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERDRHASHTBL	(uchar_t) (RPC_HIER_BASE + 5)
#define RPC_HIERSVC		(uchar_t) (RPC_HIER_BASE + 5)

/*
 * spin lock initialization table
 */
static struct lock_init_table {
	lock_t		*lit_addr;	/* lock address */
	uchar_t		 lit_hier;	/* lock hierarchy */
	pl_t		 lit_minpl;	/* minimum ipl */
	lkinfo_t	*lit_lkinfop;	/* address of lkinfo struct */
	int		 lit_kmflags;	/* to sleep or not to sleep */
};

/*
 * sleep lock initialization table
 */
static struct sleep_init_table {
	sleep_t		*sit_addr;	/* lock address */
	uchar_t		 sit_hier;	/* lock hierarchy */
	lkinfo_t	*sit_lkinfop;	/* address of lkinfo struct */
	int		 sit_kmflags;	/* to sleep or not to sleep */
};

/*
 * readers/writers spin lock initialization table
 */
static struct rwlock_init_table {
	rwlock_t	*rwlit_addr;	/* lock address */
	uchar_t		 rwlit_hier;	/* lock hierarchy */
	pl_t		 rwlit_minpl;	/* minimum ipl */
	lkinfo_t	*rwlit_lkinfop;	/* address of lkinfo struct */
	int		 rwlit_kmflags;	/* to sleep or not to sleep */
};

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPCLK_H */
