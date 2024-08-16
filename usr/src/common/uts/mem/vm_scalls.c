/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *              PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *              Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *      (c) 1986,1987,1988,1989  Sun Microsystems, Inc
 *      (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *                All rights reserved.
 *
 */

#ident	"@(#)kern:mem/vm_scalls.c	1.20"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/lock.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/ublock.h>
#include <mem/vmparam.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * VM system calls
 */

struct mmapa {
	caddr_t	addr;
	size_t	len;
	int	prot;
	int	flags;
	int	fd;
	off_t	pos;
};

/*
 * int 
 * mmap(struct mmapa *uap, rval_t *rvp)
 *	mmap system call: make the contents of a file (pointed to by 
 *      uap->fd) into the address space of the current process.
 *
 * Calling/Exit State:
 *	No locks are held on entry to the function and none are held on
 *	return. 
 * 
 *	On success, zero is returned and the file contents indicated by 
 *	uap->fd starting at uap->pos are mapped into the current address 
 *	space starting at the address returned in r_val1 (may or may not be 
 *	equal to the address passed in uap->addr) for uap->len bytes (rounded 
 *	up to the next page boundary. If uap->flags indicates a fixed mapping 
 *	(MAP_FIXED) then any mappings which may have previously existed in the 
 *	specifed address range will be replaced by the new mapping.
 *
 *	On failure, a non-zero errno is returned and the address space contents
 *	remain unchanged.
 *
 * Remarks:
 *	If the filesystem of the target file supports mapping then the 
 *	VOP_MAP operation will take care of write locking the address 
 *	space to insure sanity.
 *
 *	Since no special checks are made for remapping text and data it is
 *	possible to royally hose the process image. 
 */
int 
mmap(struct mmapa *uap, rval_t *rvp)
{
	vaddr_t addr;
	int errno;

	if ((addr = mmap_k((vaddr_t)uap->addr, uap->len, 
			   uap->prot | PROT_USER, uap->flags, uap->fd, 
			   uap->pos, &errno)) == (vaddr_t)-1)
		return errno;

	rvp->r_val1 = (int)addr;	/* return starting address */
	return (0);
}

/*
 * vaddr_t
 * mmap_k(vaddr_t addr, size_t len, int prot, int flags,
 *        int fd, off_t off, int *errno)
 *
 *      Mmap interface for callers from kernel space.
 *
 * Calling/Exit State:
 *      No locks held on entry or exit.
 *
 *      On success, the mapped address is returned.
 *      On failure, -1 is returned and the outarg errno contains the error
 *	number.
 *
 *      Called in process context.
 *
 * Remarks:
 *      This call is similar to the mmap system call except that this
 *      is called from kernel space. Unlike mmap(), however, user mappings
 *      need to be explicitly specified by PROT_USER. Otherwise, mappings
 *      will be set up for supervisory access only.
 */
/* ARGSUSED */
vaddr_t
mmap_k(vaddr_t addr, size_t len, int prot, int flags, int fd, off_t off,
	int *errno)
{
	vnode_t *vp;
	struct as *as = u.u_procp->p_as;
	struct file *fp;
	u_int maxprot;
	u_int type;
	int error;

	type = flags & MAP_TYPE;

	if ((flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_FIXED 
	    | MAP_NORESERVE	/* not implemented, but don't fail here */
	    /* | MAP_RENAME */	/* not yet implemented, let user know */
	    )) != 0) {
		*errno = EINVAL;
		return (vaddr_t)-1;
	}

	if (type != MAP_PRIVATE && type != MAP_SHARED) {
		*errno = EINVAL;
		return (vaddr_t)-1;
	}

	/*
	 * Check for bad lengths and file position.
	 * We let the VOP_MAP routine check for negative lengths
	 * since on some vnode types this might be appropriate.
	 */
	if ((off & PAGEOFFSET) != 0) {
		*errno = EINVAL;
		return (vaddr_t)-1;
	}

	/*
	 * If the user specified an address, do some simple checks here
	 */
	if ((flags & MAP_FIXED) != 0) {
		/*
		 * Use the user address.  First verify that
		 * the address to be used is page aligned.
		 * Then make some simple bounds checks.
		 */
		if ((addr & PAGEOFFSET) != 0) {
			*errno = EINVAL;
			return (vaddr_t)-1;
		}
		if (VALID_USR_RANGE(addr, len) == 0) {
			*errno = ENOMEM;
			return (vaddr_t)-1;
		}
	}

	if (error = getf(fd, &fp)) {
		*errno = error;
		return (vaddr_t)-1;
	}

	vp = fp->f_vnode;

	maxprot = PROT_ALL;		/* start out allowing all accesses */

	if (type == MAP_SHARED && (fp->f_flag & FWRITE) == 0) {
		/* no write access allowed */
		maxprot &= ~PROT_WRITE;
	}

	/*
	 * XXX - Do we also adjust maxprot based on protections
	 * of the vnode?  E.g. if no execute permission is given
	 * on the vnode for the current user, maxprot probably
	 * should disallow PROT_EXEC also?  This is different
	 * from the write access as this would be a per vnode
	 * test as opposed to a per fd test for writability.
	 */

	/*
	 * Verify that the specified protections are not greater than
	 * the maximum allowable protections.  Also test to make sure
	 * that the file descriptor does allows for read access since
	 * "write only" mappings are hard to do since normally we do
	 * the read from the file before the page can be written.
	 */
	if (((maxprot & prot) != prot) || (fp->f_flag & FREAD) == 0) {
		FTE_RELE(fp);
	  	*errno = EACCES;
		return (vaddr_t)-1;
	}

	/*
	 * Ok, now let the vnode map routine do its thing to set things up.
	 */
	error = VOP_MAP(vp, off, as, &addr, len, prot, maxprot, flags,
			fp->f_cred);

	FTE_RELE(fp);

	if (error) {
	  	*errno = error;
		return (vaddr_t)-1;
	}

	return addr;
}


struct munmapa {
	caddr_t	addr;
	size_t	len;
};

/*
 * int 
 * munmap(struct munmapa *uap, rval_t *rvp)
 *	munmap system call: unmap the address space range indicated
 *	from the current process.
 *
 * Calling/Exit State:
 *	No locks are held on entry to the function and none are held on
 *	return.
 *
 *	The address space is held write locked across the call to as_unmap
 *	to insure the results are sane.
 *
 *	On success, zero is returned and the address space starting at
 *	uap->addr for uap->len bytes (rounded up to the next page boundary)
 *	will be unmapped.
 *
 *	On failure, a non-zero errno (EINVAL) is returned and the address
 *	space contents remain unchanged. 
 *
 * Remarks:
 *	Since no special checks are made for unmapping text and data it is
 *	possible to royally hose the process image. 
 */
/* ARGSUSED */
int 
munmap(struct munmapa *uap, rval_t *rvp)
{
	return munmap_k((vaddr_t)uap->addr, uap->len);
}


/*
 * int 
 * munmap_k(vaddr_t addr, size_t  len)
 *
 *	Munmap interface for callers from kernel space.
 *
 * Calling/Exit State:
 *	No locks are held on entry to the function and none are held on
 *	return.
 *
 * 	On success, zero is returned.
 *	On failure, a non-zero errno is returned and the address
 * 	space contents remain unchanged.
 *
 * Remarks:
 *	This call is similar to the munmap system call except that this
 *	is called from kernel space.
 */
/* ARGSUSED */
int 
munmap_k(vaddr_t addr, size_t  len)
{
	proc_t *p = u.u_procp;

	if ((addr & PAGEOFFSET) != 0) {
		return(EINVAL);
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return(EINVAL);
	}

	as_wrlock(p->p_as);

	if (as_unmap(p->p_as, addr, len) != 0) {
		as_unlock(p->p_as);
		return(EINVAL);
	}

	as_unlock(p->p_as);

	return (0);
}



struct mprotecta {
	caddr_t	addr;
	size_t	len;
	int	prot;
};

/*
 * int
 * mprotect(struct mprotecta *uap, rval_t *rvp)
 *	Change the permissions on the specified portion of the current
 *	address space within the limits established by initial mapping
 *	for that portion of the address space.
 *
 * Calling/Exit State:
 *	No locks are held on entry to the function and none are held on
 *	return.
 *
 *      During the operation, the address space is held write locked.
 *
 *	On success, zero is returned and the requested permissions have
 *	become effective on the specified range of addresses.
 *
 *	On failure, a non-zero errno is returned and the permissions of 
 *	of the entire specified range may or may not be modified as per 
 *	the request. If the specified range spans more than a single underlying
 *	memory object (i.e. segment) then mprotect can partially fail for the 
 *	following reasons: 
 *
 *		- if not all of the underlying segment managers support the 
 *	          SOP_SETPROT() request. 
 *
 *	OR:
 *
 * 		- if the maximum protection allowed on one or more of the 
 *		  spanned segments would be exceeded by the request. 
 *
 *	OR:
 *
 *		- if write permissions are added to a vnode-backed 
 *		  MAP_PRIVATE/RO segment which has one or more memory
 *		  locked pages in it and memory cannot be reserved for
 *		  the new private copy of the page. (COW is broken by
 *		  the segment manager at the time of the setprot request.)
 *
 * 	As currently designed, no back-off is performed and in failures as 
 *	described above, part of the total requested range may have been 
 *	changed. There is no way to determine how effective the request may 
 *	have been in these cases since there is no mgetprot() system call or
 *	useful result code returned.
 *
 * Remarks:
 *	There should either be an mgetprot() system call, a way of returning
 *	the effectivity of the request to the caller, or a way of backing
 *	partially failed requests out. 
 */
/* ARGSUSED */
int
mprotect(struct mprotecta *uap, rval_t *rvp)
{
	vaddr_t addr = (vaddr_t)uap->addr;
	size_t len = uap->len;
	u_int prot = uap->prot | PROT_USER;
	struct as *as;
	int error;

	if (((int)addr & PAGEOFFSET) != 0) {
		return(EINVAL);
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return(ENOMEM);
	}

	as = u.u_procp->p_as;
	as_wrlock(as);
	error = as_setprot(as, addr, len, prot);
	as_unlock(as);
	return(error);
}

#define MC_CACHE        128                     /* internal result buffer */
#define MC_QUANTUM      (MC_CACHE * PAGESIZE)   /* addresses covered in loop */

struct mincorea {
	caddr_t addr;
	size_t	len;
	char	*vec;
};

/*
 * int
 * mincore(struct mincorea *uap, rval_t *rvp)
 *	Determine whether or not pages in the current address space are
 *	loaded and if so, whether or not they are memory locked.
 *
 * Calling/Exit State:
 *	No locks are held on entry to the function and none are held on
 *	return.
 *
 *	During the operation, the address space is held read locked to 
 *	prevent constitional changes occuring in parallel. This is not
 *	expected to be a bottleneck as page faults can still occur in
 *	parallel (to the extent that the AS layer does not preclude them -
 * 	underlying segment managers may or may not impose serialization 
 *	of their own). 
 *
 *	On success, zero is returned and the array pointed to by uap->addr
 *	contains the incore state information for the requested range of
 *	pages. The least significant bit of each char in the array will be
 *	set to one for resident pages and zero for non-resident pages. Other
 *	bits may be set by underlying segment managers but this cannot be
 *	counted on by the caller.  
 *
 *	On failure, a non-zero errno is returned and the contents of the 
 *	array pointed to by uap->addr is left unchanged.
 *	
 * Remarks:
 *	In a multi-threaded AS, the information returned is not guaranteed
 *	to be accurate. 
 *
 *	Note that we use a (relatively) small static buffer. This is done
 *	rather than calling kmem_alloc with good reason. If the caller
 *	passed a `bogus' size like 10M, we may never come out of kmem_alloc
 *	and even if we did we would be hogging a lot of memory. Since there
 *	is no reasonable way to do range checking on the user's len (where's
 *	the cutoff?) we `chunk up' the request.
 */
/* ARGSUSED */
int
mincore(struct mincorea *uap, rval_t *rvp)
{
	vaddr_t addr = (vaddr_t)uap->addr;
	vaddr_t ea;
	struct as *as;			/* address space */
	char vec[MC_CACHE];
	uint_t	len = uap->len;
	

	/*
	 * Validate form of address parameters.
	 */
	if (((int)addr & PAGEOFFSET) != 0) {
		return(EINVAL);
	}

	if (VALID_USR_RANGE(addr, len) == 0) {
		return(ENOMEM);
	}

	/*
	 * privilege required on a system running MAC
	 * to avoid a potential covert channel.
	 */
	if (mac_installed && pm_denied(CRED(), P_SYSOPS))
		return(EPERM);

	as = u.u_procp->p_as;

	for (ea = addr + len; addr < ea; addr += MC_QUANTUM) {
		uint_t minlen;
		uint_t rl;
		int error;

		as_rdlock(as);

		minlen = MIN(MC_QUANTUM, ea - addr);
		error = as_incore(as, addr, minlen, vec, &rl); 

		ASSERT(error || rl == minlen);

		/*
		 * Can't hold AS locked across copyout.
		 */

		as_unlock(as);

                if (rl != 0) {
                        rl = (rl + PAGESIZE - 1) / PAGESIZE;
                        if ((error = copyout(vec, uap->vec, rl)) != 0)
				return(EFAULT);
                        uap->vec += rl;
                }

                if (error != 0) 
                        return(ENOMEM);
	}
	return(0);
}


struct madvisea {
	caddr_t	addr;
	size_t len;
	int behav;
};

/*
 * int
 * madvise(struct madvisea *uap, rval_t *rvp)
 *	Give free advice to the system about paging behavour of the current
 *	process. The advice is considered 'free' because the system is free
 *	to ignore it.
 *
 * Calling/Exit State:
 *	No special locks are held on entry or exit. 
 *
 *	The address and length are validated by calling VALID_USR_RANGE().
 *
 *	On success, zero is returned. On failure a non-zero errno is returned.
 *	
 * Remarks:
 *	Vanilla ES/MP does not support these operations but they may be
 *	implemented as added-value by a vendor.
 */
/* ARGSUSED */
int
madvise(struct madvisea *uap, rval_t *rvp)
{
	caddr_t addr = uap->addr;
	size_t len = uap->len;
	int error = 0;

	if (VALID_USR_RANGE(addr, len) == 0) {
		return(ENOMEM);
	}

	switch (uap->behav) {

	case MADV_NORMAL:
	case MADV_RANDOM:
	case MADV_SEQUENTIAL:
	case MADV_WILLNEED:
	case MADV_DONTNEED:
		break;

	default:
		/* unknown behavior value */
		error = EINVAL;
		break;

	}
	return(error);
}

struct locka {
        int op;
};

/*
 * int
 * lock_mem(struct locka *uap, rval_t *rvp)
 *	System call for locking all or part of the calling process'
 *	address space.
 *
 * Calling/Exit State:
 * 	No special calling or exit state required. This call will internally
 *	acquire and release a write lock on the address.
 *
 * 	The caller must have appropriate priviledges to perform the operation.
 *
 *	On success, 0 is returned and the requested portion(s) of the AS
 *	are locked into core. Any COW pages will have been privitized as
 *	part of this operation.
 *
 *	On failure, a non-zero errno is returned.
 * 
 * Remarks:
 *	Since ES/MP chose to name its simplest locking primitive LOCK and
 *	UNLOCK and since plock(2) also uses UNLOCK (and none of the very
 *	smart developers involved noticed this) we have a slight problem
 *	on our hands. We can't change the name of the user-mode #define
 *	because of compatibility reasons. Therefore, we only #define UNLOCK
 *	for !_KERNEL in lock.h and hard-wire a 0 into the switch below.
 *
 */
/* ARGSUSED */
int
lock_mem(struct locka *uap, rval_t *rvp)
{
        int error;

        if (pm_denied(CRED(), P_PLOCK))
                return EPERM;

	as_wrlock(u.u_procp->p_as);

        switch (uap->op) {
        case TXTLOCK:
                if (u.u_procp->p_plock & (PROCLOCK|TXTLOCK))
                        error = EINVAL;
                else
                        error = textlock();
                break;

        case PROCLOCK:
                if (u.u_procp->p_plock & (PROCLOCK|TXTLOCK|DATLOCK))
                        error = EINVAL;
                else
                        error = proclock();
                break;

        case DATLOCK:
                if (u.u_procp->p_plock & (PROCLOCK|DATLOCK))
                        error = EINVAL;
                else
                        error = datalock();
                break;

        case 0:	/* UNLOCK */
		if (!(u.u_procp->p_plock & (PROCLOCK|TXTLOCK|DATLOCK)))
                        error = EINVAL;
		else {
			punlock();
			error = 0;
		}
                break;

        default:
                error = EINVAL;
        }

	as_unlock(u.u_procp->p_as);

        return error;
}

struct memcntla {
        caddr_t addr;
        size_t  len;
        int     cmd;
        caddr_t arg;
        int     attr;
        int     mask;
};

/*
 * int
 * memcntl(struct memcntla *uap, rval_t *rvp)
 *	System call to perform assorted operations on the current 
 *	address space: locking (MC_LOCK|MC_LOCKAS), unlocking (MC_UNLOCK|
 *	MC_UNLOCKAS), and syncing to disk (MC_SYNC).
 *
 * Calling/Exit State:
 *	No special MP state is required on entry or is held on exit. The
 *	current address space is write locked before calling out to perform
 *	the requested operation. Acquires p_mutex of the current process to 
 *	handle races between this request and calls to lwplock or lwpunlock.
 *
 *	On success, zero is returned and the request operation has been
 *	performed.
 *
 *	On failure, a non-zero errno is returned to indicate the exact
 *	problem. If uap->cmd specified a memory locking operation on 
 *	all or part of a previously locked range of pages, those pages
 *	will now be unlocked. This is consistent with the SVR3/4 semantic.
 *
 *
 * Remarks:
 *	In the SVR4 version of this function backout from failed memory
 *	locking requests was performed at this level. This required the
 *	allocation of a bit-map which was passed down vi as_ctl to individual
 *	segment managers. This was filled out on a bit-per-page-locked basis
 *	so memcntl could know which of the request range to unlock on error.
 *	In ES/MP this kludgery has been replaced and the segment manager
 *	performs any backing out if needed. 
 *
 */
/* ARGSUSED */
int
memcntl(struct memcntla *uap, rval_t *rvp)
{
	struct seg *seg;		/* working segment */
	struct seg *sseg;		/* starting segment */
	u_int rlen = 0;			/* rounded as length */
	addr_t raddr;			/* rounded address counter */
	caddr_t addr = uap->addr;
	u_int len = uap->len;
	int attr = uap->attr;
	caddr_t arg = uap->arg;
	struct as *as_pp = u.u_procp->p_as;  /* address space pointer */
	int error = 0;

	if (uap->mask)
		return EINVAL;
	if ((uap->cmd == MC_LOCKAS) || (uap->cmd == MC_UNLOCKAS)) {
		if ((addr != 0) || (len != 0)) {
			return EINVAL;
		}
	} else {
		if (((int)addr & PAGEOFFSET) != 0)
			return EINVAL;
		if (VALID_USR_RANGE(addr, len) == 0)
			return ENOMEM;
	}

	if ((VALID_ATTR & attr) != attr)
		return EINVAL;

	if ( (attr & SHARED) && (attr & PRIVATE) )
		return EINVAL;

	if ( ((uap->cmd == MC_LOCKAS) || (uap->cmd == MC_LOCK) ||
	     (uap->cmd == MC_UNLOCKAS) || (uap->cmd == MC_UNLOCK))
		    && (pm_denied(CRED(), P_PLOCK)) )
		return EPERM;

	if (attr)
		attr |= PROT_USER;

	/*
	 * With the exception of MC_SYNC and the default cases (which
	 * simply return from within the switch), all requests ``break'' 
	 * from this switch with the AS write locked.
	 */

	switch (uap->cmd) {
	case MC_SYNC:
		if ((int)arg & ~(MS_ASYNC|MS_INVALIDATE))
			return EINVAL;
		else {
			/*
			 * Invalidation requires privilege on a system
			 * running MAC to avoid a potential covert channel.
			 */
			if (mac_installed && ((int)arg & MS_INVALIDATE) &&
			    pm_denied(CRED(), P_PLOCK))
				return EPERM;

			as_wrlock(as_pp);
			error = as_ctl(as_pp, (vaddr_t)addr, len, 
				       uap->cmd, attr, (void *)arg);
			as_unlock(as_pp);

		}
		return error;
	case MC_LOCKAS:
		if ((int)arg & ~(MCL_FUTURE|MCL_CURRENT) || (int)arg == 0)
			return EINVAL;

		as_wrlock(as_pp);

		/* lock the ublock in memory */
		if (!(u.u_procp->p_plock & MEMLOCK)) {
			if (!(u.u_procp->p_plock & PROCLOCK)) {
				if ((error = ublock_lock(u.u_procp,
						 UB_NOSWAP_USER)) != 0) {
					as_unlock(as_pp);
					return error;
				}
				(void) LOCK(&u.u_procp->p_mutex, PLHI);
				ASSERT(!(u.u_procp->p_flag & P_NOSWAP));
				u.u_procp->p_flag |= P_NOSWAP;
				UNLOCK(&u.u_procp->p_mutex, PLBASE);
			}
			u.u_procp->p_plock |= MEMLOCK;
		}

		sseg = seg = as_pp->a_segs;
		if (seg == NULL) {
			as_unlock(as_pp);
			return 0;
		}
		
		/*
		 * Determine the base of the address space, sum the 
		 * length and pass it all off to as_ctl below.
		 */

		do {
			raddr = (addr_t)((u_int)seg->s_base & PAGEMASK);
			rlen += (((u_int)(seg->s_base + seg->s_size) +
				PAGEOFFSET) & PAGEMASK) - (u_int)raddr;
		} while ((seg = seg->s_next) != sseg);
			
		break;
	case MC_LOCK:
		/*
		 * Normalize addresses and lengths
		 */
		raddr = (addr_t)((u_int)addr & PAGEMASK);
		rlen  = (((u_int)(addr + len) + PAGEOFFSET) & PAGEMASK) -
				(u_int)raddr;

		as_wrlock(as_pp);

		break;
	case MC_UNLOCKAS:
		as_wrlock(as_pp);

		/* remove claim for ublock */
		if (u.u_procp->p_plock & MEMLOCK) {
			if (!(u.u_procp->p_plock & PROCLOCK)) {
				ublock_unlock(u.u_procp, UB_NOSWAP_USER);
				(void) LOCK(&u.u_procp->p_mutex, PLHI);
				u.u_procp->p_flag &= ~P_NOSWAP;
				UNLOCK(&u.u_procp->p_mutex, PLBASE);
			}
			u.u_procp->p_plock &= ~MEMLOCK;
		}

		break;
	case MC_UNLOCK:
		as_wrlock(as_pp);
		break;
	default:
		return EINVAL;
	}

	/*
	 * AS is write locked at this point
	 */

	error = as_ctl(as_pp, (vaddr_t)addr, len, uap->cmd, attr, (void *)arg);
	if (error && uap->cmd == MC_LOCKAS) {
		/* backout claim on ublock */
		ASSERT(u.u_procp->p_plock & MEMLOCK);
		if (!(u.u_procp->p_plock & PROCLOCK)) {
			ublock_unlock(u.u_procp, UB_NOSWAP_USER);
			(void) LOCK(&u.u_procp->p_mutex, PLHI);
			u.u_procp->p_flag &= ~P_NOSWAP;
			UNLOCK(&u.u_procp->p_mutex, PLBASE);
		}
		u.u_procp->p_plock &= ~MEMLOCK;
	}
	as_unlock(as_pp);

	return error;
}
