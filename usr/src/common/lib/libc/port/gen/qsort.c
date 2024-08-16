/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/qsort.c	1.14"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * qsort.c:
 * Our own version of the system qsort routine which is faster by an average
 * of 25%, with lows and highs of 10% and 50%.
 * The THRESHold below is the insertion sort threshold, and has been adjusted
 * for records of size 48 bytes.
 * The MTHREShold is where we stop finding a better median.
 */

#include "synonyms.h"
#include <stdlib.h>
#include "_thr_funcs.h"

#define		THRESH		4		/* threshold for insertion */
#define		MTHRESH		6		/* threshold for median */

struct qsort	{
        int		(*qcmp)();		/* the comparison routine */
        int		qsz;			/* size of each record */
        int		thresh;			/* THRESHold in chars */
        int		mthresh;		/* MTHRESHold in chars */
/*	char  		*base;			/* base and max are part of */
/*	char  		*max;			/* the passed in struct, efficiency */
};

static	void qst();

#define QSORTPTR	qsort_ptr

/*
 * qsort:
 * First, set up some global parameters for qst to share.  Then, quicksort
 * with qst(), and then a cleanup insertion sort ourselves.  Sound simple?
 * It's not...
 */
#ifdef __STDC__
void
qsort(void *base, size_t n, size_t size, int (*compar)(const void *, const void *))
#else	
void
qsort(base, n, size, compar)
	char	*base;
	size_t	n;
	size_t	size;
	int	(*compar)();
#endif
{
	register char c, *i, *j, *lo, *hi;
	char *min, *max;
	struct qsort qsort_buf;

	if (n <= 1)
		return;
	
	qsort_buf.qsz = size;
	qsort_buf.qcmp = compar;
	qsort_buf.thresh = size * THRESH;
	qsort_buf.mthresh = size * MTHRESH;
	max = (char *)base + n * qsort_buf.qsz;
/*
	qsort_buf.base = base;
	qsort_buf.max = (char *)qsort_buf.base + n * qsort_buf.qsz;
*/
	if (n >= THRESH) {
		qst(&qsort_buf, base, max);
		hi = (char *)base + qsort_buf.thresh;
	} else {
		hi = max;
	}
	/*
	 * First put smallest element, which must be in the first THRESH, in
	 * the first position as a sentinel.  This is done just by searching
	 * the first THRESH elements (or the first n if n < THRESH), finding
	 * the min, and swapping it into the first position.
	 */
	for (j = lo = base; (lo += qsort_buf.qsz) < hi; )
		if (qsort_buf.qcmp(j, lo) > 0)
			j = lo;
	if (j != base) {
		/* swap j into place */
		for (i = (char *)base, hi = (char *)base + qsort_buf.qsz; i < hi; ) {
			c = *j;
			*j++ = *i;
			*i++ = c;
		}
	}
	/*
	 * With our sentinel in place, we now run the following hyper-fast
	 * insertion sort.  For each remaining element, min, from [1] to [n-1],
	 * set hi to the index of the element AFTER which this one goes.
	 * Then, do the standard insertion sort shift on a character at a time
	 * basis for each element in the frob.
	 */
	for (min = base; (hi = min += qsort_buf.qsz) < max; ) {
		while (qsort_buf.qcmp(hi -= qsort_buf.qsz, min) > 0)
			/* void */;
		if ((hi += qsort_buf.qsz) != min) {
			for (lo = min + qsort_buf.qsz; --lo >= min; ) {
				c = *lo;
				for (i = j = lo; (j -= qsort_buf.qsz) >= hi; i = j)
					*i = *j;
				*i = c;
			}
		}
	}
}

/*
 * qst:
 * Do a quicksort
 * First, find the median element, and put that one in the first place as the
 * discriminator.  (This "median" is just the median of the first, last and
 * middle elements).  (Using this median instead of the first element is a big
 * win).  Then, the usual partitioning/swapping, followed by moving the
 * discriminator into the right place.  Then, figure out the sizes of the two
 * partions, do the smaller one recursively and the larger one via a repeat of
 * this code.  Stopping when there are less than THRESH elements in a partition
 * and cleaning up with an insertion sort (in our caller) is a huge win.
 * All data swaps are done in-line, which is space-losing but time-saving.
 * (And there are only three places where this is done).
 */

static void
qst(qsort_ptr, base, max)
	struct qsort *qsort_ptr;
	char *base;
	char *max;
{
	register char c, *i, *j, *jj;
	register int ii;
	char *mid, *tmp;
	int lo, hi;

	/*
	 * At the top here, lo is the number of characters of elements in the
	 * current partition.  (Which should be max - base).
	 * Find the median of the first, last, and middle element and make
	 * that the middle element.  Set j to largest of first and middle.
	 * If max is larger than that guy, then it's that guy, else compare
	 * max with loser of first and take larger.  Things are set up to
	 * prefer the middle, then the first in case of ties.
	 */
	lo = max - base;		/* number of elements as chars */
	do	{
		mid = i = base + QSORTPTR->qsz * ((lo / QSORTPTR->qsz) >> 1);
		if (lo >= QSORTPTR->mthresh) {
			j = (QSORTPTR->qcmp((jj = base), i) > 0 ? jj : i);
			if (QSORTPTR->qcmp(j, (tmp = max - QSORTPTR->qsz)) > 0) {
				/* switch to first loser */
				j = (j == jj ? i : jj);
				if (QSORTPTR->qcmp(j, tmp) < 0)
					j = tmp;
			}
			if (j != i) {
				ii = QSORTPTR->qsz;
				do	{
					c = *i;
					*i++ = *j;
					*j++ = c;
				} while (--ii);
			}
		}
		/*
		 * Semi-standard quicksort partitioning/swapping
		 */
		for (i = base, j = max - QSORTPTR->qsz; ; ) {
			while (i < mid && QSORTPTR->qcmp(i, mid) <= 0)
				i += QSORTPTR->qsz;
			while (j > mid) {
				if (QSORTPTR->qcmp(mid, j) <= 0) {
					j -= QSORTPTR->qsz;
					continue;
				}
				tmp = i + QSORTPTR->qsz;	/* value of i after swap */
				if (i == mid) {
					/* j <-> mid, new mid is j */
					mid = jj = j;
				} else {
					/* i <-> j */
					jj = j;
					j -= QSORTPTR->qsz;
				}
				goto swap;
			}
			if (i == mid) {
				break;
			} else {
				/* i <-> mid, new mid is i */
				jj = mid;
				tmp = mid = i;	/* value of i after swap */
				j -= QSORTPTR->qsz;
			}
		swap:
			ii = QSORTPTR->qsz;
			do	{
				c = *i;
				*i++ = *jj;
				*jj++ = c;
			} while (--ii);
			i = tmp;
		}
		/*
		 * Look at sizes of the two partitions, do the smaller
		 * one first by recursion, then do the larger one by
		 * making sure lo is its size, base and max are update
		 * correctly, and branching back.  But only repeat
		 * (recursively or by branching) if the partition is
		 * of at least size THRESH.
		 */
		i = (j = mid) + QSORTPTR->qsz;
		if ((lo = j - base) <= (hi = max - i)) {
			if (lo >= QSORTPTR->thresh)
				qst(QSORTPTR, base, j);
			base = i;
			lo = hi;
		} else {
			if (hi >= QSORTPTR->thresh)
				qst(QSORTPTR, i, max);
			max = j;
		}
	} while (lo >= QSORTPTR->thresh);
}
