/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/memfs/memfs_subr.c	1.9"

#include <acc/priv/privilege.h>
#include <fs/memfs/memfs.h>
#include <fs/memfs/memfs_hier.h>
#include <fs/memfs/memfs_mnode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <proc/cred.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int memfs_kmemspace;

int memfsfstype;

/*
 * unique timestamp for mnode updates
 */
static timestruc_t mem_uniqtime;
extern time_t time;

/*
 * void
 * memfs_timestamp(mnode_t *mp)
 *	Update the mnode timestamps based upon the flags.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
void
memfs_timestamp(mnode_t *mp)
{
	int flag;
	boolean_t vnmod;

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	vnmod = VN_CLRMOD(MNODE_TO_VP(mp));

	FSPIN_LOCK(&mp->mno_mutex);
	flag = mp->mno_flag;
	if (hrestime.tv_sec > mem_uniqtime.tv_sec ||
	    hrestime.tv_nsec > mem_uniqtime.tv_nsec) {
		mem_uniqtime.tv_sec = hrestime.tv_sec;
		mem_uniqtime.tv_nsec = hrestime.tv_nsec;
	} else {
		mem_uniqtime.tv_nsec++;
	}
	if (flag & TACC) {
		mp->mno_atime.tv_sec = mem_uniqtime.tv_sec;
		mp->mno_atime.tv_usec = mem_uniqtime.tv_nsec / 1000;
	}
	if ((flag & TUPD) || vnmod) {
		mp->mno_mtime.tv_sec = mem_uniqtime.tv_sec;
		mp->mno_mtime.tv_usec = mem_uniqtime.tv_nsec / 1000;
	}
	if ((flag & TCHG) || vnmod) {
		mp->mno_ctime.tv_sec = mem_uniqtime.tv_sec;
		mp->mno_ctime.tv_usec = mem_uniqtime.tv_nsec / 1000;
	}
	mp->mno_flag &= ~(TACC|TUPD|TCHG);
	FSPIN_UNLOCK(&mp->mno_mutex);
}

#define TST_GROUP       3
#define TST_OTHER       6


/*
 * int
 * memfs_taccess(mnode_t *mp, int mode, struct cred *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
int
memfs_taccess(mnode_t *mp, int mode, struct cred *cred)
{
	int	denied_mode;
	int	lshift;
	int	i;

	if (cred->cr_uid == mp->mno_uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(mp->mno_gid, cred)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	}
	else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (mp->mno_mode & mode)) == mode) {
		return 0;
	}
	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cred, P_DACREAD))
		return (EACCES);
	if ((denied_mode & IWRITE) && pm_denied(cred, P_DACWRITE))
		return (EACCES);
	return (0);
}

/*
 * void *
 * memfs_kmemalloc(mem_vfs_t *mem_vfsp, u_int size, int flags)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	memfs kernel memory allocation
 * 	Does some bookkeeping in addition to calling kmem_zalloc().
 *
 * 	NULL returned on failure.
 */
void *
memfs_kmemalloc(mem_vfs_t *mem_vfsp, u_int size, int flags)
{
	void *cp;
	pl_t pl;

	FSPIN_LOCK(&memfs_mutex);
	if (memfs_kmemspace + size < memfs_maxkmem) {
		memfs_kmemspace += size;
		FSPIN_UNLOCK(&memfs_mutex);

		pl = LOCK(&mem_vfsp->mem_contents, FS_VFSPPL);
		mem_vfsp->mem_kmemspace += size;
		UNLOCK(&mem_vfsp->mem_contents, pl);

		cp = kmem_zalloc(size, flags);
		return (cp);
	}
	FSPIN_UNLOCK(&memfs_mutex);
	/*
	 *+ Mounted memfs file system is overflowing its KMA limit.
	 *+ Corrective action - remove unused file/directories.
	 */
	cmn_err(CE_WARN, "memfs_kmemalloc: memfs over kma limit\n");
	return (NULL);
}

/*
 * void
 * memfs_kmemfree(mem_vfs_t *mem_vfsp, void *cp, u_int size)
 *
 * Calling/Exit State:
 *
 *
 * Description:
 * 	Does some bookkeeping in addition to calling kmem_free().
 */
void
memfs_kmemfree(mem_vfs_t *mem_vfsp, void *cp, u_int size)
{
	pl_t pl;

	pl = LOCK(&mem_vfsp->mem_contents, FS_VFSPPL);
	if (mem_vfsp->mem_kmemspace < size)
		/*
		 *+ When freeing KMA, it was found that the KMA space used
		 *+ by a mounted memfs was going negative. This indicates
		 *+ a software bug in KMA accounting. Corrective action -
		 *+ reboot.
		 */
		cmn_err(CE_PANIC, "memfs_kmemfree: kmem %d size %d memfs_kmem %d\n",
		    mem_vfsp->mem_kmemspace, size, memfs_kmemspace);
	mem_vfsp->mem_kmemspace -= size;
	UNLOCK(&mem_vfsp->mem_contents, pl);

	kmem_free(cp, size);

	FSPIN_LOCK(&memfs_mutex);
	memfs_kmemspace -= size;
	FSPIN_UNLOCK(&memfs_mutex);
}

/*
 * long
 * memfs_mapalloc(mem_vfs_t *tm)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Allocate and return an unused number as part of the id
 *	for a named mnode.
 * 
 */
long
memfs_mapalloc(mem_vfs_t *mem_vfsp)
{
	struct mnode_map *tmapp, *ntmapp;
	struct mnode_map *atmapp = NULL;
	int i, id;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	i = 0;
	(void) LOCK(&mem_vfsp->mem_contents, FS_VFSPPL);
	tmapp = mem_vfsp->mem_mnomap;
	for (;;) {
		for (id = 0; id < MNOMAPNODES; id++)
			if (!TESTBIT(tmapp->mmap_bits, id))
				break;
		if (id < MNOMAPNODES) {
			SETBIT(tmapp->mmap_bits, id);
			UNLOCK(&mem_vfsp->mem_contents, PLBASE);

			/*
			 * If we have an extra mnode_map block,
			 * then release it now.
			 */
			if (atmapp)
				memfs_kmemfree(mem_vfsp, atmapp,
					       sizeof(*atmapp));

			return ((i * MNOMAPNODES) + id);
		}
		ntmapp = tmapp->mmap_next;
		if (ntmapp == NULL) {
			if (atmapp != NULL) {
				tmapp->mmap_next = ntmapp = atmapp;
				atmapp = NULL;
			} else {
				/*
				 * We must drop the mem_contents mutex
				 * in order to potentially sleep for
				 * memory.
				 */
				UNLOCK(&mem_vfsp->mem_contents, PLBASE);
				atmapp = memfs_kmemalloc(mem_vfsp,
							 sizeof(*atmapp),
							 KM_SLEEP);
				if (atmapp == NULL)
					return (-1);

				i = 0;
				(void) LOCK(&mem_vfsp->mem_contents,
					    FS_VFSPPL);
				tmapp = mem_vfsp->mem_mnomap;
				continue;
			}
		}
		tmapp = ntmapp;
		++i;
	}
	/* NOTREACHED */
}

/*
 * void
 * memfs_mapfree(mem_vfs_t *mem_vfsp, ino_t number)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Free inumber of a memfs file that is being destroyed.
 *
 */
void
memfs_mapfree(mem_vfs_t *mem_vfsp, ino_t number)
{
	int i;
	struct mnode_map *tmapp;
	pl_t pl;

	pl = LOCK(&mem_vfsp->mem_contents, FS_VFSPPL);
	for (i = 1, tmapp = mem_vfsp->mem_mnomap; tmapp;
	    i++, tmapp = tmapp->mmap_next) {
		if (number < i * MNOMAPNODES) {
			CLEARBIT(tmapp->mmap_bits, (number % MNOMAPNODES));
			UNLOCK(&mem_vfsp->mem_contents, pl);
			return;
		}
	}
	UNLOCK(&mem_vfsp->mem_contents, pl);
	/*
	 *+ An inumber that is to be freed failed to be found in the
	 *+ mnode map. This indicates a software bug.
	 *+ Corrective action - cleanup the file system, unmount and remount.
	 */
	cmn_err(CE_WARN, "memfs_mapfree: Couldn't free nodeid %d\n", number);
}

