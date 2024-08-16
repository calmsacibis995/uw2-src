/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5bmap.c	1.10"

#include <fs/buf.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5hier.h>
#include <fs/s5fs/s5filsys.h>
#include <mem/seg.h>
#include <mem/page.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/debug.h>

extern void pvn_fail(page_t *, int);
extern int s5_blkalloc(vfs_t *, daddr_t *);
extern int s5_blkfree(vfs_t *, daddr_t);

STATIC page_t *s5_bmap_lockpages(vnode_t *, off_t, size_t, page_t *,
			page_t **, daddr_t **);
STATIC void s5_bmap_unlockpages(page_t *, page_t **, daddr_t);
STATIC void s5_bmap_error_unlock(page_t *);

/*
 * s5_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp,
 *	daddr_t *db_list, daddr_t *io_list, int rw, cred_t *cr)
 *
 * 	s5_bmap defines the structure of file system storage by mapping
 * 	a logical block number in a file to a physical block number
 * 	on the device.
 *
 * Calling/Exit State:
 *	If the file size is being changed, the caller holds the
 *	inode's rwlock in *exclusive* mode.
 *
 *	Returns 0 on success, or a non-zero errorno if an error occurs.
 *
 * Description :
 *
 *	If the caller is s5_getapage, the existing block numbers are
 * 	returned via io_list for the caller to do io from the backing
 *	store to fill the page. The newly allocated or reallocated
 *	blocks are kept in the dblist in the page structure if they fit.
 *	For file systems with smaller logical blocks that wouldn't fit
 *	the dblist in the page,	this function will be called again by
 *	s5_putpage().
 *
 *	rw specifies whether the mapping is for read or write.
 *	If for write, the block must be at least size bytes and will be
 *	extended or allocated as needed. 
 *
 *	If the caller is s5_stablestore(), then ppp is NULL, the caller
 *	expects the backing store to be filled and returned via io_list.
 *
 *	Note: we are passing the exact range to bmappage(). Therefore,
 *	'off' is not page-aligned, nor is it block-aligned.
 *
 */
/* ARGSUSED */
int
s5_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp, daddr_t *db_list,
	daddr_t *io_list, int rw, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	vfs_t *vfsp;
	s5_fs_t *s5fsp;
	long bshift;
	off_t roff, eoff;
	daddr_t lbn, lastlbn;
	daddr_t wlbn, lastwlbn;
	daddr_t bn, ob, nb, *bap;
	page_t *rpl[MAXBSIZE/PAGESIZE + 1];
	page_t  *pp, **tmpp;
	buf_t *bp, *nbp;
	dev_t dev = ip->i_dev;
	long bsize = VBSIZE(vp);
	long old_isize;
	size_t size;
	int i, j, sh, nshift, error;
	boolean_t issync, alloc;
	boolean_t fix_isize = B_FALSE, page_locked;
	pl_t s;

	ASSERT(ppp == NULL || *ppp != NULL);

	pp = (ppp == NULL ? NULL : *ppp);

	old_isize = ip->i_size;

	/* Treat S_OVERWRITE like S_WRITE. */
	if (rw == S_OVERWRITE) {
		fix_isize = (off + len > old_isize);
		rw = S_WRITE;
	}

	vfsp = vp->v_vfsp;
	s5fsp = S5FS(vfsp);
	bshift = s5fsp->fs_bshift;

	lbn = pagerounddown(off) >> bshift;
	lastlbn = lbn + (PAGEOFFSET >> bshift);
	roff = lbn << bshift;
	eoff = off + len;

	if (rw == S_WRITE) {
		wlbn = off >> bshift;
		lastwlbn = (eoff - 1) >> bshift;
	}

	/*
	 * Round eoff up to the old isize so we can use it to
	 * quickly compute the end of backing store allocation.
	 */
	if (eoff < old_isize)
		eoff = old_isize;
	size = MIN(eoff - roff, bsize);

	issync = ((ip->i_flag & ISYNC) != 0);
	
	page_locked = alloc = B_FALSE;

	while (lbn < NADDR - 3) {

		nb = ob = ip->i_addr[lbn];
		if (ob == 0) {
			if (rw != S_WRITE || lbn < wlbn || lbn > lastwlbn)
				goto gotit;

			/*
			 * Lock all pages in the range.
			 */
			if (!page_locked) {
				if (pp &&
				   (pp = 
				      s5_bmap_lockpages(vp, roff, size, pp, rpl,
								&db_list)))
				*ppp = pp;
			    	page_locked = B_TRUE;
			}

			/* Allocate the block. */
			error = s5_blkalloc(vfsp, &nb);

			if (error) {
				if (pp) {
					tmpp = rpl;
					while (*tmpp != NULL)
						s5_bmap_error_unlock(*tmpp++);
				}
				return (error);
			}

			/*
			 * Check to see if the bmap state has changed
			 * while we were allocating backing store.
			 * If the state has changed, someone else won
			 * the race and has allocated the block already.
			 * We need to free the block we allocated.
			 */
			s = S5_ILOCK(ip);
			if (ip->i_addr[lbn] != ob) {
				S5_IUNLOCK(ip, s);
				ASSERT(pp != NULL);
				(void)s5_blkfree(vfsp, nb);
				nb = ip->i_addr[lbn];
				goto gotit;
			}

			ip->i_addr[lbn] = nb;
			IMARK(ip, IUPD|ICHG);
			S5_IUNLOCK(ip, s);
		}
gotit:
		*io_list = ob;
		*db_list = nb;

		if (fix_isize && roff + size > old_isize)
			old_isize = ip->i_size = roff + size;

		if (++lbn > lastlbn) 
			goto done;
		roff += bsize;
		size = MIN(eoff - roff, bsize);
		db_list++;
		io_list++;
	}

	/* Indirect blocks. */

	nshift = s5fsp->fs_nshift;
	while (lbn <= lastlbn) {

	    /*
	     * Determine how many levels of indirection.
	     */
	    sh = 0;
	    nb = 1;
	    bn = lbn - NDADDR;
	    for (j = NIADDR; j > 0; j--) {
	    	sh += nshift;
		nb <<= nshift;
	    	if (bn < nb)
	    		break;
	    	bn -= nb;
	    }

	    if (j == 0) {
		if (pp)
			s5_bmap_error_unlock(pp);
	    	return (EFBIG);
	    }

	    /*
	     * fetch the first indirect block
	     */
	    nb = ip->i_addr[NADDR - j];
	    if (nb == 0) {
		*io_list = DB_HOLE;
	    	if (rw != S_WRITE || lbn < wlbn || lbn > lastwlbn) {
cont_rdonly:
			*db_list = DB_HOLE;
			lbn++;
			roff += bsize;
			size = MIN(eoff - roff, bsize);
			db_list++;
			io_list++;
			continue;
	    	}

	        error = s5_blkalloc(vfsp, &nb);
	    	if (error) {
			if (pp)
				s5_bmap_error_unlock(pp);
	    		return (error);
	    	}

	    	/*
	    	 * Write synchronously so that indirect blocks
	    	 * never point at garbage.
	    	 */
	    	bp = getblk(dev, LTOPBLK(nb, bsize), bsize, 0);
		clrbuf(bp);
		(void)bwrite(bp);

		/*
		 * Check the state. If it hasn't changed, we can
		 * fill in the indirect block. Otherwise, someone
		 * raced with us and won.
		 */
		s = S5_ILOCK(ip);
		if (ip->i_addr[NADDR - j] != 0) {
			S5_IUNLOCK(ip, s);
			(void)s5_blkfree(vfsp, nb);
			nb = ip->i_addr[NADDR - j];
			goto got_indir;
		}

		ip->i_addr[NADDR - j] = nb;
		IMARK(ip, IUPD|ICHG);
		S5_IUNLOCK(ip, s);
	    }

got_indir:

	    /*
	     * fetch through the indirect blocks
	     */
	    for (; j < NIADDR; j++) {
	    	bp = bread(dev, LTOPBLK(nb, bsize), bsize);
		error = geterror(bp);
	    	if (error) {
	    		brelse(bp);
			if (pp)
				s5_bmap_error_unlock(pp);
	    		return (EIO);
	    	}
	    	bap = bp->b_un.b_daddr;
	    	sh -= nshift;
	    	i = (bn >> sh) & s5fsp->fs_nmask;
	    	nb = bap[i];
	    	if (nb == 0) {
	    		if (rw != S_WRITE || lbn < wlbn || lbn > lastwlbn) {
	    			brelse(bp);
				*io_list = DB_HOLE;
	    			goto cont_rdonly;
	    		}

	    	        error = s5_blkalloc(vfsp, &nb);
	    		if (error) {
	    			brelse(bp);
				if (pp)
					s5_bmap_error_unlock(pp);
	    			return (error);
	    		}

			/*
			 * Write synchronously so indirect blocks
			 * never point at garbage and blocks
			 * in directories never contain garbage.
			 */
			nbp = getblk(dev, LTOPBLK(nb, bsize), bsize, 0);
			clrbuf(nbp);
			(void)bwrite(nbp);

			bap[i] = nb;
			s = S5_ILOCK(ip);
			IMARK(ip, IUPD|ICHG);
			S5_IUNLOCK(ip, s);
	    		if (issync) {
	    			(void)bwrite(bp);
	    		} else {
	    			bdwrite(bp);
	    		}
	    	} else {
	    		brelse(bp);
	    	}
	    }

	    /* Last level of indirection. */
	    bp = bread(dev, LTOPBLK(nb, bsize), bsize);
	    error = geterror(bp);
	    if (error) {
	    	brelse(bp);
		if (pp)
			s5_bmap_error_unlock(pp);
	    	return (EIO);
	    }
	    bap = bp->b_un.b_daddr;
	    sh -= nshift;
	    i = (bn >> sh) & s5fsp->fs_nmask;

	    do {
		nb = bap[i];

		/* If it's not a hole, it's simple. */
	    	if (nb != 0 || rw != S_WRITE || lbn < wlbn || lbn > lastwlbn) {
			*db_list = *io_list = nb;
			goto indir_cont;
		}

		error = s5_blkalloc(vfsp, &nb);
		if (error) {
			brelse(bp);
			if (pp)
				s5_bmap_error_unlock(pp);
			return (error);
		}

		if (!page_locked) {
			if (pp &&
			   (pp = s5_bmap_lockpages(vp, roff, size, pp, rpl,
								&db_list)))
			*ppp = pp;

			page_locked = B_TRUE;
		}

		bap[i] = nb;
		s = S5_ILOCK(ip);
		IMARK(ip, IUPD|ICHG);
		S5_IUNLOCK(ip, s);
		alloc = B_TRUE;
		*io_list = DB_HOLE;
		*db_list = nb;

indir_cont:
		lbn++;
		roff += bsize;
		size = MIN(eoff - roff, bsize);
		db_list++;
		io_list++;
	    } while (lbn <= lastlbn && ++i < NINDIR(s5fsp));

	    if (alloc) {
		alloc = B_FALSE;
		if (issync) {
			(void)bwrite(bp);
		} else {
			bdwrite(bp);
		}
	    } else
		brelse(bp);
	}

done:
	if (pp && page_locked)
	    s5_bmap_unlockpages(pp, rpl, nb);
	if (fix_isize) {
	    ip->i_size = off + len;
	    s = S5_ILOCK(ip);
	    IMARK(ip, IUPD|ICHG);
	    S5_IUNLOCK(ip, s);
	}

	return 0;
}

/*
 * STATIC page_t *
 * s5_bmap_lockpages(vnode_t *vp, off_t roff, size_t size, page_t *pp,
 *		page_t *rpl[])
 *
 *	This function locks all pages in the [roff, roff+size) range.
 *	It returns the	center page to the caller.
 *
 * Calling/Exit State:
 *	On entry, 'pp' may be reader or writer locked.
 *	At exit, all pages in the range are locked.
 *
 * Description:
 *
 *	If 'pp' is a center page, we need to drop the page lock,
 *	get the front pages and then acquire the center page to
 *	avoid deadlock potential (locking out of order). The
 *	center page could be a different page.
 *
 */
/* ARGSUSED */
STATIC page_t *
s5_bmap_lockpages(vnode_t *vp, off_t roff, size_t size, page_t *pp,
		page_t *rpl[], daddr_t **db_listp)
{

	off_t eoff, center_off;

	eoff = roff + size;
	roff = pagerounddown(roff);

	/*
	 * If 'pp' is not center page, need to drop the page
	 * lock before going for the front page lock.
	 */
	if ((center_off = pp->p_offset) > roff) {
		if (PAGE_IS_WRLOCKED(pp))
			pvn_fail(pp, B_READ);
		else
			page_unlock(pp);
	} else {
		*rpl++ = pp;
		roff += PAGESIZE;
	}

	/* Get the rest of the pages if there are any. */
	while (roff < eoff && roff >= 0) {
		*rpl++ = page_lookup_or_create(vp, roff);

		if (roff == center_off && pp != NULL) {
			/*
			 * Center page. We have to return it to caller
			 * since we have unlocked it before trying
			 * to get the front pages.
			 */
			if (*db_listp == PG_DBLIST(pp))
				*db_listp = PG_DBLIST(rpl[-1]);
			pp = rpl[-1];
		}

		roff += PAGESIZE;
	}

	*rpl = NULL;

	return (pp);
}

/*
 * STATIC void
 * s5_bmap_unlockpages(page_t *center_pp, page_t *rpl[], daddr_t bn,
 *	 int db_index)
 *
 *	This function unlocks the pages in the bmapped block.
 *      It also fills in each page's dblist entry for this block
 *      and marks it modified (so the zeroed data will get written
 *      out to the newly allocated block).
 *
 * Calling/Exit State:
 *	At exit, pages in the rpl list except the center
 *	page are unlocked.
 *
 */
/* ARGSUSED */
STATIC void
s5_bmap_unlockpages(page_t *center_pp, page_t *rpl[], daddr_t bn)
{
	page_t *pp;


	while ((pp = *rpl++) != NULL) {

		if (pp != center_pp) {
			(PG_DBLIST(pp))[0] = bn;
			page_setmod_iowait(pp);
			if (PAGE_IS_WRLOCKED(pp))
				pagezero(pp, 0, PAGESIZE);

			page_unlock(pp);
		}
	}

}

/*
 * STATIC void
 * s5_bmap_error_unlock(page_t *pp)
 *
 *	This function unlocks the page if any error occurred.
 *
 * Calling/Exit State:
 *	At exit, the page is aborted if it is not initialized.
 *      otherwise it is unlocked.
 *
 */
/* ARGSUSED */
STATIC void
s5_bmap_error_unlock(page_t *pp)
{

	if (PAGE_IS_WRLOCKED(pp))
		pvn_fail(pp, B_READ);
	else
		page_unlock(pp);
}

