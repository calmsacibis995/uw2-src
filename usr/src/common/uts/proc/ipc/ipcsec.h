/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_IPCSEC_H	/* wrapper symbol for kernel use */
#define	_PROC_IPC_IPCSEC_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ipc/ipcsec.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <acc/dac/acl.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/acl.h>

#endif /* _KERNEL_HEADERS */

/* Common IPC DAC structure */
struct ipc_dac {
	int 		aclcnt;		/* number of ACL entries */
	struct 	acl	acls[1];	/* ACL entries in memory */
};

/* Common IPC Access Control Structure */
struct ipc_sec {
	struct ipc_dac	*dacp;		/* DAC ptr */
	lid_t		ipc_lid;	/* MAC level identifier */
	lid_t		ipc_cmwlid;	/* MAC level identifier  CMW */
};

/*
 * For portability, the size of the DAC structure is computed by the
 * following macro.  This is to avoid potential problems caused by padding.
 */
#define	DACSIZE(aclcnt) \
		(sizeof(struct ipc_dac) + (((aclcnt) - 1) * sizeof(struct acl)))

/*
 * Common IPC routine to free DAC structure.
 * Note that secp must be a pointer to an ipc_sec structure.
 * Further note that writing this macro as an expression allows
 * it to be called from anywhere without generating ambiguous
 * if-else code or syntax errors.
 */
#define	FRIPCACL(secp) \
	(void) (((secp)->dacp != NULL) ?			\
		kmem_free((void *)((secp)->dacp),		\
			(size_t)DACSIZE((secp)->dacp->aclcnt)),	\
		(secp)->dacp = NULL : 0);

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_IPCSEC_H */
