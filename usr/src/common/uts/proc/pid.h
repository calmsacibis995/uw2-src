/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_PID_H	/* wrapper symbol for kernel use */
#define _PROC_PID_H	/* subject to change without notice */

#ident	"@(#)kern:proc/pid.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/ksynch.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

/* 
 * procent structure definition, and the macros to convert from a
 * procdir slot number to a process pointer.
 */

/* directory entries for /proc */

union procent {
        struct proc *pe_proc;
        union procent *pe_next;
};

extern union procent *procdir;

/* 
 * To translate from a procdir slot index to a process pointer:
 * check if the pointer stored in the slot points to an address
 * within the procdir table (i.e., is between procdir[0] and
 * procdir[v.v_proc]).  If so, it cannot be a valid process pointer.
 */
#define PDIR2PROC(slotno) procdir[(slotno)].pe_proc
#define ISPDIRPTR(ptr) (((ptr) >= (proc_t *)procdir) \
		&& ((ptr) < (proc_t *)(procdir + v.v_proc)))
#define PSLOT2PROC(i) (ISPDIRPTR(PDIR2PROC(i)) ? NULL : PDIR2PROC(i))

/*
 * Process ID info:
 *  Pid references:
 *	process-ID, process group-ID, session-ID reference,
 *	foreground process group-ID.
 *
 * Locking rules:
 *  The following fields are locked by holding the proper pidhash_mutex
 *  lock:
 *	pid_link, pid_procref, pid_ref.
 *
 *  The following fields are locked by holding the appropriate session
 *  locked:
 *	pid_zpgref, pid_pgprocs, pid_pgrpf (pid_sess.sess.f),
 *	pid_pgrpb (pid_sess.sess.b), pid_zpgsess (pid_sess.zpg).
 *
 *  The following fields are locked by holding the referenced process
 *  p_mutex lock:
 *	pid_procp.
 */
struct pid {
	pid_t  pid_id;			/* ID */
	struct pid *pid_link;		/* next pid on hash chain */
	volatile u_long pid_procref;	/* #of driver refs to proc with ID */
	volatile u_int pid_ref    :  4;	/* #of ID refs */
	volatile u_int pid_zpgref : 28;	/* #of ID refs as pgrp-ID by zombies */
	struct proc *pid_pgprocs;	/* non-zombie processes in pgrp */
	union {
		struct {		/* pgrp sess links when non-zomb pgrp */
			struct pid *f;	/* forward pgrp session link */
			struct pid *b;	/* backward pgrp session link */
		} pgrp;
		struct sess *zpg;	/* containing session when zomb pgrp */
	} pid_sess;
#define pid_pgrpf pid_sess.pgrp.f	/* forward pgrp in session */
#define pid_pgrpb pid_sess.pgrp.b	/* backward pgrp in session */
#define pid_zpgsess pid_sess.zpg	/* session when no non-zomb in pgrp */
	volatile struct proc *pid_procp;/* process with this ID if the ID */
};					/* is in use as process-ID */

/*
 * Process-ID hash chain structure:
 */
typedef struct pidhash {
	lock_t pidhash_mutex;		/* pidhash_link lock */
	struct pid *pidhash_link;	/* first pid structure on hash chain */
} pidhash_t;

extern pidhash_t *pidhash;		/* pid hash table */
extern int pid_hashsize;		/* size of pid hash table */
/*
 * This macro returns the hash table entry for pid.
 */
#define HASHPID(pid)	(&pidhash[((pid)&(pid_hashsize-1))])

/*
 * Structure used to pass information from pid_next_entries() and
 * lwpid_next_entries() to their callers.
 */
typedef struct idbuf {
	id_t	id_id;		/* Process or LWP id */
	int	id_nodeid;	/* Node ID */
	int	id_slot;	/* Slot number */
} idbuf_t;

struct prcommon;	/* XXX */

int pid_next_entries(void *, int, int, idbuf_t *, int *);
int lwpid_next_entries(struct prcommon *, int, int, idbuf_t *, int *);

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_PID_H */
