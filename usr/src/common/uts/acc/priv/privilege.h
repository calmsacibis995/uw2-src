/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_ACC_PRIV_PRIVILEGE_H	/* wrapper symbol for kernel use */
#define	_ACC_PRIV_PRIVILEGE_H	/* subject to change without notice */

#ident	"@(#)kern:acc/priv/privilege.h	1.24"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <svc/systm.h>		/* REQUIRED (for rval_t) */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/systm.h>		/* REQUIRED (for rval_t) */

#else

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

/*
 *
 * The following is the typedef for the user-level privilege
 * definition.  It is here because kernel routines also need
 * to know about this particular type.
 *
 */

typedef	unsigned	long	priv_t;

/*
 *
 * The following are the known privilege sets.
 *
 *	PS_FIX		for fixed privilege sets
 *	PS_INH		for inheritable privilege sets
 *	PS_MAX		for maximum privilege sets
 *	PS_WKG		for working privilege sets
 *
 */

#define	PS_FIX		0x66000000
#define	PS_INH		0x69000000
#define	PS_MAX		0x6d000000
#define	PS_WKG		0x77000000
#define	PS_TYPE		0xff000000

/*
 *
 * The following are the supported object types for
 * privilege mechanisms.
 *
 */

#define	PS_FILE_OTYPE	0x00000000
#define	PS_PROC_OTYPE	0x00000001

/*
 *
 * The following is the set of all known privileges
 *
 * Also, the define NPRIVS is the number of privileges
 * currently in use.  It should be modified whenever a
 * privilege is added or deleted.
 * The component libcmd:common/lib/libcmd/privname.c
 * must also be updated if privileges are added or
 * deleted.
 *
 */

#define	NPRIVS		27

#define	P_OWNER		0x00000000
#define	P_AUDIT		0x00000001
#define	P_COMPAT	0x00000002
#define	P_DACREAD	0x00000003
#define	P_DACWRITE	0x00000004
#define	P_DEV		0x00000005
#define	P_FILESYS	0x00000006
#define	P_MACREAD	0x00000007
#define	P_MACWRITE	0x00000008
#define	P_MOUNT		0x00000009
#define	P_MULTIDIR	0x0000000a
#define	P_SETPLEVEL	0x0000000b
#define	P_SETSPRIV	0x0000000c
#define	P_SETUID	0x0000000d
#define	P_SYSOPS	0x0000000e
#define	P_SETUPRIV	0x0000000f
#define	P_DRIVER	0x00000010
#define	P_RTIME		0x00000011
#define	P_MACUPGRADE	0x00000012
#define	P_FSYSRANGE	0x00000013
#define	P_SETFLEVEL	0x00000014
#define	P_AUDITWR	0x00000015
#define	P_TSHAR		0x00000016
#define	P_PLOCK		0x00000017
#define P_CORE		0x00000018
#define P_LOADMOD	0x00000019
#define P_BIND		0x0000001a

#define P_FPRI		P_RTIME

/*
 *
 * P_ALLPRIVS is a shorthand for all defined privileges.  When adding
 * or removing privileges to the list above, DO NOT change the
 * definition of P_ALLPRIVS.  It is NOT a bitmap, so it's definition
 * has nothing to do with how many privileges are defined.  (Do make
 * sure that NPRIVS is defined correctly above.)
 *
 */

#define	P_ALLPRIVS	0x00ffffff


/*
 *
 * The following  defines  are recognized by the privilege
 * mechanisms.  They are returned in the argument value of
 * the secsys() system call in the form of flags when  the
 * command is ES_PRVINFO.
 *
 */

#define	PM_UIDBASE	0x00000001
#define	PM_ULVLINIT	0x00000002
#define PM_PRVMODE	0x00000004

/*
 *
 * The following are the CMDS recognized by the procpriv()
 * and filepriv() system calls.
 *
 */

#define	SETPRV		0x0
#define	CLRPRV		0x1
#define	PUTPRV		0x2
#define GETPRV		0x3
#define CNTPRV		0x4

/*
 *
 * Structure definition for the privilege sets supported
 * by individual privilege servers.  Also some defines
 * that are used at user-level related to the privilege
 * mechanisms.
 *
 */

#define	PRVNAMSIZ	 32
#define	PRVMAXSETS	256

typedef	struct	pm_setdef {
	priv_t	sd_mask;
	uint	sd_setcnt;
	char	sd_name[PRVNAMSIZ];
	ulong	sd_objtype;
} setdef_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *
 * The following macros are used by the different privilege
 * servers to manipulate privilege bits.
 *
 */

#define	pm_allon		((1 << NPRIVS) - 1)
#define	pm_pos(p)		(pvec_t)((p) & ~PS_TYPE)
#define	pm_type(p)		(pvec_t)((p) & PS_TYPE)
#define	pm_pridc(p)		(pvec_t)((p) >> 24)
#define	pm_privbit(p)		(pvec_t)(1 << (p))
#define	pm_pridt(p)		(pvec_t)((p) << 24)
#define	pm_invalid(p)		(((pm_pos((p)) < 0 || pm_pos((p)) >= NPRIVS) && pm_pos((p)) != P_ALLPRIVS) ? 1 : 0)
#define	pm_setbits(p, v)	(v |= (((p) == P_ALLPRIVS) ? pm_allon : (1<<pm_pos(p))))
#define pm_privon(a, b)		((a)->cr_wkgpriv & (b))
#define	pm_subset(a, b)		(((a)->cr_maxpriv & (b)->cr_maxpriv) == (b)->cr_maxpriv)
/*
 * If the maximum privileges in the credentials passed are non-zero
 * then the process is privileged.
 */
#define	pm_privileged(a)	((a)->cr_maxpriv)
/*
 * The pm_dacread() macro examines the maximum privilege vector to
 * determine if it contains the P_DACREAD privilege to override DAC.
 */
#define	pm_dacread(cr)		((cr)->cr_maxpriv & pm_privbit(P_DACREAD))

/*
 * The pm_macread() macro examines the maximum privilege vector to
 * determine if it contains the ability to override MAC with either
 * the P_MACREAD privilege (explicit) or the P_SETPLEVEL privilege
 * (by changing the process level).
 */
#define	pm_macread(cr)		((cr)->cr_maxpriv & \
				(pm_privbit(P_MACREAD)|pm_privbit(P_SETPLEVEL)))


#endif	/* _KERNEL || _KMEMUSER */

#if defined(_KERNEL) || defined(_KMEMUSER)

extern int pm_denied(cred_t *credp, int);
extern int pm_process(int, rval_t *, cred_t *, ulong_t *, int, cred_t **);
extern boolean_t pm_calcpriv(vnode_t *, vattr_t *, cred_t *, int);
extern int pm_file(int, vnode_t *, vattr_t *, rval_t *, cred_t *,
		   ulong_t *, int);
extern void pm_init(void);
extern void pm_clrdev(vfs_t *);
extern void pm_recalc(cred_t *);
extern int pm_secsys(int, rval_t *, caddr_t);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _ACC_PRIV_PRIVILEGE_H */
