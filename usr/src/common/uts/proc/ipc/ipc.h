/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_IPC_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_IPC_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ipc/ipc.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Common IPC Access Structures.
 *
 * The kernel supports both the SVR3 ipc_perm and expanded ipc_perm
 * structures.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <proc/ipc/ipc_f.h>	/* PORTABILITY */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/ipc_f.h>		/* PORTABILITY */

#else	/* user */

#include <sys/ipc_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */


/* XPG4 data types */

#ifndef _USHORT_T
#define _USHORT_T
typedef unsigned short	ushort_t;
#endif

#ifndef _ULONG_T
#define _ULONG_T
typedef unsigned long	ulong_t;
#endif

#ifndef _KEY_T
#define _KEY_T
typedef int		key_t;		/* IPC key type */
#endif

#ifndef _MODE_T
#define _MODE_T
typedef unsigned long	mode_t;		/* file attribute type */
#endif

#ifndef _UID_T
#define _UID_T
typedef long		uid_t;		/* UID type */
#endif

#ifndef _GID_T
#define _GID_T
typedef uid_t		gid_t;		/* GID type */
#endif


/* Padding constants used to reserve space for future use */
#define	IPC_PERM_PAD	3

#if defined(_STYPES)

/* SVR3 binary compatibility ipc_perm structure */
typedef struct ipc_perm {
	o_uid_t	uid;		/* owner's user id */
	o_gid_t	gid;		/* owner's group id */
	o_uid_t	cuid;		/* creator's user id */
	o_gid_t	cgid;		/* creator's group id */
	o_mode_t mode;		/* access modes */
	ushort_t seq;		/* slot usage sequence number */
	key_t	key;		/* key */
} ipc_perm_t;

/* SVR3 binary compatibility Control Commands. */
#define	IPC_RMID	0	/* remove identifier */
#define	IPC_SET		1	/* set options */
#define	IPC_STAT	2	/* get options */

#else	/* !_STYPES */

/* SVR4 ipc_perm structure */
typedef struct ipc_perm {
	uid_t	uid;		/* owner's user id */
	gid_t	gid;		/* owner's group id */
	uid_t	cuid;		/* creator's user id */
	gid_t	cgid;		/* creator's group id */
	mode_t	mode;		/* access modes */
	ulong_t	seq;		/* slot usage sequence number */
	key_t	key;		/* key */
	struct ipc_sec *ipc_secp;	/* security structure ptr */
	long	pad[IPC_PERM_PAD];	/* reserve area */
} ipc_perm_t;

/* Control Commands. */
#define	IPC_RMID	10	/* remove identifier */
#define	IPC_SET		11	/* set options */
#define	IPC_STAT	12	/* get options */

#endif	/* !_STYPES */


#if defined(_KERNEL) || defined(_KMEMUSER)

/* SVR3 binary compatibility ipc_perm structure */
struct o_ipc_perm {
	o_uid_t	uid;		/* owner's user id */
	o_gid_t	gid;		/* owner's group id */
	o_uid_t	cuid;		/* creator's user id */
	o_gid_t	cgid;		/* creator's group id */
	o_mode_t mode;		/* access modes */
	ushort_t seq;		/* slot usage sequence number */
	key_t	key;		/* key */
};

/* SVR3 binary compatibility command values */
#define	IPC_O_RMID	0	/* remove identifier */
#define	IPC_O_SET	1	/* set options */
#define	IPC_O_STAT	2	/* get options */


typedef struct ipcops {
	int	(*ipc_alloc)(ipc_perm_t **);
	void	(*ipc_dealloc)(ipc_perm_t *);
} ipcops_t;

typedef struct ipcdirent {
	ipc_perm_t *ipcd_ent;		/* ptr to ipc_perm */
	ulong	ipcd_seq;		/* entry usage sequence number */
} ipcdirent_t;

typedef struct ipcdir {
	int	ipcdir_nents;		/* total # of entries */
	int	ipcdir_nactive;		/* total # of active entries */
	ipcdirent_t *ipcdir_entries;	/* array of ipcdirent's */
} ipcdir_t;

/*
 * Format of structure passed to ipcget():
 */
typedef struct ipcdata {
	ipcdir_t *ipc_dir;		/* ipc directory */
	ipcops_t *ipc_ops;		/* operations vector */
} ipcdata_t;

/*
 * Macros to convert from an ipc id to a slot number; ipc id to a
 * sequence number; or from a slot number to an ipc id; respectively.
 * The sequence number is in the 'high order' part of the identifier
 * and the slot number is in the 'low order' part.
 */
#define	IPCID_TO_SLOT(id, n)		((id) % (n))
#define	IPCID_TO_SEQ(id, n)		((id) / (n))
#define	SLOT_TO_IPCID(slot, seq, n)	(((seq) * (n)) + (slot))

#endif	/* _KERNEL || _KMEMUSER */


/* To differentiate IPC types */
#define	IPC_SHM		0x01		/* shared memory */
#define	IPC_SEM		0x02		/* semaphores */
#define	IPC_MSG		0x04		/* message queues */

/* Common IPC Permission Masks */
#define	IPC_R		0400
#define	IPC_W		0200
#define	IPC_PERM	0777

/* Common IPC_Security Access Requests */
#define	IPC_MAC		0x01
#define	IPC_DAC		0x02

/* Common IPC Definitions. */
/* Mode bits. */
#define	IPC_ALLOC	0100000		/* entry currently allocated */
#define	IPC_CREAT	0001000		/* create entry if key doesn't exist */
#define	IPC_EXCL	0002000		/* fail if key exists */
#define	IPC_NOWAIT	0004000		/* error if request must wait */

/* Keys. */
#define	IPC_PRIVATE	(key_t)0	/* private key */



#if defined(_KERNEL)
/*
 * Function prototypes for ipc interfaces:
 */
struct cred;
extern int ipcaccess(ipc_perm_t *, int, uint, struct cred *);
extern int ipcget(key_t, int, ipcdata_t *, boolean_t *, ipcdirent_t **);

/*
 * Macros used by ipcget() to allocate/deallocate an instance
 * of an ipc structure.
 *	IPC_ALLOCATE(ipcdata_t *ipcdatp, ipc_perm_t **ipcpp)
 *	IPC_DEALLOCATE(ipcdata_t *ipcdatp, ipc_perm_t *ipcp)
 */
#define	IPC_ALLOCATE(ipcdatp, ipcpp) ((*(ipcdatp)->ipc_ops->ipc_alloc)  (ipcpp))
#define	IPC_DEALLOCATE(ipcdatp, ipcp)((*(ipcdatp)->ipc_ops->ipc_dealloc)(ipcp))

#else /* !_KERNEL */

#if !defined(_XOPEN_SOURCE)

#if defined(__STDC__)
extern key_t ftok(const char *, int);
#else
extern key_t ftok();
#endif

#endif /* !defined(_XOPEN_SOURCE)*/

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_IPC_H */
