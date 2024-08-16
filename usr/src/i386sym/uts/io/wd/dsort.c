/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/wd/dsort.c	1.7"
#ident	"$Header: $"

/*
 * dsort.c
 *	Seek sort for disks.
 */


#include <fs/buf.h>
#include <io/vtoc/vtoc.h>
#include <io/wd/wd.h>
#include <util/param.h>
#include <util/types.h>

#define CYLIN(buf_p)	((buf_p)->b_priv2.un_daddr)

/*
 * void 
 * disksort(buf_t *, buf_t *)
 *
 * Calling/Exit State:
 *	The caller must have all other access to the the
 *	sort queue and the bp to be added to it blocked.
 *
 *	Basic locks may be held upon entry/exit.
 *
 *	bp->b_priv2.un_daddr must contain the new request's key
 *	for sorting into the list.  This is normally a
 *	disk block address for the start of the request.
 * 
 *	Upon exit bp will be linked into the list for which 
 *	dp is the header, i.e., bp->av_forw will be modified, 
 *	as may dp->av_forw and dp->av_back.
 *
 *	No return value. 
 *
 * Remarks:
 *      The caller must have initialized dp->av_forw 
 *	and dp->av_back to NULL at bootup.
 *
 *	The dp's structure holds a av_forw activity chain pointer
 *	on which we keep two queues, sorted in ascending cylinder order.
 *	The first queue holds those requests which are positioned after
 *	the current cylinder (in the first request); the second holds
 *	requests which came in after their cylinder number was passed.
 *	Thus we implement a one way scan, retracting after reaching the
 *	end of the drive to the first request on the second queue,
 *	at which time it becomes the first queue.
 *
 *	A one-way scan is natural because of the way UNIX 
 *	read-ahead blocks are allocated.
 */
void
disksort(buf_t *dp, buf_t *bp)
{
	buf_t *ap;

	/*
	 * If nothing on the activity queue, then
	 * we become the only thing.
	 */
	ap = dp->av_forw;
	if(ap == NULL) {
		dp->av_forw = dp->av_back = bp;
		bp->av_forw = NULL;
		return;
	}

	/*
	 * If we lie after the first (currently active)
	 * request, then we must locate the second request list
	 * and add ourselves to it.
	 */
	if (CYLIN(bp) < CYLIN(ap)) {
		while (ap->av_forw) {
			/*
			 * Check for an ``inversion'' in the
			 * normally ascending cylinder numbers,
			 * indicating the start of the second request list.
			 */
			if (CYLIN(ap->av_forw) < CYLIN(ap)) {
				/*
				 * Search the second request list
				 * for the first request at a larger
				 * cylinder number.  We go before that;
				 * if there is no such request, we go at end.
				 */
				do {
					if (CYLIN(bp) < CYLIN(ap->av_forw)) {
						goto insert;
					}
					ap = ap->av_forw;
				} while (ap->av_forw);
				goto insert;		/* after last */
			}
			ap = ap->av_forw;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while (ap->av_forw) {
		/*
		 * We want to go after the current request
		 * if there is an inversion after it (i.e. it is
		 * the end of the first request list), or if
		 * the next request is a larger cylinder than our request.
		 */
		if (CYLIN(ap->av_forw) < CYLIN(ap) ||
		    CYLIN(bp) < CYLIN(ap->av_forw)) {
			goto insert;
		}
		ap = ap->av_forw;
	}
	/*
	 * Neither a second list nor a larger
	 * request... we go at the end of the first list,
	 * which is the same as the end of the whole schebang.
	 */
insert:
	bp->av_forw = ap->av_forw;
	ap->av_forw = bp;
	if (ap == dp->av_back) {
		dp->av_back = bp;
	}
}
