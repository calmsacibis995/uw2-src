/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/flock.c	1.13"
#ident	"$Header: $"

/*
 * This file contains all of the file/record locking specific routines.
 * 
 * All record lock lists (referenced by a pointer in the vnode) are
 * ordered by starting position relative to the beginning of the file.
 * 
 * In this file the name "l_end" is a macro and is used in place of
 * "l_len" because the end, not the length, of the record lock is
 * stored internally.
 */

#include <util/debug.h>
#include <util/types.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <proc/user.h>
#include <proc/exec.h>
#include <fs/fcntl.h>
#include <mem/kmem.h>
#include <fs/flock.h>
#include <svc/systm.h>
#include <mem/tuneable.h>
#include <util/metrics.h>
#include <fs/fs_hier.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

void	rm_sleeplcks(struct flock *);

#define	WAKEUP(ptr)	 if (SV_BLKD(&ptr->frlock_sv)) {		\
				SV_BROADCAST(&ptr->frlock_sv, 0);	\
			 }

#define SAMEOWNER(a, b)	 (((a)->l_pid == (b)->l_pid) &&			\
				((a)->l_sysid == (b)->l_sysid))

/*
 * the following macro is currently replaced by rm_sleeplcks().
 * later on rm_sleeplcks() will be switched with this.
 */
#define	RM_SLEEPLCKS(tmp, lck)					\
	pl_t	pl2;						\
	pl2 = LOCK(&sleeplcks_mutex, FS_SLPLOCKPL);		\
	for ((tmp) = sleeplcks; (tmp) != NULL;) {		\
		if ((tmp)->set.l_whence == (lck)->l_whence &&	\
		    (tmp)->set.l_start  >= (lck)->l_start &&	\
		    (tmp)->set.l_len    <= (lck)->l_len &&	\
		    (tmp)->stat.blk.sysid  == (lck)->l_sysid &&	\
		    (tmp)->stat.blk.pid    == (lck)->l_pid &&	\
		    (tmp)->set.l_sysid != 0) {			\
			delflck(&sleeplcks, tmp);		\
			(tmp) = sleeplcks;			\
		} else {					\
			(tmp) = (tmp)->next;			\
		}						\
	}							\
	UNLOCK(&sleeplcks_mutex, pl2);

/* region types */
#define	S_BEFORE	010
#define	S_START		020
#define	S_MIDDLE	030
#define	S_END		040
#define	S_AFTER		050
#define	E_BEFORE	001
#define	E_START		002
#define	E_MIDDLE	003
#define	E_END		004
#define	E_AFTER		005

STATIC	struct	filock	*sleeplcks;	/* head of chain of sleeping locks */

/*
 * Fast spin lock to protect file lock metrics. The metric information
 * for the number of file lock table entries in use is stored in
 * msf_flck[MET_INUSE] of the mets_file structure. This number is
 * obtained by the call MET_FLCK_CNT(); an entry is added to or
 * subtracted from by calling MET_FLCK_INUSE(inuse) where inuse
 * is either a 1 or -1.
 */
STATIC	fspin_t	frlock_mutex;

/*
 * Spin lock to protect sleeplcks list. Before traversing sleeplcks
 * to insert or delete an entry, sleeplcks_mutex must be held to
 * prevent other LWPs from accessing this list.
 */
STATIC	lock_t	sleeplcks_mutex;
lkinfo_t	sleeplcks_lkinfo;
LKINFO_DECL(sleeplcks_lkinfo, "FS: sleeplcks_mutex: sleeplcks list", 0);

/*
 * void
 * frlck_init(void)
 *	Initialize file/record spin locks and store system maximum
 *	for number of file/record locks.
 *
 * Calling/Exit State:
 *	Called from fs_init(). Thus, no locking is necessary.
 */
void
frlck_init(void)
{
	LOCK_INIT(&sleeplcks_mutex, FS_SLPLOCKHIER, FS_SLPLOCKPL,
		  &sleeplcks_lkinfo, KM_SLEEP);
	FSPIN_INIT(&frlock_mutex);

	MET_FLCK_MAX(tune.t_flckrec);
}

/*
 * filock_t *
 * insflck(filock_t **lck_list, flock_t *lckdat, filock_t *fl, lock_t *lck_ptr)
 *	Insert lock (lckdat) after given lock (fl).
 *
 * Calling/Exit State:
 *	Spin lock sleeplcks_mutex or vp->v_filocks_mutex (denoted by lck_ptr)
 *	is held on entry and exit.
 *
 * Description:
 *	If fl is NULL place the new lock at the beginning of the list
 *	and update the head ptr to list which is stored at the address
 *	given by lck_list. frlock_mutex is held while checking/updating
 *	metric info.
 */
/* ARGSUSED */
static filock_t *
insflck(filock_t **lck_list, flock_t *lckdat, filock_t *fl, lock_t *lck_ptr)
{
	filock_t *new;

	FSPIN_LOCK(&frlock_mutex);
	if (MET_FLCK_CNT() >= tune.t_flckrec) {
		MET_FLCK_FAIL();
		FSPIN_UNLOCK(&frlock_mutex);
#ifdef CC_PARTIAL
		CC_COUNT(CC_RE_FLOCK, CCBITS_RE_FLOCK);
#endif
		return (NULL);
	}
	FSPIN_UNLOCK(&frlock_mutex);

	new = (filock_t *)kmem_zalloc(sizeof(filock_t), KM_NOSLEEP);
	if (new == NULL) {
		FSPIN_LOCK(&frlock_mutex);
		MET_FLCK_FAIL();
		FSPIN_UNLOCK(&frlock_mutex);
		return (NULL);
	}
	SV_INIT(&new->frlock_sv);

	FSPIN_LOCK(&frlock_mutex);
	MET_FLCK_INUSE(1);
	MET_FLCK_TOTAL(1);
	FSPIN_UNLOCK(&frlock_mutex);

	new->set = *lckdat;
	new->set.l_pid = lckdat->l_pid;
	new->set.l_sysid = lckdat->l_sysid;
	new->stat.wakeflg = 0;
	if (fl == NULL) {
		new->next = *lck_list;
		if (new->next != NULL) {
			new->next->prev = new;
		}
		*lck_list = new;
	} else {
		new->next = fl->next;
		if (fl->next != NULL) {
			fl->next->prev = new;
		}
		fl->next = new;
	}
	new->prev = fl;

	return (new);
}

/*
 * delflck(filock_t **lck_list, filock_t *fl)
 *	Delete lock (fl) from the record lock list (lck_list).
 *
 * Calling/Exit State:
 *	Spin lock sleeplcks_mutex or vp->v_filocks_mutex is held
 *	on entry and exit.
 *
 * Description:
 *	If fl is the first lock in the list, remove it and update the
 *	head ptr to the list which is stored at the address given by
 *	lck_list. Call SV_BROADCAST() to awaken any blocked LWPs wishing
 *	to set a file/record lock on a region (or portion thereof).
 */
static
delflck(filock_t **lck_list, filock_t *fl)
{
	if (fl->prev != NULL) {
		fl->prev->next = fl->next;
	} else {
		*lck_list = fl->next;
	}
	if (fl->next != NULL) {
		fl->next->prev = fl->prev;
	}
	WAKEUP(fl);

	FSPIN_LOCK(&frlock_mutex);
	MET_FLCK_INUSE(-1);
	FSPIN_UNLOCK(&frlock_mutex);

	kmem_free((caddr_t)fl, sizeof(struct filock));
	return (0);
}

/*
 * int
 * regflck(flock_t *ld, filock_t *flp)
 *	Sets the type of span of this (un)lock relative to the specified
 *	already existing locked section.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	There are five regions relative to the already locked section.
 *	The type is two octal digits, the 8's digit is the start type
 *	and the 1's digit is the end type.
 *
 *  S_BEFORE        S_START         S_MIDDLE         S_END          S_AFTER
 *     010            020             030             040             050
 *  E_BEFORE        E_START         E_MIDDLE         E_END          E_AFTER
 *      01             02              03              04              05
 * 			|-------------------------------|
 *
 */
static int
regflck(flock_t *ld, filock_t *flp)
{
	int regntype;

	if (ld->l_start > flp->set.l_start) {
		if (ld->l_start-1 == flp->set.l_end) {
			return (S_END|E_AFTER);
		} else if (ld->l_start > flp->set.l_end) {
			return (S_AFTER|E_AFTER);
		} else {
			regntype = S_MIDDLE;
		}
	} else if (ld->l_start == flp->set.l_start) {
		regntype = S_START;
	} else {
		regntype = S_BEFORE;
	}

	if (ld->l_end < flp->set.l_end) {
		if (ld->l_end == flp->set.l_start-1) {
			regntype |= E_START;
		} else if (ld->l_end < flp->set.l_start) {
			regntype |= E_BEFORE;
		} else {
			regntype |= E_MIDDLE;
		}
	} else if (ld->l_end == flp->set.l_end) {
		regntype |= E_END;
	} else {
		regntype |= E_AFTER;
	}

	return (regntype);
}

/*
 * int
 * flckadj(filock_t **lck_list, filock_t *insrtp, flock_t *ld, lock_t *lck_ptr)
 *	Adjust file lock from region specified by 'ld', in the record
 *	lock list indicated by the head ptr stored at the address given
 *	by lck_list.
 *
 * Calling/Exit State:
 *	Spin lock vp->v_filocks_mutex (denoted by lck_ptr) is held on
 *	entry and exit.
 *
 * Description:
 *	Start updates at the lock given by 'insrtp'.  It is 
 *	assumed the list is ordered on starting position, relative to 
 *	the beginning of the file, and no updating is required on any
 *	locks in the list previous to the one pointed to by insrtp.
 *	Insrtp is a result from the routine blocked().  Flckadj() scans
 *	the list looking for locks owned by the process requesting the
 *	new (un)lock :
 *
 * 		- If the new record (un)lock overlays an existing lock of
 * 	  	  a different type, the region overlaid is released.
 *
 * 		- If the new record (un)lock overlays or adjoins an exist-
 * 	  	  ing lock of the same type, the existing lock is deleted
 * 	  	  and its region is coalesced into the new (un)lock.
 *
 *	When the list is sufficiently scanned and the new lock is not 
 *	an unlock, the new lock is inserted into the appropriate
 *	position in the list.
 */
static int
flckadj(filock_t **lck_list, filock_t *insrtp, flock_t *ld, lock_t *lck_ptr)
{
	filock_t *flp;
	filock_t *nflp;
	int	 regtyp;

	nflp = (insrtp == NULL) ? *lck_list : insrtp;

	flp = nflp;
	while (flp != NULL) {
		nflp = flp->next;
		if(SAMEOWNER(&(flp->set), ld)) {

			/* Release already locked region if necessary */

			regtyp = regflck(ld, flp);
			switch (regtyp) {
			case S_BEFORE|E_BEFORE:
				nflp = NULL;
				break;
			case S_BEFORE|E_START:
				if (ld->l_type == flp->set.l_type) {
					ld->l_end = flp->set.l_end;
					delflck(lck_list, flp);
				}
				nflp = NULL;
				break;
			case S_START|E_END:
				/*
				 * Don't bother if this is in the middle of
				 * an already similarly set section.
				 */
				if (ld->l_type == flp->set.l_type) {
					return (0);
				}
				/* FALLTHRU */
			case S_START|E_AFTER:
				insrtp = flp->prev;
				delflck(lck_list, flp);
				break;
			case S_BEFORE|E_END:
				if (ld->l_type == flp->set.l_type) {
					nflp = NULL;
				}
				/* FALLTHRU */
			case S_BEFORE|E_AFTER:
				delflck(lck_list, flp);
				break;
			case S_START|E_MIDDLE:
				insrtp = flp->prev;
				/* FALLTHRU */
			case S_MIDDLE|E_MIDDLE:
				/*
				 * Don't bother if this is in the middle of
				 * an already similarly set section.
				 */
				if (ld->l_type == flp->set.l_type) {
					return (0);
				}
				/* FALLTHRU */
			case S_BEFORE|E_MIDDLE:
				if (ld->l_type == flp->set.l_type) {
					ld->l_end = flp->set.l_end;
				} else {
					/* setup piece after end of (un)lock */
					filock_t *tdi, *tdp;
					flock_t  td;

					td = flp->set;
					td.l_start = ld->l_end + 1;
					tdp = tdi = flp;
					do {
						if (tdp->set.l_start < td.l_start) {
							tdi = tdp;
						} else {
							break;
						}
					} while (tdp = tdp->next);
					if (insflck(lck_list, &td, tdi,
					    lck_ptr) == NULL) {
						return (ENOLCK);
					}
				}
				if (regtyp == (S_MIDDLE|E_MIDDLE)) {
					/* setup piece before (un)lock */
					flp->set.l_end = ld->l_start - 1;
					WAKEUP(flp);
					insrtp = flp;
				} else {
					delflck(lck_list, flp);
				}
				nflp = NULL;
				break;
			case S_MIDDLE|E_END:
				/*
				 * Don't bother if this is in the middle of
				 * an already similarly set section.
				 */
				if (ld->l_type == flp->set.l_type) {
					return (0);
				}
				flp->set.l_end = ld->l_start - 1;
				WAKEUP(flp);
				insrtp = flp;
				break;
			case S_MIDDLE|E_AFTER:
				if (ld->l_type == flp->set.l_type) {
					ld->l_start = flp->set.l_start;
					insrtp = flp->prev;
					delflck(lck_list, flp);
				} else {
					flp->set.l_end = ld->l_start - 1;
					WAKEUP(flp);
					insrtp = flp;
				}
				break;
			case S_END|E_AFTER:
				if (ld->l_type == flp->set.l_type) {
					ld->l_start = flp->set.l_start;
					insrtp = flp->prev;
					delflck(lck_list, flp);
				}
				break;
			case S_AFTER|E_AFTER:
				insrtp = flp;
				break;
			}
		}
		flp = nflp;
	}

	if (ld->l_type != F_UNLCK) {
		flp = insrtp;
		if (flp != NULL) {
			do {
				if (flp->set.l_start < ld->l_start) {
					insrtp = flp;
				} else {
					break;
				}
			} while (flp = flp->next);
		}
		if (insflck(lck_list, ld, insrtp, lck_ptr) == NULL) {
			return (ENOLCK);
		}
	}

	return (0);
}

/*
 * filock_t *
 * blocked(filock_t *flp, flock_t *lckdat, filock_t **insrt)
 *	Checks whether a new lock (lckdat) would be blocked by a previously
 *	set lock owned by another process. Insrt is set to point to the lock
 *	where lock list updating should begin to place the new lock.
 *
 * Calling/Exit State:
 *	Spin lock vp->v_filocks_mutex is held on entry and exit.
 *
 * Description:
 *	A pointer to a filock_t is returned if another process has a 
 *	file/record lock on part of the region requested to be locked.
 *	If no file/record lock covers the range requested to be locked,
 *	NULL is returned.
 */ 
static filock_t *
blocked(filock_t *flp, flock_t *lckdat, filock_t **insrt)
{
	filock_t *f;

	/* XENIX Support */
	unsigned xenix_compat;

	/* XENIX binaries doing an fcntl() with
	 * GETLK expect to receive blocking info
	 * for F_UNLCK.  This is bogus, but that's
	 * the way XENIX behaved...
	 * Pre-System V XENIX binaries cannot
	 * have overlapping read locks.  The flag
	 * xenix_compat specifies whether either of
	 * these conditions is true.
	 */
	xenix_compat = ((VIRTUAL_XOUT && ISFCNTL) && lckdat->l_type == F_UNLCK)
				|| BADVISE_PRE_SV;
	/* End XENIX Support */

	*insrt = NULL;
	for (f = flp; f != NULL; f = f->next) {
		if (f->set.l_start < lckdat->l_start) {
			*insrt = f;
		} else {
			break;
		}
		if(SAMEOWNER(&(f->set), lckdat)) {
			if ((lckdat->l_start-1) <= f->set.l_end) {
				break;
			}
		} else if (lckdat->l_start <= f->set.l_end
		  && (f->set.l_type == F_WRLCK
				/* XENIX Support */
				|| (xenix_compat && ISLOCKING)
				/* End XENIX Support */
		  || (f->set.l_type == F_RDLCK && lckdat->l_type == F_WRLCK))) {
			return (f);
		}
	}

	for (; f != NULL; f = f->next) {
		if (lckdat->l_end < f->set.l_start) {
			break;
		}
		if (lckdat->l_start <= f->set.l_end
		  && (!SAMEOWNER(&(f->set), lckdat))
		  && (f->set.l_type == F_WRLCK
				/* XENIX Support */
				|| (xenix_compat && ISLOCKING)
				/* End XENIX Support */
		  || (f->set.l_type == F_RDLCK && lckdat->l_type == F_WRLCK))) {
			return (f);
		}
	}

	return (NULL);
}

/*
 * int
 * convoff(off_t size, flock_t *lckdat, int whence, off_t offset)
 * 	converts the given data (start, whence) to the given whence.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *	The caller must insure that the size of the vnode remains
 *	stable; if not, the whence value will be incorrect since
 *	it will be calculated relative to the (old) file size.
 */
int
convoff(off_t size, flock_t *lckdat, int whence, off_t offset)
{

	if (lckdat->l_whence == 1) {
		lckdat->l_start += offset;
	} else if (lckdat->l_whence == 2) {
		lckdat->l_start += size;
	} else if (lckdat->l_whence != 0) {
		return (EINVAL);
	}
	if (lckdat->l_start < 0) {
		return (EINVAL);
	}
	if (whence == 1) {
		lckdat->l_start -= offset;
	} else if (whence == 2) {
		lckdat->l_start -= size;
	} else if (whence != 0) {
		return (EINVAL);
	}
	lckdat->l_whence = (short)whence;
	return (0);
}

/*
 * int
 * deadflck(filock_t *flp, flock_t *lckdat)
 *	Detects a deadlock for a given record.
 *
 * Calling/Exit State:
 *	sleeplcks_mutex is held on entry and exit to prevent the list
 *	of LWPs blocked from changing while checking for deadlock.
 *	vp->v_filocks_mutex is also held to stabilize the list of
 *	outstanding file/record locks for the vnode.
 *
 * Description:
 *	Returns 1 is a deadlock situation would occur if the calling
 *	context were allowed to block; otherwise 0 is returned.
 */
static int
deadflck(filock_t *flp, flock_t *lckdat)
{
	filock_t *blck;
	filock_t *sf;
	pid_t	 blckpid;
	long	 blcksysid;

	blck = flp;	/* current blocking lock pointer */
	blckpid = blck->set.l_pid;
	blcksysid = blck->set.l_sysid;
	do {
		if (blckpid   == lckdat->l_pid &&
		    blcksysid == lckdat->l_sysid) {
			return (1);
		}
		/*
		 * If the blocking process is sleeping on a locked region,
		 * change the blocked lock to this one.
		 */
		for (sf = sleeplcks; sf != NULL; sf = sf->next) {
			if (blckpid == sf->set.l_pid &&
			    blcksysid == sf->set.l_sysid) {
				blckpid = sf->stat.blk.pid;
				blcksysid = sf->stat.blk.sysid;
				break;
			}
		}
		blck = sf;
	} while (blck != NULL);
	return (0);
}

/*
 * int
 * reclock(vnode_t *vp, flock_t *lckdat, int cmd, int flag, off_t offset,
 * 	   off_t size)
 *	Get, set or unlock a file/record lock.
 *
 * Calling/Exit State:
 *	The inode's rwlock is held exclusive or shared on entry and exit.
 *
 * Description:
 *	The vnode's v_filocks_mutex must be acquired before determining
 *	whether the caller can perform any of the file/record lock ops.
 *
 *	cmd & SETFLCK indicates setting a lock.
 *	cmd & SLPFLCK indicates waiting if there is a blocking lock.
 *	cmd & INOFLCK indicated the associated vnode is locked.
 */
int
reclock(vnode_t *vp, flock_t *lckdat, int cmd, int flag, off_t offset,
	off_t size)
{
	filock_t	**lock_list;
	filock_t	*sf;
	filock_t	*found;
	filock_t	*insrt;
	int		retval, ret = 0;
	int		contflg;
	pl_t		pl1;
	pl_t		pl2;

	found = insrt = NULL;
	/* check access permissions */
	/* XENIX Support */
	/*
	 * If this is not a pre-System V XENIX binary, then
	 * check access permissions if trying to set a lock, or if this
	 * is a XENIX binary doing an fcntl() system call or a binary
	 * doing a XENIX locking() system call.
	 */
	if (!BADVISE_PRE_SV)
	if (((cmd & SETFLCK) || (((VIRTUAL_XOUT) && ISFCNTL) || ISLOCKING))
	/* End XENIX Support - UNIX SVR3 Code was deleted */
	  && ((lckdat->l_type == F_RDLCK && (flag & FREAD) == 0)
	  ||  (lckdat->l_type == F_WRLCK && (flag & FWRITE) == 0))) {
		/* XENIX Support */
		if (VIRTUAL_XOUT || ISLOCKING) {
			/* return EINVAL vs. EBADF for XENIX compatibility */
			return (EINVAL);
		} else {
		/* End XENIX Support */
			return (EBADF);
		}
	}
	
	/* Convert start to be relative to beginning of file */
	retval = convoff(size, lckdat, 0, offset);
	if (retval) {
		return (retval);
	}

	/* Convert l_len to be the end of the rec lock l_end */
	if (lckdat->l_len < 0) {
		return (EINVAL);
	} else if (lckdat->l_len == 0) {
		lckdat->l_end = MAXEND;
	} else {
		lckdat->l_end += (lckdat->l_start - 1);
	}

	/* check for arithmetic overflow */
	if (lckdat->l_start > lckdat->l_end) {
		return (EINVAL);
	}

	pl1 = LOCK(&vp->v_filocks_mutex, FS_VPFRLOCKPL);
	lock_list = &vp->v_filocks;

	do {
	   contflg = 0;
	   switch (lckdat->l_type) {
	   case F_RDLCK:
	   case F_WRLCK:
	   	found = blocked(*lock_list, lckdat, &insrt);
	   	if (found == NULL) {
	   		if (cmd & SETFLCK) {
	   			retval = flckadj(lock_list, insrt,
	   				 lckdat, &vp->v_filocks_mutex);
	   		} else {
	   			lckdat->l_type = F_UNLCK;
	   		}
	   		if ((cmd & (RCMDLCK|SLPFLCK)) == (RCMDLCK|SLPFLCK)) {
	   			rm_sleeplcks(lckdat);
	   		}
	   	} else if (cmd & SLPFLCK) {
	   		pl2 = LOCK(&sleeplcks_mutex, FS_SLPLOCKPL);
	   		/* do deadlock detection here */
	   		if (deadflck(found, lckdat)) {
	   			UNLOCK(&sleeplcks_mutex, pl2);
	   			retval = EDEADLK;
	   		} else if (cmd & RCMDLCK) {
	   		    retval = ELKBUSY;
   
   			    /* If request not already on sleeplcks,
   			     * put it there. (for deadlock detection)
     			     */

   			    for (sf = sleeplcks; sf != NULL; sf = sf->next) {
   				if (sf->set.l_type == lckdat->l_type &&
   				    sf->set.l_whence == lckdat->l_whence &&
   				    sf->set.l_start == lckdat->l_start &&
   				    sf->set.l_len == lckdat->l_len &&
   				    sf->set.l_sysid == lckdat->l_sysid &&
   				    sf->set.l_pid == lckdat->l_pid && 
   				    sf->stat.blk.sysid == found->set.l_sysid &&
   				    sf->stat.blk.pid == found->set.l_pid) {
   					break;
   				}
   			    }
   			    if (sf == NULL) {
   				sf = insflck(&sleeplcks, lckdat,
   					(filock_t *)NULL, &sleeplcks_mutex);
	   			if (sf == NULL) {
	   			    retval = ENOLCK;
	   			} else {
	   			    sf->stat.blk.pid = found->set.l_pid;
	   			    sf->stat.blk.sysid = found->set.l_sysid;
	   			}
	   		    }
	   		    UNLOCK(&sleeplcks_mutex, pl2);
	   		} else {
	   		   sf = insflck(&sleeplcks, lckdat,
	   				(filock_t *)NULL, &sleeplcks_mutex);
	   		   if (sf == NULL) {
	   		   	UNLOCK(&sleeplcks_mutex, pl2);
	   		   	retval = ENOLCK;
	   		   } else {
	   		   	found->stat.wakeflg++;
	   		   	sf->stat.blk.pid = found->set.l_pid;
	   		   	sf->stat.blk.sysid = found->set.l_sysid;
	   		   	UNLOCK(&sleeplcks_mutex, pl2);
	   		   	VOP_RWUNLOCK(vp, lckdat->l_start, lckdat->l_len);
	   		   	if (SV_WAIT_SIG(&found->frlock_sv, PRIVFS,
					    &vp->v_filocks_mutex) == B_FALSE) {
	   		   		retval = EINTR;
	   		   	} else {
	   		   		contflg = 1;
	   		   	}
	   		   	pl2 = LOCK(&sleeplcks_mutex, FS_SLPLOCKPL);
	   		   	sf->stat.blk.pid = 0;
	   		   	sf->stat.blk.sysid = 0;
	   		   	delflck(&sleeplcks, sf);
	   		   	UNLOCK(&sleeplcks_mutex, pl2);
	   		   	if (lckdat->l_type == F_WRLCK) {
	   		   		ret = VOP_RWWRLOCK(vp, lckdat->l_start,
							   lckdat->l_len, 0);
	   		   	} else {
	   		   		ret = VOP_RWRDLOCK(vp, lckdat->l_start,
							   lckdat->l_len, 0);
	   		   	}
	   		   	pl1 = LOCK(&vp->v_filocks_mutex, FS_VPFRLOCKPL);
	   		   	if (retval || ret) {
	   		   		contflg = 0;
	   		   	}
	   		   }
	   		}
	   	} else if (cmd & SETFLCK) {
	   		retval = EAGAIN;
	   	} else {
	   		*lckdat = found->set;
	   	}
	   	break;
	   case F_UNLCK:
	   	/* removing a file record lock */
	   	if (cmd & SETFLCK) {
	   		retval = flckadj(lock_list, *lock_list, lckdat,
	   				 &vp->v_filocks_mutex);
	   	/* XENIX Support */
	   	/*
	   	 * XENIX binaries doing an fcntl() with
	   	 * GETLK expect to receive blocking info
	   	 * for F_UNLCK.  This is bogus, but that's
	   	 * the way XENIX behaved...
	   	 */
	   	} else if (((VIRTUAL_XOUT) && ISFCNTL) &&
	   		((found = blocked(*lock_list, lckdat, &insrt))
	   					!= NULL)) {
	   			*lckdat = found->set;
	   	}
	   	/* End XENIX Support */
   
   		if (cmd & RCMDLCK) {
   			rm_sleeplcks(lckdat);
   		}
   		break;
	   default:
   		/* invalid lock type */
   		retval = EINVAL;
   		break;
   	   }
	} while (contflg);
   
	UNLOCK(&vp->v_filocks_mutex, pl1);

	/* Restore l_len */
	if (lckdat->l_end == MAXEND) {
		lckdat->l_len = 0;
	} else {
		lckdat->l_len -= (lckdat->l_start-1);
	}
	/*
	 * POSIX compatibility requires that we disable this perfectly
	 * nice behavior.  The returned lock description will be normalized
	 * relative to SEEK_SET (0) instead of being converted back to the
	 * format that was passed in.
	 *
	 * (void) convoff(vp, lckdat, whence, offset);
	 */
	if (retval)
		return (retval);
	else
		return (ret);
}

/*
 * int
 * chklock(vnode_t *vp, int iomode, off_t offset, int len, int fmode,
 *	   off_t size)
 *	Enforce record locking protocol on regular file vp.
 *
 * Calling/Exit State:
 *	The inode's rwlock is held exclusively or shared on entry and exit.
 *
 * Description:
 *	Check whether a file/record lock exists on a given region of a
 *	file. If it does, the caller will conditionally block until the
 *	file/record lock covering the region is released. If the caller's
 *	open instantiation of the file used FNDELAY or FNONBLOCK and a
 *	file/record lock exists on the region specified, then EAGAIN is
 *	returned instead of blocking the LWP.
 */
int
chklock(vnode_t *vp, int iomode, off_t offset, int len, int fmode, off_t size)
{
	flock_t	bf;
	int	i;
	int	error;

	error = 0;
	bf.l_type = (iomode & FWRITE) ? F_WRLCK : F_RDLCK;
	bf.l_whence = 0;
	bf.l_start = offset;
	bf.l_len = len;

	/* XENIX Support */
	if (bf.l_len == 0)      /* Don't check whole file in reclock() */
		if (VIRTUAL_XOUT || ISLOCKING)
			bf.l_len = 1;
	/* End XENIX Support */

	bf.l_pid = u.u_procp->p_epid;
	bf.l_sysid = u.u_procp->p_sysid;
	i = (fmode & (FNDELAY|FNONBLOCK)) ? INOFLCK : INOFLCK|SLPFLCK;
	i = reclock(vp, &bf, i, 0, offset, size);
	if (i || bf.l_type != F_UNLCK) {
		error = i ? i : EAGAIN;
	}

	/* XENIX Support */
	if (BADVISE_PRE_SV && (error == EAGAIN))
		error = EACCES;
	/* End XENIX Support */

	return (error);
}

/*
 * void
 * cleanlocks(vnode_t *vp, pid_t pid, sysid_t sysid)
 *	Clean up record locks left around by process.
 *
 * Calling/Exit State:
 *	The inode's rwlock is held exclusively on entry and exit.
 *
 * Description:
 *	Acquire the vnode's v_filocks_mutex before calling
 *	delflck() for any file/record lock contained in the
 *	list of file/record locks for the vnode that matches
 *	pid and sysid. Release v_filocks_mutex when through.
 */
void
cleanlocks(vnode_t *vp, pid_t pid, sysid_t sysid)
{
	filock_t *flp, *nflp, **lock_list;
	pl_t	 pl;

	pl = LOCK(&vp->v_filocks_mutex, FS_VPFRLOCKPL);

	lock_list = &vp->v_filocks;
	nflp = (struct filock *)0;
	for (flp = *lock_list; flp != NULL; flp = nflp) {
		nflp = flp->next;
		if (((flp->set.l_pid == pid) || (pid == IGN_PID))
		   && flp->set.l_sysid == sysid) {
			delflck(lock_list, flp);
		}
	}

	UNLOCK(&vp->v_filocks_mutex, pl);
}

/*
 * void
 * rm_sleeplcks(lck)
 *	Remove any lock blocked for lck from sleeplcks.
 *
 * Calling/Exit State:
 *	sleeplcks_mutex must not be held on entry.
 *
 * Description:
 *	Remove any lock(s) blocked for lck from sleeplcks.
 *	Since the NFS lock manager will add a lock to
 *	sleeplcks, but will not wait for it, we need to
 *	clean sleeplcks when the blocking lock is released.
 *	So lck is the blocking lock, and all locks present
 *	in sleeplcks for lck will be removed. Note that the
 *	NFS lock manager will retry the lock request and
 *	eventually get the lock.
 */
void
rm_sleeplcks(lck)
struct  flock   *lck;
{
	struct  filock  *sf;
	pl_t		opl;

	opl = LOCK(&sleeplcks_mutex, FS_SLPLOCKPL);

	sf = sleeplcks;
	while (sf != NULL) {
		/*
		 * if whence, start, len, bocking sysid, blocking pid
		 * all match, remove this lock from sleeplcks. note that
		 * we still need to check for non-zero l_sysid as we do
		 * not want to remove local blocked locks from sleeplcks,
		 * but only remote ones.
		 */
		if ((sf->set.l_whence == lck->l_whence) &&
				(sf->set.l_start >= lck->l_start) &&
				(sf->set.l_len <= lck->l_len) &&
				(sf->stat.blk.sysid == lck->l_sysid) &&
				(sf->stat.blk.pid == lck->l_pid) &&
				(sf->set.l_sysid != 0)) {
			delflck(&sleeplcks, sf);
			sf = sleeplcks;
		} else {
			sf = sf->next;
		}
	}

	UNLOCK(&sleeplcks_mutex, opl);
}
