/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_IPC_SHM_H	/* wrapper symbol for kernel use */
#define _PROC_IPC_SHM_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ipc/shm.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
**	IPC Shared Memory Facility.
*/

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>  	/* SVR4COMPAT */
#include <proc/ipc/ipc.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/param.h>  	/* SVR4COMPAT */
#include <sys/ipc.h>		/* REQUIRED */

#else /* user */


#if defined(_XOPEN_SOURCE)
#include <sys/types.h>
#else
#include <sys/param.h>		/* SVR4COMPAT */
#endif /* defined(_XOPEN_SOURCE)*/

#include <sys/ipc.h>

#endif /* _KERNEL_HEADERS */

/*
 * Implementation constants.
 */

#define SHMLBA	_SHMLBA	/* segment low boundary address multiple */

/*
 * Padding constants used to reserve space for future use.
 */

#define	SHM_PAD		2
#define	SHM_PAD1	4

/*
 * Permission Definitions.
 */

#define	SHM_R	IPC_R	/* read permission */
#define	SHM_W	IPC_W	/* write permission */

/*
 * Message Operation Flags.
 */

#define	SHM_RDONLY	010000	/* attach read-only (else read-write) */
#define	SHM_RND		020000	/* round attach address to SHMLBA */


typedef ulong_t shmatt_t;


/*
 * Structure Definitions.
 */

#if !defined(_STYPES)

/*
 * There is a shared mem id data structure (kshmid_ds and shmid_ds) for each 
 * segment in the system.
 */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	_VOID		*shm_pad0;	/* placeholder for historical shm_amp */
	ushort_t	shm_lkcnt;	/* number of times it is being locked */
	pid_t		shm_lpid;	/* pid of last shmop */
	pid_t		shm_cpid;	/* pid of creator */
	shmatt_t	shm_nattch;	/* used only for shminfo */
	ulong_t		shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	long		shm_pad1;	/* reserved for time_t expansion */
	time_t		shm_dtime;	/* last shmdt time */
	long		shm_pad2;	/* reserved for time_t expansion */
	time_t		shm_ctime;	/* last change time */
	long		shm_pad3;	/* reserved for time_t expansion */
	long		shm_pad4[SHM_PAD1];	/* reserve area  */
};

#else	/* _STYPES */

/* SVR3 binary compatibility shmid_ds */
struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
	_VOID		*shm_pad0;	/* placeholder for historical shm_reg */
	ushort_t	shm_lkcnt;	/* number of times it is being locked */
	char 		pad[SHM_PAD];
	o_pid_t		shm_lpid;	/* pid of last shmop */
	o_pid_t		shm_cpid;	/* pid of creator */
	ushort_t	shm_nattch;	/* used only for shminfo */
	ushort_t	shm_cnattch;	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */
};

#endif	/* _STYPES */


#if defined(_KERNEL) || defined(_KMEMUSER)

struct kshmid_ds {
	struct shmid_ds kshm_ds;	/* SVR4 shared memory header */
	lock_t        	kshm_lck;	/* mutex lock for this object.	
					 * This lock potects all fields
					 * in shmid_ds, kshm_asp contents,
					 * kshm_flag, and shm_perm contents.
					 */
	struct vnode	*kshm_mvp;	/* vnode providing backing store */
	int		kshm_mapsize;	/* actual amount of backing store
					 * kshm_size >= shm_segsz (above)
					 */
	int		kshm_align;	/* byte alignment required for
					 * mapping segment
					 */
	uint_t   	kshm_refcnt;	/* reference count of this header */
	struct as 	*kshm_asp;	/* shadow as used to lock the object
					 * in memory and unlock/free all
					 * resources currently locked
					 */
	char    	kshm_flag;	/* set to SHM_BUSY to hold off all
					 * accesses to this object.
					 */
	sv_t         	kshm_sv;	/* synchronization variable for this
					 * object.
					 */
};

/* kshm_flag values */
#define	SHM_BUSY	0x01	/* the shared memory segment is currently
                                 * been manipulated. This will block out
				 * shmconv and consequently all shmat, ahmctl,
				 * aclipc and lvlipc calls.
				 * The other user shmdt will do explicit
				 * serialization with the above routines.
				 */

/* SVR3 structure */
struct o_shmid_ds {
	struct o_ipc_perm shm_perm; /* operation permission struct */
	int               shm_segsz;	/* size of segment in bytes */
	_VOID		 *shm_pad0;	/* segment anon_map pointer */
	ushort_t	 shm_lkcnt;	/* number of times it is being locked */
	char 		 pad[SHM_PAD];		
	o_pid_t		 shm_lpid;	/* pid of last shmop */
	o_pid_t		 shm_cpid;	/* pid of creator */
	ushort_t	 shm_nattch;	/* used only for shminfo */
	ushort_t	 shm_cnattch;	/* used only for shminfo */
	time_t		 shm_atime;	/* last shmat time */
	time_t		 shm_dtime;	/* last shmdt time */
	time_t		 shm_ctime;	/* last change time */
};


struct	shminfo {
	int	shmmax,		/* max shared memory segment size */
		shmmin,		/* min shared memory segment size */
		shmmni,		/* # of shared memory identifiers */
		shmseg;		/* max attached shared memory	  */
				/* segments per process		  */
};

typedef struct segacct {
	struct segacct	*sa_next;	/* the next item in the link list */
	vaddr_t		 sa_addr;	/* address at which a shared memory
					 * object is attched in current address
					 * space
					 */
	size_t		 sa_len;	/* actual size of the shared memory
					 * object in bytes
					 */
	struct kshmid_ds *sa_kshmp;	/* the header of the shared memory
					 * object
					 */
} segacct_t;

#endif	/* _KERNEL || _KMEMUSER */


/*
 * Shared memory control operations
 */
#define SHM_LOCK	3	/* Lock segment in core */
#define SHM_UNLOCK	4	/* Unlock segment */


#ifdef	_KERNEL

/*
 * Macros to lock and unlock a kshmid_ds cell.
 * idp is a pointer to the kshmid_ds cell to be locked/unlocked.
 */
#define SHMID_LOCK(kshmp)	LOCK_PLMIN(&(kshmp)->kshm_lck)
#define SHMID_UNLOCK(kshmp, pl)	UNLOCK_PLMIN(&(kshmp)->kshm_lck, (pl))

struct proc;
extern int	shmconv(int , struct kshmid_ds **);
extern void	shminit(void);
extern void	shmfork(struct proc *, struct proc *);
extern void	shmexit(struct proc *pp);
extern void	shmexec(struct proc *pp);

#else

#ifdef __STDC__

void	*shmat(int, const void *, int);
int	shmctl(int, int, struct shmid_ds *);
int	shmdt(const void *);
int	shmget(key_t, size_t, int);

#else

int shmctl();
int shmget();
void *shmat();
int shmdt();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_IPC_SHM_H */
