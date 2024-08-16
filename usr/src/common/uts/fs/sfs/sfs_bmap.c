/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_bmap.c	1.30"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <fs/buf.h>
#include <proc/disp.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <mem/seg.h>
#include <mem/page.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_fs.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <fs/vfs.h>
#include <util/cmn_err.h>
#include <util/debug.h>

extern int sfs_alloc(inode_t *, daddr_t, int, daddr_t *, cred_t *);
extern int sfs_free(inode_t *, daddr_t, off_t);
extern int sfs_realloccg(inode_t *, daddr_t, daddr_t, int, int,
			 daddr_t *, cred_t *);
extern void pvn_fail(page_t *, int);
extern int sfs_chkdq(inode_t *, long, int, cred_t *);
STATIC page_t *sfs_bmap_lockpages(vnode_t *, off_t, size_t, page_t *,
			page_t **, daddr_t **);
STATIC void sfs_bmap_unlockpages(page_t *, page_t **, daddr_t);
STATIC void sfs_bmap_error_unlock(page_t *);

/*
 * int
 * sfs_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp,page_t *rpl[],
 *	daddr_t *db_list, daddr_t *io_list, int rw, cred_t *cr)
 *
 *	This function defines the structure of file system storage by mapping
 *	a logical block number in a file to a physical block number
 *	on the device.
 *
 * Calling/Exit State:
 *
 *	If the file size is being changed, the caller holds the inode's
 *	rwlock in *exclusive* mode.
 *
 *	Returns 0 on success, or a non-zero errno if an error occurs.
 *
 * Description:
 *	Translates logical block number lbn to a physical block
 *	number.
 *
 *	If the caller is sfs_getpage() or sfs_getapage,
 *	the old block numbers are returned via io_list
 *	for the caller to do io from the backing store to fill
 *	the page. The newly allocated or reallocated blocks are kept in the
 *	dblist in the page structure if they fit. For file system with
 *	smaller logical blocks that wouldn't fit the dblist in the page,
 *	this function will be called again by sfs_putpage().
 *	rw specifies whether the mapping is for read or write.
 *	If for write, the block must be at least size bytes and will be
 *	extended or allocated as needed. 
 *
 *	If the caller is sfs_stablestore(), then both ppp and rpl are
 *	NULL. The caller expects the backing
 *	store to be filled and returned via io_list.
 *
 *	Note: we are passing the exact range to bmappage(). Therefore,
 *	'off' is not page-aligned, nor is it block-aligned.
 * 
 */
/* ARGSUSED */
int
sfs_bmappage(vnode_t *vp, off_t off, size_t len, page_t **ppp, page_t *rpl[],
	daddr_t *db_list, daddr_t *io_list, int rw, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	fs_t *fs;
	long bshift;
	off_t roff, eoff;
	daddr_t lbn, lastlbn;
	daddr_t wlbn, lastwlbn;
	daddr_t pref, bn, ob, nb, *bap;
	page_t *local_rpl[MAXBSIZE/PAGESIZE + 1];
	page_t  *pp, **tmpp;
	buf_t *bp, *nbp;
	dev_t dev = ip->i_dev;
	long bsize;
	long old_isize;
	size_t size, osize, nsize;
	int i, j, sh, error;
	pl_t opl;
	boolean_t issync, alloc;
	boolean_t fix_isize = B_FALSE, page_locked, alloc_only;

	ASSERT(ppp == NULL || *ppp != NULL);

	pp = (ppp == NULL ? NULL : *ppp);

	old_isize = ip->i_size;

	fs = ip->i_fs;
	bshift = fs->fs_bshift;
	bsize = fs->fs_bsize;

	/* Treat S_OVERWRITE like S_WRITE. */
	if (rw == S_OVERWRITE) {
		if (off + len < ip->i_iolen)
			len = ip->i_iolen - off;
		fix_isize = (off + len > old_isize);
		rw = S_WRITE;
	}

	ASSERT((bsize > PAGESIZE) ? off >> bshift == (off + len - 1) >> bshift
			: off >> PAGESHIFT == (off + len - 1) >> PAGESHIFT);

	lbn = pagerounddown(off) >> bshift;
	lastlbn = lbn + (PAGEOFFSET >> bshift);
	roff = lbn << bshift;
	eoff = off + len;

	if (ppp == NULL && rpl == NULL)
		alloc_only = B_TRUE;
	else
		alloc_only = B_FALSE;

	if (rw == S_WRITE) {
		if (rpl == NULL) {
			rpl = local_rpl;
			rpl[0] = NULL;
		}
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

	while (lbn < NDADDR) {

		nb = ob = ip->i_db[lbn];
		if (rw != S_WRITE || lbn < wlbn || lbn > lastwlbn)
			goto gotit;

		if (ob == 0 || old_isize < (lbn + 1) << bshift) {
			nsize = fragroundup(fs, size);

			/* consider need to reallocate a frag */
			if (ob != 0) {
				osize = fragroundup(fs, blkoff(fs, old_isize));
				if (nsize <= osize)
					/* Still fits. No need to realloc. */
					goto gotit;
			}

			/* Either realloc or filling in holes. */ 

			/*
			 * Lock all pages in the range.
			 */
			if (!alloc_only && !page_locked) {
			        pp = sfs_bmap_lockpages(vp, roff, size, pp,
							 rpl, &db_list);
				if (ppp)
					*ppp = pp;
				page_locked = B_TRUE;
			}

			/* Allocate the block. */
			pref = sfs_blkpref(ip, lbn, (int)0, &ip->i_db[0]);
			if (ob != 0) {
				error = sfs_realloccg(ip, ob, pref, osize,
						    nsize, &nb, cr);
			} else {
				osize = 0;
				error = sfs_alloc(ip, pref, nsize, &nb, cr);

			}
			if (error) {
				if (!alloc_only) {
					tmpp = rpl;
					while (*tmpp != NULL)
						sfs_bmap_error_unlock(*tmpp++);
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
                        opl = SFS_ILOCK(ip);
			if (ip->i_db[lbn] != ob) {
				SFS_IUNLOCK(ip, opl);
				ASSERT(!fix_isize);
				(void)sfs_free(ip, nb, (off_t)nsize);
				if (ip->i_dquot != NULL) {
					(void) sfs_chkdq(ip,
						-(long)btodb(nsize - osize),
						0, CRED());
				}
				nb = ip->i_db[lbn];
				goto gotit;
			}

			ip->i_db[lbn] = nb;
			ip->i_blocks += btodb(nsize - osize);
			IMARK(ip, IUPD|ICHG|IMOD);
                        SFS_IUNLOCK(ip, opl);

			if (ob != 0 && ob != nb) {
				(void)sfs_free(ip, ob, (off_t)osize);
				if (ip->i_dquot != NULL) {
					(void) sfs_chkdq(ip,
						-(long)btodb(nsize - osize),
						0, CRED());
				}
			}
			ob = 0;
		}
gotit:
		*io_list = ob;
		*db_list = nb;

		if (fix_isize && roff + size > old_isize)
			old_isize = ip->i_size = roff + size;

		if ((++lbn > lastlbn) || (alloc_only && lbn > lastwlbn))
			goto done;
		roff += bsize;
		size = MIN(eoff - roff, bsize);
		db_list++;
		io_list++;
	}

	/* Indirect blocks. */

	while (lbn <= lastlbn) {

	    /*
	     * Determine how many levels of indirection.
	     */
	    sh = 1;
	    bn = lbn - NDADDR;
	    for (j = NIADDR; j > 0; j--) {
	    	sh *= NINDIR(fs);
	    	if (bn < sh)
	    		break;
	    	bn -= sh;
	    }
	    if (j == 0) {
		if (!alloc_only)
			sfs_bmap_error_unlock(pp);
	    	return (EFBIG);
	    }

	    pref = 0;

	    /*
	     * fetch the first indirect block
	     */
	    nb = ip->i_ib[NIADDR - j];
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

	    	pref = sfs_blkpref(ip, lbn, 0, (daddr_t *)0);
	        error = sfs_alloc(ip, pref, (int)bsize, &nb, cr);
	    	if (error) {
			if (!alloc_only)
				sfs_bmap_error_unlock(pp);
	    		return (error);
	    	}

	    	/*
	    	 * Write synchronously so that indirect blocks
	    	 * never point at garbage.
	    	 */
	    	bp = getblk(dev, fsbtodb(fs, nb), bsize, 0);
		clrbuf(bp);
		(void)bwrite(bp);

		/*
		 * Check the state. If it hasn't changed, we can
		 * fill in the indirect block. Otherwise, someone
		 * raced with us and won.
		 */
                opl = SFS_ILOCK(ip);
		if (ip->i_ib[NIADDR - j] != 0) {
                        SFS_IUNLOCK(ip, opl);
			(void)sfs_free(ip, nb, (off_t)0);
			if (ip->i_dquot != NULL) {
				(void) sfs_chkdq(ip, -(long)bsize, 0,
						 CRED());
			}
			nb = ip->i_ib[NIADDR - j];
			goto got_indir;
		}

		ip->i_ib[NIADDR - j] = nb;
		ip->i_blocks += btodb(bsize);
		IMARK(ip, IUPD|ICHG|IMOD);
		SFS_IUNLOCK(ip, opl);
	    }

got_indir:

	    /*
	     * fetch through the indirect blocks
	     */
	    for (; j < NIADDR; j++) {
	    	bp = bread(dev, fsbtodb(fs, nb), bsize);
	    	if (geterror(bp)) {
			if (ip->i_dquot != NULL) {
				(void) sfs_chkdq(ip, -(long)btodb(bsize), 0,
						 CRED());
			}
	    		brelse(bp);
			if (!alloc_only)
				sfs_bmap_error_unlock(pp);
	    		return (EIO);
	    	}
	    	bap = bp->b_un.b_daddr;
	    	sh /= NINDIR(fs);
	    	i = (bn / sh) % NINDIR(fs);
	    	nb = bap[i];
	    	if (nb == 0) {
	    		if (rw != S_WRITE || lbn < wlbn || lbn > lastwlbn) {
	    			brelse(bp);
				*io_list = DB_HOLE;
	    			goto cont_rdonly;
	    		}

	    		if (pref == 0)
	    			pref = sfs_blkpref(ip, lbn, 0, (daddr_t *)0);

	    	        error = sfs_alloc(ip, pref, (int)bsize, &nb, cr);
	    		if (error) {
	    			brelse(bp);
				if (!alloc_only)
					sfs_bmap_error_unlock(pp);
	    			return (error);
	    		}

			/*
			 * Write synchronously so indirect blocks
			 * never point at garbage and blocks
			 * in directories never contain garbage.
			 */
			nbp = getblk(dev, fsbtodb(fs,nb), bsize, 0);
			clrbuf(nbp);
			(void)bwrite(nbp);

			bap[i] = nb;
                        opl = SFS_ILOCK(ip);
                        ip->i_blocks += btodb(bsize);
                        IMARK(ip, IUPD|ICHG|IMOD);
                        SFS_IUNLOCK(ip, opl);
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
	    bp = bread(dev, fsbtodb(fs, nb), bsize);
	    if (bp->b_flags & B_ERROR) {
	    	brelse(bp);
		if (!alloc_only)
			sfs_bmap_error_unlock(pp);
	    	return (EIO);
	    }
	    bap = bp->b_un.b_daddr;
	    i = (bn % NINDIR(fs));

	    do {
		nb = bap[i];

		/* If it's not a hole, it's simple. */
	    	if (nb != 0 || rw != S_WRITE || lbn < wlbn || lbn > lastwlbn) {
			*db_list = *io_list = nb;
			goto indir_cont;
		}

		if (pref == 0)
			pref = sfs_blkpref(ip, lbn, i, &bap[0]);

		error = sfs_alloc(ip, pref, (int)fs->fs_bsize, &nb, cr);
		if (error) {
			brelse(bp);
			if (!alloc_only)
				sfs_bmap_error_unlock(pp);
			return (error);
		}

		if (!alloc_only && !page_locked) {
			pp = sfs_bmap_lockpages(vp, roff, size, pp, rpl,
						&db_list);
			if (ppp)
				*ppp = pp;
			page_locked = B_TRUE;
		}

		bap[i] = nb;
                opl = SFS_ILOCK(ip);
                ip->i_blocks += btodb(bsize);
                IMARK(ip, IUPD|ICHG|IMOD);
                SFS_IUNLOCK(ip, opl);
		alloc = B_TRUE;
		*io_list = DB_HOLE;
		*db_list = nb;

indir_cont:
		lbn++;
		roff += bsize;
		size = MIN(eoff - roff, bsize);
		db_list++;
		io_list++;
	    } while (lbn <= lastlbn && ++i < NINDIR(fs));

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
	if (page_locked)
	    sfs_bmap_unlockpages(pp, rpl, nb);
	if (fix_isize) {
	    ip->i_size = off + len;
	    opl = SFS_ILOCK(ip);
	    IMARK(ip, IUPD|ICHG|IMOD);
	    SFS_IUNLOCK(ip, opl);
	}

	return 0;
}

/*
 * STATIC page_t *
 * sfs_bmap_lockpages(vnode_t *vp, off_t roff, size_t size, page_t *pp,
 *		page_t *rpl[])
 *
 *	This function locks all pages in the [roff, roff+size) range.
 *	It returns the	center page to the caller.
 *
 * Calling/Exit State:
 *	On entry, if 'pp' is NULL, pages in the 'rpl' list are
 *	reader-locked. If 'pp' is not NULL, 'pp' may be reader or
 *	writer locked. At exit, all pages in the range are locked.
 *
 * Description:
 *
 *	'pp' is NULL if we are called from sfs_getpage via sfs_bmappage.
 *	This is a block realloc case. Pages in the realloc range have
 *	already  been brought in from disk by sfs_getpage and these
 *	pages are stashed in the 'rpl' list reader-locked.
 *
 *	'pp' is non-null if we are called from sfs_getapage via
 *	sfs_bmappage. This is a non-realloc case. We are either
 *	filling in holes or the block is an indirect block. We need
 *	to make sure all other pages in the same block are properly
 *	initialized and flushed out to disk.
 *
 *	If 'pp' is a center page, we need to drop the page lock,
 *	get the front pages and then acquire the center page to
 *	avoid deadlock potential (locking out of order). The
 *	center page could be a different page.
 *
 */
/* ARGSUSED */
STATIC page_t *
sfs_bmap_lockpages(vnode_t *vp, off_t roff, size_t size, page_t *pp,
		page_t *rpl[], daddr_t **db_listp)
{

	off_t eoff, center_off;

	eoff = roff + size;
	roff = pagerounddown(roff);

	if (pp != NULL) {
		ASSERT(*rpl == NULL);
		/*
		 * Non-realloc case, caller is getpage.
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
	} else {
		ASSERT(rpl[0] != NULL);
		do {
			ASSERT(roff < eoff);
			roff += PAGESIZE;
		} while (*++rpl != NULL);
	}

	/* Get the rest of the pages if there are any. */
	while (roff < eoff && roff >= 0) {
		*rpl++ = page_lookup_or_create(vp, roff);

		if (roff == center_off && pp != NULL) {
			/*
			 * Center page. We have to return it to caller
			 * since we have given it up before trying
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
 * sfs_bmap_unlockpages(page_t *center_pp, page_t *rpl[], daddr_t bn)
 *
 *	This function unlocks the pages in the bmapped block.
 *      It also fills in each page's dblist entry for this block
 *      and marks it modified (so the zeroed data will get written
 *      out to the newly allocated block).
 *
 * Calling/Exit State:
 *	At exit, pages within the bmap range except the center
 *	page are unlocked.
 *
 */
/* ARGSUSED */
STATIC void
sfs_bmap_unlockpages(page_t *center_pp, page_t *rpl[], daddr_t bn)
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
 * sfs_bmap_error_unlock(page_t *pp)
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
sfs_bmap_error_unlock(page_t *pp)
{

	if (PAGE_IS_WRLOCKED(pp))
		pvn_fail(pp, B_READ);
	else
		page_unlock(pp);
}

/*
 * int
 * sfs_bmap_might_realloc(vnode_t *vp, off_t off, size_t len)
 *
 *	This function, called by sfs_getpage() for the S_OVERWRITE
 *	case, returns the length from 'off' rounded down to the
 *	block boundary upto the current file size that might be
 *	relocated to a different block due to file extension.
 *
 * Calling/Exit State:
 *	Returns 0 if no block realloc.
 *
 */
int
sfs_bmap_might_realloc(vnode_t *vp, off_t off, size_t len)
{
	inode_t *ip = VTOI(vp);
	fs_t *fs;
	daddr_t lbn;
	size_t osize, nsize;

	ASSERT(len != 0);

	fs = ip->i_fs;
	lbn = lblkno(fs, off);
	ASSERT(lblkno(fs, off + len -1 ) == lbn);

	if (lbn >= NDADDR)
		return 0;

	osize = fragroundup(fs, ip->i_size);
	nsize = fragroundup(fs, off + len);
	if (nsize > osize &&
	    lblkno(fs, ip->i_size - 1) == lbn)
		return (pageroundup(ip->i_size) -
			MIN(pagerounddown(off), blkrounddown(fs, off)));
	else
		return (0);
}
