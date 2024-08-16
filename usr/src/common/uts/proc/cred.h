/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_CRED_H	/* wrapper symbol for kernel use */
#define _PROC_CRED_H	/* subject to change without notice */

#ident	"@(#)kern:proc/cred.h	1.17"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*
 * User credentials.  The size of the cr_groups[] array is configurable
 * but is the same (ngroups_max) for all cred structures; cr_ngroups
 * records the number of elements currently in use, not the array size.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct cred {
	fspin_t	cr_mutex;		/* state lock for cr_ref */
	ushort_t cr_ngroups;		/* number of groups in cr_groups */
	uid_t	cr_uid;			/* effective user id */
	gid_t	cr_gid;			/* effective group id */
	uid_t	cr_ruid;		/* real user id */
	gid_t	cr_rgid;		/* real group id */
	uid_t	cr_suid;		/* "saved" user id (from exec) */
	gid_t	cr_sgid;		/* "saved" group id (from exec) */
	pvec_t	cr_savpriv;		/* saved privilege vector */
	pvec_t	cr_wkgpriv;		/* working privilege vector */
	pvec_t	cr_maxpriv;		/* maximum privilege vector */
	lid_t	cr_lid;			/* Level IDentifier (MAC) */
	lid_t	cr_cmwlid;		/* Level IDentifier (MAC) CMW */
	ulong_t	cr_flags;		/* add'l cred flags (see below) */
	ulong_t cr_seqnum;              /* sequence number of cred */
	ulong_t	cr_ref;			/* reference count */
	gid_t	cr_groups[1];		/* supplementary group list */
} cred_t;

/*
 * Definitions for flags in cr_flags
 */
#define CR_MLDREAL 0x00000001		/* set if proc is in "real" MLD mode */
#define CR_RDUMP   0x00000002		/* set if cred record is written */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern int ngroups_max;			/* max #of groups in a cred structure */
extern cred_t *sys_cred;		/* system credentials */

extern void cred_init(void);
extern void crholdn(cred_t *, u_int);
extern void crfreen(cred_t *, u_int);
extern cred_t *crget(void);
extern cred_t *crdup(cred_t *);
extern cred_t *crdup2(cred_t *);
extern size_t crgetsize(void);
extern void crinstall(cred_t *);
extern int groupmember(gid_t, cred_t *);
extern int hasprocperm(cred_t *, cred_t *);

#define	crhold(credp)	crholdn((credp), 1)	/* hold one reference */
#define	crfree(credp)	crfreen((credp), 1)	/* release one reference */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_CRED_H */
