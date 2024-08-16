/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/newent.c	1.1"
/*LINTLIBRARY*/

#include <stdio.h>
#include <stdlock.h>
#include "dprof.h"

#ifdef _REENTRANT
static StdLock _newmon_lock;
#endif

Cntb    *_cntbuffer;    /* pointer to call count buffer */
int      countbase = FCNTOT;

/* * * * * *
 * mcountNewent() -- call to get a new mcount call count entry.
 * 
 * this function is called by _mcount to get a new call count entry
 * (struct cnt, in the region allocated by _monitor()), or to return
 * zero if profiling is off.
 *
 * This function acts as a funnel, an access function to make sure
 * that all instances of mcount (the one in the a.out, and any in
 * any shared objects) all get entries from the correct array, and
 * all know when profiling is off.
 * 
 * NOTE: when mcount calls this function, it sets a private flag
 * so that it does not call again until this function returns,
 * thus preventing recursion.
 * 
 * At Worst, the mcount in either a shared object or the a.out
 * could call once, and then the mcount living in the shared object
 * with monitor could call a second time (i.e. libc.so.1, although
 * presently it does not have mcount in it).  This worst case
 * would involve Two active calls to mcountNewent, which it can
 * handle, since the second one would find a already-set value
 * in countbase.
 * 
 * The only unfortunate result is that No new call counts
 * will be handed out until this function returns.
 * Thus if malloc or other routines called inductively by
 * this routine have not yet been provided with a call count entry,
 * they will not get one until this function call is completed.
 * Thus a few calls to library routines during the course of
 * profiling setup, may not be counted.
 *
 * NOTE: countbase points at the next available entry, and
 * countlimit points past the last valid entry, in the current
 * function call counts array.
 * 
 * 
 * if profiling is off		// scale == 0
 *   just return 0
 *
 * else
 *   if need more entries	// because countbase == 0
 *     link in a new block
 * endif
 * if Got more entries
 *   set pointer to next Cnt entry,
 *   set SOentry information,
 *   drecrement number of entries used,
 *   return pointer to Cnt entry
 *
 *   else			// failed to get more entries
 *     just return 0
 *
 *   endif
 * endif
 */

#if 0
static void 
_mnewblock()
{
	Cntb	*ncntbuf;	/* temporary Cntb pointer */

	/* temporarily turn off SIGPROF signal */
	sigemptyset(&psig.sa_mask);
	sigaddset(&psig.sa_mask, SIGPROF);
	sigprocmask(SIG_BLOCK, &psig.sa_mask, 0);
	/* get space for new buffer, malloc returns NULL on failure */
	ncntbuf = (Cntb *)malloc(sizeof(Cntb));
	if (ncntbuf == NULL)
	{
		perror("mcount(mnewblock)");
		return;
	}

	/* link new call count buffer to old call count buffer */
	ncntbuf->next = _cntbuffer;

	/* reset countbase and _cntbuffer */
	_cntbuffer = ncntbuf;
	countbase  = FCNTOT;
	/* turn back on SIGPROF signal */
	sigprocmask(SIG_UNBLOCK, &psig.sa_mask, 0);
}
#endif

#ifdef DSHLIB
void _mnewblock()
{}
#endif

Cnt *
_mcountNewent(SOentry *curr_SO)
{

	static Cnt	*cur_countbase;	/* pointer to last used Cnt */

	if ( curr_SO == 0 )
		return ((Cnt *)0);
	STDLOCK(&_newmon_lock);
	if ( cur_countbase == 0)
		cur_countbase = &_cntbuffer->cnts[0];
	else
		cur_countbase++;

	if ( countbase == 0 )
	{
		_mnewblock();
		cur_countbase = &_cntbuffer->cnts[0];
	}

	if ( countbase != 0 )
	{
		cur_countbase->_SOptr = curr_SO;
		curr_SO->ccnt++;
		countbase--;

	}
	else
	{
		cur_countbase =  ((Cnt *)0);
	}
	STDUNLOCK(&_newmon_lock);
	return (cur_countbase);

}


