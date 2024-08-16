/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_SEM_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_SEM_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ipc/sem.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 * System V IPC Semaphore Facility.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/list.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/ipc/ipc.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/list.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/ipc.h>		/* REQUIRED */

#else

#include <sys/ipc.h>

#endif /* _KERNEL_HEADERS */

/*
 * X/Open XPG4 types.
 */

#ifndef _PID_T
#define _PID_T
typedef long	pid_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long	time_t;
#endif

#ifndef _KEY_T
#define _KEY_T
typedef int	key_t;
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int	size_t;
#endif

/*
 * Implementation Constants.
 */
#if !defined(_XOPEN_SOURCE)
#define	SEMNPRI	(PRIMED + 1)	/* sleep priority waiting for greater value */
#define	SEMZPRI	(PRIMED)	/* sleep priority waiting for zero */
#endif /* !defined(_XOPEN_SOURCE) */

/*
 * Padding constants used to reserve space for future use.
 */
#define	SEM_PAD		4

/*
 * Permission Definitions:
 */
#define	SEM_A		IPC_W	/* alter permission */
#define	SEM_R		IPC_R	/* read permission */

/*
 * Semaphore Operation Flags:
 *	Value must be unique w.r.t. the common IPC definition
 *	for IPC_NOWAIT.
 */
#define	SEM_UNDO	010000	/* set up adjust on exit entry */

/*
 * Semctl Command Definitions:
 */
#define	GETNCNT		3	/* get semncnt */
#define	GETPID		4	/* get sempid */
#define	GETVAL		5	/* get semval */
#define	GETALL		6	/* get all semval's */
#define	GETZCNT		7	/* get semzcnt */
#define	SETVAL		8	/* set semval */
#define	SETALL		9	/* set all semval's */

/*
 * Structure Definitions.
 */

#if !defined(_STYPES)

/*
 * There is one semaphore id data structure (semid_ds) for each set of
 * semaphores in the system.
 */
struct semid_ds {
	struct ipc_perm sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t	sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	long		sem_pad1;	/* reserved for time_t expansion */
	time_t		sem_ctime;	/* last change time */
	long		sem_pad2;		/* time_t expansion */
	long		sem_pad3[SEM_PAD];	/* reserve area */
};

#else	/* _STYPES */

/* SVR3 binary compatibility semid_ds structure */
struct semid_ds {
	struct ipc_perm sem_perm;	/* operation permission struct */
	struct sem	*sem_base;	/* ptr to first semaphore in set */
	ushort_t	sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
};

#endif /* _STYPES */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Since the semid_ds is a published data structure, it is
 * extended via the ksemid_ds structure which encapsulates
 * a semid_ds structure.
 *
 * The kernel manipulates ksemid_ds structures, the user
 * application sees semid_ds structures.
 */
struct ksemid_ds {
	struct semid_ds	ksem_ds;
	lock_t		ksem_mutex;	/* per semid_ds mutex */
	long		ksem_flag;	/* per semid_ds flags, see below */
	sv_t		ksem_sv;	/* per semid_ds synch variable */
};

/* ksem_flag values: */
#define	SEMID_BUSY	0x1		/* semid_ds is busy */

/*
 * Macros to LOCK/UNLOCK a semid_ds structure.
 * 'sp' is a pointer to the ksemid_ds structure
 * to be locked/unlocked.
 */
#define	SEMID_LOCK(sp)		LOCK_PLMIN(&(sp)->ksem_mutex)
#define	SEMID_UNLOCK(sp, pl)	UNLOCK_PLMIN(&(sp)->ksem_mutex, (pl))

/*
 * Macro to convert from the address of the embedded ipc_perm structure
 * to the encapsulating ksemid_ds structure.
 */
#define	IPC_TO_SEMDS(ipcp)	((struct ksemid_ds *)(ipcp))

/* SVR3 structure */
struct o_semid_ds {
	struct o_ipc_perm sem_perm;	/* operation permission struct */
	void		*sem_base;	/* ptr to first semaphore in set */
	ushort_t	sem_nsems;	/* # of semaphores in set */
	time_t		sem_otime;	/* last semop time */
	time_t		sem_ctime;	/* last change time */
};

/*
 * There is one semaphore structure (sem) for each semaphore in the system.
 */

struct sem {
	ushort_t semval;	/* semaphore value */
	pid_t	sempid;		/* pid of last operation */
	ushort_t semncnt;	/* # awaiting semval > cval */
	ushort_t semzcnt;	/* # awaiting semval = 0 */
	sv_t	semn_sv;	/* synch variable for semncnt */
	sv_t	semz_sv;	/* synch variable for semzcnt */
};

/*
 * There is one undo structure per process in the system which has
 * done a semop(2) with the SEM_UNDO flag set.
 * This structure is pointed to by the p_semundo member of the proc
 * structure.
 */
struct sem_undo {
	list_t		un_np;		/* list of active sem_undo structures */
	short		un_cnt;		/* # active entries in the undo array */
	fspin_t		un_mutex;	/* mutex for this sem_undo struct */
	struct undo {
		short	un_aoe;		/* adjust on exit value */
		ushort_t un_num;	/* semaphore # within semaphore set */
		int	un_id;		/* semid */
	} un_ent[1];			/* undo entries (one minimum) */
};

/*
 * Semaphore information structure.
 *	For SVR4.2, the semmap, semmnu, and semusz fields
 *	of the seminfo structure are not used.  This is
 *	because everything is dynamically allocated.
 */
struct	seminfo	{
	int	semmap;		/* XXX # of entries in semaphore map */
	int	semmni;		/* # of semaphore identifiers */
	int	semmns;		/* # of semaphores in system */
	int	semmnu;		/* XXX # of undo structures in system */
	int	semmsl;		/* max # of semaphores per id */
	int	semopm;		/* max # of operations per semop call */
	int	semume;		/* max # of undo entries per process */
	int	semusz;		/* XXX size in bytes of undo structure */
	int	semvmx;		/* semaphore maximum value */
	int	semaem;		/* adjust on exit max value */
};

#endif	/* _KERNEL || _KMEMUSER */


/*
 * User semaphore template for semop(2):
 */
struct sembuf {
	ushort_t sem_num;	/* semaphore # */
	short	sem_op;		/* semaphore operation */
	short	sem_flg;	/* operation flags */
};


#ifdef _KERNEL

extern int	semconv(const int, struct ksemid_ds **, const int);
extern void	seminit(void);

#else

#ifdef __STDC__

int semctl(int, int, int, ...);
int semget(key_t, int, int);
int semop(int, struct sembuf *, size_t);

#else

int semctl();
int semget();
int semop();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_SEM_H */
