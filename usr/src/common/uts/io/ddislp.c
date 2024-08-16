/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/ddislp.c	1.14"
#ident	"$Header: $"

/*
 * sleep/wakeup -- for backwards compatibility with SVR4 DDI/DKI
 */

#include <mem/kmem.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define	_DDI_C

#include <io/ddi.h>	/* must come last */

#define PMASK	0177

static int convertpri(int);

#define	SLEEP_HIER	16 /* exact value not important, since won't */
			   /* be holding locks coming into sleep/wakeup */

/*
 * have to map from sleep's caddr_t to SV_WAIT's sv_t 
 */
struct sleeper {
	caddr_t		sl_chan;
	sv_t		sl_synch;
	struct sleeper 	*sl_nextp;
};
typedef struct sleeper sleeper_t;

/*
 * sleep-wakeup hashing:  Each entry in sleepq[] points
 * to the front and back of a linked list of sleeper_t's.
 * NSLEEPQ must be a power of 2.  Sqhash(x) is used to index into
 * sleepq[] based on the sleep channel.
 */

#define NSLEEPQ		64
#define SQHASH(X)	(&sleepq[((int)(X) >> 3) & (NSLEEPQ - 1)])

struct sleepq {
	sleeper_t	*sq_first;
	lock_t		sq_lock;
};

static struct sleepq sleepq[NSLEEPQ];
LKINFO_DECL(sq_lkinfo, "DDI:sleep:sleep queue", LK_SLEEP);

/* use this one as backup, in case cannot allocate sleepers */
STATIC sv_t	ddislp_synch;

/*
 * void
 * ddisleep_init(void)
 *	allocate and initialize sleepq locks
 *
 * Calling/Exit State:
 *	no locks held on entry or exit
 */
void
ddisleep_init(void)
{
	int i;

	for (i = 0; i < NSLEEPQ; ++i) {
		LOCK_INIT(&sleepq[i].sq_lock, SLEEP_HIER, PLHI, &sq_lkinfo,
			  KM_NOSLEEP);
	}
	SV_INIT(&ddislp_synch);
	return;
}


/*
 * int
 * sleep(caddr_t chan, int disp)
 *	Give up the processor till a wakeup occurs on chan.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 *	disp determines whether the sleep can be interrupted by a
 *	signal.  If disp & PMASK <= PZERO,  a signal cannot disturb
 *	the sleep; if disp & PMASK > PZERO signals will be processed.
 *	Callers of this routine must be prepared for premature return,
 *	and check that the reason for sleeping has gone away.
 *
 *	Returns:
 *	  0 if the process is awakened by explicit wakeup()
 *	  1 if disp > PZERO, PCATCH is set in disp, and process is
 *	    awakened by signal
 *	  never returns (longjmps) if disp > PZERO, PCATCH is not set,
 *	    and process is awakened by signal
 */
int
sleep(caddr_t chan, int disp)
{
	sleeper_t	*slp, **slpp;
	struct sleepq	*sqp;
	sv_t		*synchp;
	pl_t		oldpri;
	int		priority;
	boolean_t	found_slp = B_FALSE;

	ASSERT(chan != 0);
	ASSERT(KS_HOLD0LOCKS());

	priority = convertpri(disp & PMASK);

	sqp = SQHASH(chan);
	oldpri = LOCK(&sqp->sq_lock, PLHI);

	for (slp = sqp->sq_first; slp != NULL; slp = slp->sl_nextp) {
		if (slp->sl_chan == chan)
			break;
	}

	if (slp == NULL) {
		if ((slp = kmem_alloc(sizeof(sleeper_t), KM_NOSLEEP)) == NULL) {
			/*
			 * cannot get a new sleeper structure.  use the
			 * backup ddislp_synch instead.  this may cause
			 * unnecessary wakeups, but wakeup(D3) warns
			 * that "Whenever a driver calls sleep, it should
			 * test to ensure that the event on which the
			 * driver called sleep occurred."  Thus extra
			 * wakeups shouldn't cause a problem.
			 */
			synchp = &ddislp_synch;
		} else {
			synchp = &slp->sl_synch;
			SV_INIT(synchp);
			slp->sl_chan = chan;
			slp->sl_nextp = sqp->sq_first;
			sqp->sq_first = slp;
		}
	} else
		synchp = &slp->sl_synch;

	if ((disp & PMASK) <= PZERO) {
		/* not awakened by signals */
		SV_WAIT(synchp, priority, &sqp->sq_lock);
		splx(oldpri);
		return 0;
	}

	if (SV_WAIT_SIG(synchp, priority, &sqp->sq_lock)) {
		splx(oldpri);
		return 0;	/* normal wakeup */
	}

	/* Sleep was interrupted, free up sleeper structure. */
	if (slp != NULL) {
		/*
		 * First, check to be sure we're still queued up, in case
		 * we're racing with a wakeup.
		 */
		oldpri = LOCK(&sqp->sq_lock, PLHI);
		for (slpp = &sqp->sq_first; (*slpp) != NULL;
						slpp = &(*slpp)->sl_nextp) {
			if (*slpp != slp)
				continue;
			/*
			 * Before we free the sleeper struct, we have to
			 * make sure nobody is sleeping on it.  See the
			 * comment above about wakeup(D3).  First dequeue
			 * the sleeper struct so nobody new can sleep on it.
			 */
			*slpp = slp->sl_nextp;
			UNLOCK(&sqp->sq_lock, oldpri);
			SV_BROADCAST(&slp->sl_synch, 0);
			kmem_free(slp, sizeof(sleeper_t));
			found_slp = B_TRUE;
			break;
		}
		if (!found_slp)
			UNLOCK(&sqp->sq_lock, oldpri);
	}

	splx(oldpri);

	/* If caller wants control, let it know the sleep was interrupted. */
	if (disp & PCATCH)
		return 1;

	/*
	 * The corresponding setjmp is done in the shadow code (dkibind.c)
	 * that intercepts calls to UP drivers & binds 'em to a processor.
	 * It then xlates the longjmp return into the appropriate error.
	 */
	longjmp(&u.u_qsav);
	/* NOTREACHED */
}

/*
 * void
 * wakeup(caddr_t chan)
 *	Wake up all processes sleeping on chan.
 * 
 * Calling/Exit State:
 *	No return value.
 *
 *	Find the matching sleeper_t and call SV_BROADCAST
 */
void
wakeup(caddr_t chan)
{
	struct sleepq	*sqp;
	sleeper_t	*slp, **slpp;
	pl_t		pl;

	ASSERT(chan != 0);

	sqp = SQHASH(chan);
	pl = LOCK(&sqp->sq_lock, PLHI);
	slp = *(slpp = &sqp->sq_first);
	while (slp && slp->sl_chan != chan)
		slp = *(slpp = &slp->sl_nextp);

	if (slp) {
		*slpp = slp->sl_nextp;
		UNLOCK(&sqp->sq_lock, pl);
		SV_BROADCAST(&slp->sl_synch, 0);
		kmem_free(slp, sizeof(sleeper_t));
	} else
		UNLOCK(&sqp->sq_lock, pl);

	if (SV_BLKD(&ddislp_synch))
		SV_BROADCAST(&ddislp_synch, 0);
}


/*
 * Table for mapping old priorities to new.  Tries to preserve
 * relationship for "PFOO[+-][123]" mappings.
 */
int primap[] = {
	/* old pri */	/* new pri */	/* old pri names */
	/* 0 */		PRIMEM,		/* PMEM, PSWP */
	/* 1 */		PRIMEM-1,	/* PMEM+1, PSWP+1 */
	/* 2 */		PRIMEM-2,	/* PMEM+2, PSWP+2 */
	/* 3 */		PRIMEM-3,	/* PMEM+3, PSWP+3 */
	/* 4 */		PRIMEM-3,	/* (PMEM+4) */
	/* 5 */		PRIMEM-3,	/* (PMEM+5) */
	/* 6 */		PRINOD+3,	/* (PINOD-4) */
	/* 7 */		PRINOD+3,	/* PINOD-3, PSNDD-3 */
	/* 8 */		PRINOD+2,	/* PINDD-2, PSNDD-2 */
	/* 9 */		PRINOD+1,	/* PINOD-1, PSNDD-1 */
	/* 10 */	PRINOD,		/* PINOD, PSNDD */
	/* 11 */	PRINOD-1,	/* PINDD+1, PSNDD+1 */
	/* 12 */	PRINOD-2,	/* PINOD+2, PSNDD+2 */
	/* 13 */	PRINOD-3,	/* PINOD+3, PSNDD+3 */
	/* 14 */	PRINOD-3,	/* (PINOD+4) */
	/* 15 */	PRINOD-3,	/* (PINOD+4) */
	/* 16 */	PRIBUF+3,	/* (PRIBIO-4) */
	/* 17 */	PRIBUF+3,	/* PRIBIO-3 */
	/* 18 */	PRIBUF+2,	/* PRIBIO-2 */
	/* 19 */	PRIBUF+1,	/* PRIBIO-1 */
	/* 20 */	PRIBUF,		/* PRIBIO */
	/* 21 */	PRIBUF-1,	/* PRIBIO+1 */
	/* 22 */	PRIBUF-2,	/* PRIBIO+2, PZERO-3  */
	/* 23 */	PRIMED+2,	/* PPIPE-3, PRIBIO+3, PZERO-2 */
	/* 24 */	PRIMED+1,	/* PPIPE-2, PVFS-3, PZERO-1 */
	/* 25 */	PRIMED,		/* PPIPE-1, PVFS-2, PZERO */
	/* 26 */	PRIMED-1,	/* PPIPE, PFS-1, PZERO+1 */
	/* 27 */	PRIVFS,		/* PPIPE+1, PVFS, PWAIT-3, PZERO+2 */
	/* 28 */	PRIVFS-1,	/* PPIPE+2, PVFS+1, PWAIT-2, PZERO+3 */
	/* 29 */	PRIWAIT+3,	/* PPIPE+3, PVFS+2, PWAIT-1 */
	/* 30 */	PRIWAIT,	/* PVFS+3, PWAIT */
	/* 31 */	PRIWAIT-1,	/* PWAIT+1 */
	/* 32 */	PRIWAIT-2,	/* PWAIT+2 */
	/* 33 */	PRIWAIT-3,	/* PWAIT+3 */
	/* 34 */	PRIWAIT-3,	/* (PWAIT+4) */
	/* 35 */	PRISLEP+3,	/* (PSLEP-4) */
	/* 36 */	PRISLEP+3,	/* PREMOTE-3, PSLEP-3 */
	/* 37 */	PRISLEP+2,	/* PREMOTE-2, PSLEP-2 */
	/* 38 */	PRISLEP+1,	/* PREMOTE-1, PSLEP-1 */
	/* 39 */	PRISLEP,	/* PREMOTE, PSLEP */
	/* 40 */	PRISLEP-1,	/* PREMOTE+1, PSLEP+1 */
	/* 41 */	PRISLEP-2,	/* PREMOTE+2, PSLEP+2 */
	/* 42 */	PRISLEP-3,	/* PREMOTE+3, PSLEP+3 */
	/* 43 */	PRIZERO
};
#define	NPRIMAP	(sizeof(primap) / sizeof(primap[0]))

/*
 * static int
 * convertpri(int pri)
 *	convert from SVR4 priorities to SVR4.2MP
 *
 * Calling/Exit State:
 *	returns SVR4.2MP priority, based on mapping described in
 *	"Detailed Design of Kernel Synch Primitives".
 */
static int
convertpri(int pri)
{
	ASSERT(pri >= 0);

	if (pri < NPRIMAP)
		return primap[pri];
	return primap[NPRIMAP-1];
}
