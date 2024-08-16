/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/metdisk.c	1.11"
#ident	"$Header: $"

#include <fs/buf.h>
#include <io/metdisk.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * met_ds_list points to the beginning of the list of disk stat combo structures
 * It's how these stats will be accessed from outside.
 */
met_disk_stats_list_t * met_ds_list = NULL;

/*
 * So that the other routines can avoid checking for NULL disk stat pointers
 * met_ds_alloc_stats will ALWAYS return a pointer to a disk stats structure.
 * If we can't allocate a real one, we'll point to a "garbage location"
 * that won't provide info, but will at least be a place to point to.
 */
STATIC met_disk_stats_t ds_garbage;

STATIC lock_t	met_ds_lock;
/*
 *+ met_ds_lock is used to protect the state of the disk statistic list
 *+ and the list structures.
 */
static LKINFO_DECL(met_ds_lkinfo, "IO::met_ds_lock", 0);

STATIC met_disk_stats_t * find_stats_struct(char *, met_disk_stats_list_t **);
STATIC void turn_on_hist(met_disk_stats_t *, uint, uint, met_ds_hist_t *);
STATIC void turn_off_hist(met_disk_stats_t *, uint, uint, met_ds_hist_t *);

/*
 * void met_ds_init()
 *	initializes the metric disk structures and locks.
 *	Called from ddi_init().
 *
 * Calling/Exit State:
 *	no locks held on entry, none held on exit
 */
void
met_ds_init()
{
	LOCK_INIT(&met_ds_lock, MET_DISK_HIER, PLMIN, &met_ds_lkinfo,
		KM_NOSLEEP); 
}

/*
 * struct met_disk_stats * met_ds_alloc_stats(char *prefix,
 *					      int unit,
 *					      ulong size,
 *					      uint flags)
 *
 * Allocates a met_disk_stats structure and returns a pointer to it. 
 * This is typically done during initialization and the pointer stored
 * in a device-specific structure.
 *
 *	'prefix' is the prefix for the device (usually the same as what would
 *		appear in the device name).  A real name for this particular
 *		device is constructed from 'prefix' and 'unit'.
 *	'unit' is the unit number for this particular device.  It is used
 *		along with 'prefix' in constructing the name for this device.
 *	'size' is the "size" of the device, either in cylinders if this info
 *		is available or in DEV_BSIZE blocks.  If it is in blocks,
 *		then the MET_DS_BLK_HIST flag must be set (see flags).
 *	'flags' - flag bits.  Available flags are:
 *		MET_DS_CYL		size is in cyls
 *		MET_DS_BLK		size is in DEV_BSIZE blocks
 *		MET_DS_NO_ACCESS_HIST	driver doesn't provide info for
 *					access and seek distance histograms
 *		MET_DS_NO_RESP_HIST	driver doesn't provide info for
 *					response time histograms
 * Calling/Exit State:
 * 	Returns a pointer to a met_disk_stats structure.
 */

struct met_disk_stats *
met_ds_alloc_stats(char *prefix, int unit, ulong size, uint flags)
{
	met_disk_stats_t *dsp;
	met_disk_stats_list_t *dlp;
	met_disk_stats_list_t *prev = NULL;
	met_disk_stats_combo_t *ds_space;
	char tmp[11];	/* Used to convert the 'unit' to a decimal string.
			   Needs to be size 11 to hold all the digits */
	static physreq_t preq = {
		/* phys_align */	1,
		/* phys_boundary */	PAGESIZE,
		/* phys_dmasize */	0,
		/* phys_max_scgth */	0
	};
	unsigned tmplen, len;
	pl_t	s;
	int	i;

	if (!physreq_prep(&preq, KM_NOSLEEP)) {
		/*
		 *+ Kernel memory for disk stat structures
		 *+ could not be allocated. Reconfigure the
		 *+ system to consume less memory.
		 */
		cmn_err(CE_WARN,
		  "met_ds_alloc_stats: allocation of disk stats failed\n");
		/*
		 * allocation failed. Return pointer to
		 * the common garbage structure.
		 */
		return(&ds_garbage);
	}

	s = LOCK(&met_ds_lock, PLMIN);

	/*
	 * Walk down the chain looking for a block of stats with
	 * a free entry.  
	 */
	for (dlp = met_ds_list;
	     dlp != NULL && dlp->dl_used == dlp->dl_num; dlp = dlp->dl_next) {
		prev = dlp;
	}

	/*
	 * dlp == NULL implies a new chunk of met_disk_stats is
	 * needed so attempt to allocate.  Ask for space for
	 * both the list structure and the array of met_disk_stats
	 * structures at the same time to reduce fragmentation.
	 * Since kmem_alloc does not guarantee that a request
	 * of PAGESIZE bytes will be PAGESIZE aligned, we
	 * must use kmem_alloc_physcontig() here.  We need
	 * the space to be page aligned so that we can mmap it.
	 */
	ASSERT(sizeof(met_disk_stats_combo_t) <= PAGESIZE);
	if (dlp == NULL) {
		ds_space = kmem_alloc_physcontig(sizeof(met_disk_stats_combo_t),
						 &preq, KM_NOSLEEP);

		if (ds_space == NULL) {
			/*
			 *+ Kernel memory for disk stat structures
			 *+ could not be allocated. Reconfigure the
			 *+ system to consume less memory.
			 */
			cmn_err(CE_WARN,
			  "met_ds_alloc_stats: allocation of disk stats failed\n");
			/*
			 * allocation failed. Return pointer to
			 * the common garbage structure.
			 */
			UNLOCK(&met_ds_lock, s);
			return(&ds_garbage);
		} else {
			/*
			 * Initialize the list structure
			 */
			struct_zero(ds_space, sizeof(met_disk_stats_combo_t));
			dlp = (met_disk_stats_list_t *)&(ds_space->dc_list);
			dlp->dl_stats =(met_disk_stats_t *)&(ds_space->dc_stats);
			dlp->dl_num = MET_DS_STATS_PER_LIST;
			dlp->dl_used = 0;
			dlp->dl_next = NULL;

			/* 
			 * put this new buffer on the list.
			 */
			if (prev)
				prev->dl_next = dlp;
			else 
				met_ds_list = dlp;
		}
	}

	/*
	 * Found a block of stats with a free entry.  Find one by
	 * searching for a null ds_name.  Then increment 
	 * the number of entries used. 
	 */
	for (i=0; i<dlp->dl_num && dlp->dl_stats[i].ds_name[0] != '\0'; i++);

	ASSERT(i < dlp->dl_num);

	dsp = &(dlp->dl_stats[i]);
	dlp->dl_used++;

	/*
	 * Zero out the structure
	 */
	bzero(dsp, sizeof(met_disk_stats_t));
	/*
	 * Initialize dsp->ds_name.
	 */
	len = MIN(strlen(prefix), (MET_DS_NAME_SZ - 1));
	strncpy(dsp->ds_name, prefix, len);

	/*
	 * We can unlock now. 
	 * Setting the name claims this structure as taken.
	 */
	UNLOCK(&met_ds_lock, s);

	/*
	 * convert unit to string and append to ds_name
	 * (There is no concern with overflowing the tmp array
	 * since we made it long enough to hold the decimal 
	 * representation of a 32 bit number.)
	 */
	tmplen = 0;
	do {
		tmp[tmplen++] = unit % 10 + '0';
		unit /= 10;
	} while (unit != 0);

	for (; (tmplen != 0) && (len < MET_DS_NAME_SZ - 1); )
		dsp->ds_name[len++] = tmp[--tmplen];
	
	dsp->ds_name[len] = '\0';

	/*
	 * Initialize dsp->ds_cyls
	 */
	if (flags & MET_DS_BLK) {
		size >>= MET_DS_BPC_SHIFT;
	}
	dsp->ds_cyls = size;

	/*
	 * Initialize the flags
	 */
	dsp->ds_flags = flags;

	/*
	 * Increment the number of disks we keep data for
	 * and return the pointer
	 */
	MET_DISK_INUSE(1);
	return(dsp);
}

/*
 * void met_ds_dealloc_stats(met_disk_stats_t * dsp);
 *
 *	met_ds_dealloc_stats() is called to return a disk stats
 *	structure to the pool of structures.  Only unloadable
 *	disk drivers will have cause to use this, so we don't
 *	expect it to be used very often.  Thus, we don't have
 *	a mechanism to easily locate the header structure associated
 *	with the disk stats structure being returned.  We just
 *	do a linear search.
 *
 *	dsp	points to the structure being returned
 *
 * Calling/Exit State:
 *	None.
 */
void
met_ds_dealloc_stats(met_disk_stats_t * dsp)
{
	met_disk_stats_list_t *dlp;
	int	i;
	pl_t	s;

	/*
	 * if dsp points to the garbage area, just return.
	 */
	if (dsp == &ds_garbage)
		return;

	s = LOCK(&met_ds_lock, PLMIN);
	/*
	 * If there's any histogram space allocated, we'll just
	 * set the inactive flag and leave this structure allocated.
	 * This is so we don't freak out any histogram collectors
	 * that might have the location of the histogram pegged.
	 * When the last histogram is freed, then we'll deallocate
	 * the structure in met_ds_hist_off().
	 */
	if (dsp->ds_resphist.dh_hist != NULL
	    || dsp->ds_cylhist.dh_hist != NULL
	    || dsp->ds_seekhist.dh_hist != NULL ){
		dsp->ds_flags |= MET_DS_INACTIVE;
		UNLOCK(&met_ds_lock, s);
		return;
	}

	/*
	 * NULL out the name.  That's the sign that this is a free structure.
	 */
	dsp->ds_name[0] = '\0';

	/*
	 * Now find the met_disk_stats_list structure pointing to
	 * this stats structure so we can decrement the used count
	 */
	for (dlp = met_ds_list; dlp != NULL; dlp = dlp->dl_next) {
		if (dlp->dl_used > 0) {
			for (i=0; i<MET_DS_STATS_PER_LIST; i++) {
				/*
				 * Is this the structure?
				 */
				if (&dlp->dl_stats[i] == dsp) {
					dlp->dl_used--;
					MET_DISK_INUSE(-1);
					UNLOCK(&met_ds_lock, s);
					return;
				}
			}
		}
	}
	UNLOCK(&met_ds_lock, s);
	/*
	 *+ Called met_ds_dealloc_stats() to dealloc a pointer that is
	 *+ not a met_disk_stats_t pointer.  This pointer, at least,
	 *+ must have been munged.
	 */
	cmn_err(CE_WARN,
	  "met_ds_dealloc_stats: called with invalid disk stats pointer\n");

	/* Should not reach this point.  Fail assertion if we do. */
#ifndef lint
	ASSERT(B_FALSE);
#endif
}

/*
 * void met_ds_queued(met_disk_stats_t *dsp, uint b_flags)
 *
 *	At each point when a request has been added to the
 * 	driver queue but before it has been queued to the device, add a
 *	call to met_ds_queued().
 *
 *	'dsp' is a pointer to the disk stats structure for the drive.
 *	'b_flags' are the buffer header flags for this request
 *		(Not currently used, but may be important to
 *		know if other disk stat algorithms needed.)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
met_ds_queued(met_disk_stats_t *dsp, uint b_flags)
{
	ulong	time;
	ulong	since_lasttime;

	GET_MET_TIME(time);
	if (dsp->ds_qlen != 0) {
		DIFF_MET_TIME(time, dsp->ds_lasttime, since_lasttime);
		ldladd(&(dsp)->ds_active, since_lasttime);
		/*
		 * response time is the area under the curve
		 */
		since_lasttime *= (dsp)->ds_qlen;
		ldladd(&(dsp)->ds_resp, since_lasttime);
	}
	(dsp)->ds_qlen++;
	(dsp)->ds_lasttime = time;
}

/*
 * void met_ds_dequeued(met_disk_stats_t *dsp, uint b_flags)
 *
 *	Called only in the case where met_ds_queued was called but	
 * 	the job was backed out before going to disk for some reason.
 *	This might be used by an driver interface in cases where the
 *	host adaptor driver refused to queue the job for some reason.
 *
 *	'dsp' is a pointer to the disk stats structure for the drive.
 *
 *	'b_flags' is the buffer header flags.  We don't use it now,
 *		  but might need to someday.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
met_ds_dequeued(met_disk_stats_t *dsp, uint b_flags)
{
	dsp->ds_qlen--;
}

/*
 * void met_ds_iodone(met_disk_stats_t *dsp, int b_flags, ulong bytes)
 *
 *	met_ds_iodone() should be called at the time a request finishes
 *	before the next one is sent.
 *
 *	'dsp' is the pointer to the met_disk_stats structure for this drive.
 *	'b_flags' are the buffer header flags for this request
 *	'bytes' is the number of bytes read or written
 *
 * Calling/Exit State:
 *	None.
 */

void
met_ds_iodone(met_disk_stats_t *dsp, int b_flags, ulong bytes)
{
	ulong time;
	ulong since_lasttime;

	GET_MET_TIME(time);
	DIFF_MET_TIME(time, dsp->ds_lasttime, since_lasttime);
	ldladd(&(dsp)->ds_active, since_lasttime);
	/*
	 * response time is the area under the curve
	 */
	since_lasttime *= dsp->ds_qlen;
	ldladd(&dsp->ds_resp, since_lasttime);
	dsp->ds_qlen--;
	dsp->ds_lasttime = time;

	/*
	 * Update info on the type of request and data transfered
	 */
	dsp->ds_op[MET_DS_OP_INDEX(b_flags)]++;
	dsp->ds_opblks[MET_DS_OP_INDEX(b_flags)] +=
		((bytes) + DEV_BSIZE - 1) >> DEV_BSHIFT;
}



/*
 * void _met_ds_hist_stats(struct met_disk_stats *dsp,
 *			  long cyl,
 *			  ulong * start,
 *			  ulong * end);
 *
 *	_met_ds_hist_stats() should be called after the job has completed
 *	and after the call to MET_DS_IODONE(). 
 *	This updates response, cylinder access, and
 * 	seek distance histograms.
 *
 *	'dsp' is the pointer to the drive's disk stat structure
 *
 *	'cyl' is the cylinder at which the request being processed starts
 *		(if MET_DS_CYL was specified) or the physical block number
 *		the request starts at (if MET_DS_BLK was specified).
 *
 *	'start' points to the time the request started
 *
 *	'end' points to time the request finished
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	When the HRESTIME option of drv_getparm is functional we'll have
 *	to change how we calculate the response time.  Until it is functional
 *	start_p and end_p will point to lbolt times.
 *
 *	We also need to implement the response time histogram
 */
/* ARGSUSED */
void
_met_ds_hist_stats(met_disk_stats_t * dsp, long cyl, ulong * start, ulong * end)
{
	long sdist;

	/*
	 * If keeping the response time histogram then calculate
	 * response time for this job and update histogram.
	 *
	 * TBD:	The format of the timestamps and the bucket sizes
	 * 	are still under design.
	 */
	
	/*
	 * If faking cylinder access data with buckets of
	 * blocks, cyl is really a block number, so convert
	 * it to a bucket.  Otherwise leave it the same.
	 */
	if (dsp->ds_flags & MET_DS_BLK)
		cyl >>= MET_DS_BPC_SHIFT;

	/*
	 * If we've been given a bad cylinder number don't update
	 * seek and access histograms.
	 */
	if ((0 <= cyl) && (cyl <= dsp->ds_cyls)) {
		/*
		 * If keeping the seek distance histogram,
		 * determine the seek distance and update the histogram
		 */
		if (dsp->ds_seekhist.dh_hist != NULL) {
			sdist = dsp->ds_lastcyl - cyl;
			if (sdist < 0)
				sdist = -sdist;
			dsp->ds_seekhist.dh_hist[sdist]++;

			/*
			 * Remember which cylinder we've accessed.
			 */
			dsp->ds_lastcyl = cyl;
		}

		/*
		 * If keeping the cylinder access histogram, update it.
		 */
		if (dsp->ds_cylhist.dh_hist != NULL)
			dsp->ds_cylhist.dh_hist[cyl]++;
	}
}

/*
 * met_disk_stats_t * met_ds_hist_on(char * namep, uint histflag)
 *
 *	met_ds_hist_on is called to turn on the collection of
 *	disk drive histogram data and to allocate
 *	space for the disk drive histograms.
 *
 *	The exact caller isn't known yet.  It will depend on the metric
 *	collection interface we provide for the user level caller. 
 *
 *	namep		The drive name we want a histogram for.
 *
 *	histflag	The histogram(s) requested.
 *			MET_DS_HIST_RESP for the response time histogram
 *			MET_DS_HIST_ACCESS for the cylinder access histogram
 *			MET_DS_HIST_SEEK for the seek distance histogram
 *
 *
 * Calling/Exit State:
 *	Return value is the address of the disk stats structure (for now).
 *	It is NULL in case of error.
 *
 * Remarks:
 *	We need to implement the response time histogram.
 */

met_disk_stats_t *
met_ds_hist_on(char *namep, uint histflag)
{
	met_disk_stats_t *dsp;
	met_disk_stats_list_t *dlp;
	pl_t	s;

	s = LOCK(&met_ds_lock, PLMIN);
	/*
	 * Find the disk stats structure
	 */
	if ((dsp = find_stats_struct(namep, &dlp)) == NULL) {
		/*
		 * Didn't find stats for a drive by this name.
		 */
		UNLOCK(&met_ds_lock, s);
		return(NULL);
	}

	/* 
	 * If this drive has been inactivated, then don't
	 * turn on the histograms.
	 */
	if (dsp->ds_flags & MET_DS_INACTIVE) {
		UNLOCK(&met_ds_lock, s);
		return(NULL);
	}

	/*
	 * If response time histogram requested, and driver provides
	 * info for it, turn it on.
	 *
	 * TBD:	The format of the timestamps and the bucket sizes
	 * 	are still under design so we can't turn the response
	 * 	time histogram on yet.  
	 *
	 * if ((histflag & MET_DS_HIST_RESP)
	 *    && !(dsp->ds_flags & MET_DS_NO_RESP_HIST)) {
	 *	 turn_on_hist(dsp, MET_DS_HIST_RESP,
	 *	    MET_DS_RESP_HISTSZ(dsp->ds_cyls), &(dsp->ds_resphist));
	 * }
	 */

	/*
	 * If the cylinder access histogram requested and driver gives info,
	 * allocate space for it if it's not already allocated
	 */
	if ((histflag & MET_DS_HIST_ACCESS)
	    && !(dsp->ds_flags & MET_DS_NO_ACCESS_HIST))
		turn_on_hist(dsp, MET_DS_HIST_ACCESS,
		    MET_DS_CYL_HISTSZ(dsp->ds_cyls), &(dsp->ds_cylhist));

	/*
	 * If seek distance histogram requested and driver gives info,
	 * allocate space for it if it's not already allocated
	 */
	if ((histflag & MET_DS_HIST_SEEK)
	    && !(dsp->ds_flags & MET_DS_NO_ACCESS_HIST)) {
		turn_on_hist(dsp, MET_DS_HIST_SEEK,
		    MET_DS_SEEK_HISTSZ(dsp->ds_cyls), &(dsp->ds_seekhist));
	}

	UNLOCK(&met_ds_lock, s);
	return (dsp);
}

/*
 * int met_ds_hist_off(char * namep, uint histflag)
 *
 *	met_ds_hist_off() is called to turn off the collection of
 *	disk drive histogram data and to deallocate
 *	space for the disk drive histograms.
 *	In addition, if the disk stats structure has the MET_DS_INACTIVE
 *	flag set, and no histograms are still allocated, then we free
 *	the stats structure.
 *
 *	The exact caller isn't known yet.  It will depend on the metric
 *	collection interface we provide for the user level caller. 
 *
 *	namep		The drive name we want a histogram for.
 *
 *	histflag	The histogram(s) requested.
 *			MET_DS_HIST_RESP for the response time histogram
 *			MET_DS_HIST_ACCESS for the cylinder access histogram
 *			MET_DS_HIST_SEEK for the seek distance histogram
 *
 * Calling/Exit State:
 *	Return value is 0 if successful (a ds_name was found that matched
 *	the name pointed to by namep) and non-zero if unsuccessful.
 *
 * Remarks:
 *	We need to implement the response time histogram.
 */

int
met_ds_hist_off(char *namep, uint histflag)
{
	met_disk_stats_t *dsp;
	met_disk_stats_list_t *dlp;
	pl_t	s;

	s = LOCK(&met_ds_lock, PLMIN);

	/*
	 * Find the disk stats structure
	 */
	if ((dsp = find_stats_struct(namep, &dlp)) == NULL) {
		/*
		 * Didn't find stats for a drive by this name.
		 */
		UNLOCK(&met_ds_lock, s);
		return(1);
	}

	/*
	 * If response time histogram flagged, turn it off.
	 *
	 * TBD:	The format of the timestamps and the bucket sizes
	 * 	are still under design so we can't turn the response
	 * 	time histogram on yet.  
	 *
	 * if ((histflag & MET_DS_HIST_RESP)
	 *    && dsp->ds_resphist.dh_numusers != 0){
	 *	turn_off_hist(dsp, MET_DS_HIST_RESP,
	 *	MET_DS_RESP_HISTSZ(dsp->ds_cyls), &(dsp->ds_cylhist));
	 * }
	 */

	/*
	 * If cylinder access histogram flagged,
	 * decrement use count, and deallocate space if that count is 0
	 */
	if ((histflag & MET_DS_HIST_ACCESS) && dsp->ds_cylhist.dh_numusers != 0)
		turn_off_hist(dsp, MET_DS_HIST_ACCESS,
		    MET_DS_CYL_HISTSZ(dsp->ds_cyls), &(dsp->ds_cylhist));

	/*
	 * If seek distance histogram flagged,
	 * decrement use count, and deallocate space if that count is 0
	 */
	if ((histflag & MET_DS_HIST_SEEK) && dsp->ds_seekhist.dh_numusers != 0)
		turn_off_hist(dsp, MET_DS_HIST_SEEK,
		    MET_DS_SEEK_HISTSZ(dsp->ds_cyls), &(dsp->ds_seekhist));

	/*
	 * If the stats structure is inactive and no histograms
	 * are still allocated, then mark the structure as free.
	 */
	if ((dsp->ds_flags & MET_DS_INACTIVE)
			&& (dsp->ds_resphist.dh_hist == NULL)
			&& (dsp->ds_cylhist.dh_hist == NULL)
			&& (dsp->ds_seekhist.dh_hist == NULL) ) {
		dsp->ds_name[0] = '\0';
		dlp->dl_used--;
		MET_DISK_INUSE(-1);
	}

	UNLOCK(&met_ds_lock, s);
	return(0);
}

/*
 * met_disk_stats_t * find_stats_struct(char * namep,
					met_disk_stats_list_t **dlpp)
 *
 *	Common code for finding the disk stats structure whose ds_name
 *	field matches the string pointed to by namep.
 *
 *	Called by met_ds_hist_on() and met_ds_hist_off().
 *
 *	namep		The drive name of the disk stats struct wanted
 *
 *	dlpp		Location to store the disk stats list pointer that
 *			maintains the disk stats structure returned.
 *
 * Calling/Exit State:
 *	Return value is the pointer to the disk stats structure if successful
 *	(a ds_name was found that matched the name pointed to by namep).
 *	The disk stats list pointer is stored in the value pointed to by dlpp.
 *	Returns NULL if the name could not be matched.
 */
STATIC met_disk_stats_t *
find_stats_struct(char * namep, met_disk_stats_list_t **dlpp)
{
	int	i;
	met_disk_stats_list_t	*dlp;

	for (dlp = met_ds_list; dlp != NULL; dlp = dlp->dl_next)
		if (dlp->dl_used != 0)
			for (i=0; i<MET_DS_STATS_PER_LIST; i++)
				if (strcmp(namep, dlp->dl_stats[i].ds_name)==0){
					*dlpp = dlp;
					return(&dlp->dl_stats[i]);
				}
	/*
	 * If we get here, we didn't find a stats structure matching namep.
	 * Return NULL
	 */
	*dlpp = NULL;
	return(NULL);
}

/*
 * void turn_on_hist(met_disk_stats_t * dsp, uint which_hist,
 *		     uint size, met_ds_hist_t * dhp)
 *
 *	Common code to turn on and possibly allocate space for a histogram.
 *	Called for different histograms from met_ds_hist_on().
 *
 *	dsp		the pointer to the disk stats structure whose stats
 *			are being turned on
 *
 *	which_hist;	the flag for the histogram we're turning on
 *
 *	size		the number of bytes in the histogram (for kmem_zalloc).
 *
 *	dhp		pointer to the histogram structure we're turning on
 *
 * Calling/Exit State:
 *	No return value.  Requested histogram is marked as "on".
 */
STATIC void
turn_on_hist(met_disk_stats_t * dsp, uint which_hist,
	     uint size, met_ds_hist_t * dhp)
{
	if (dhp->dh_hist == NULL) {
		if ((dhp->dh_hist = kmem_zalloc(size, KM_NOSLEEP)) != NULL) {
			dsp->ds_flags |= which_hist;
			dhp->dh_numusers++;
		}
	} else
		dhp->dh_numusers++;
	return;
}


/*
 * void turn_off_hist(met_disk_stats_t * dsp, uint which_hist,
 *		      uint size, met_ds_hist_t * dhp)
 *
 *	Common code to decrement the numuser count for a histogram,
 *	and if that count becomes 0, then to deallocate the histogram space.
 *
 *	Called for different histograms from met_ds_hist_off().
 *
 *	dsp		the pointer to the disk stats structure whose stats
 *			are being turned off
 *
 *	which_hist;	the flag for the histogram we're turning off
 *
 *	size		the number of bytes in the histogram (for kmem_free).
 *
 *	dhp		pointer to the histogram structure we're turning off
 *
 * Calling/Exit State:
 *	No return value.  Requested histogram is marked as "on".
 */

STATIC void
turn_off_hist(met_disk_stats_t * dsp, uint which_hist,
	      uint size, met_ds_hist_t * dhp)
{
	if (--dhp->dh_numusers == 0) {
		kmem_free((void *)dhp->dh_hist, size);
		dhp->dh_hist = NULL;
		dsp->ds_flags &= ~which_hist;
	}
	return;
}
