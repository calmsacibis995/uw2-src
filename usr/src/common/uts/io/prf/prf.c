/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/prf/prf.c	1.7"
#ident	"$Header: $"
/*
 * UNIX Operating System Profiler
 *
 * Paths and addresses of loaded kernel modules are returned to user level
 * from the in-core symbol table.  The user program opens the modules for
 * symbols and addresses, and loads the profiler with a table of sorted
 * kernel symbols.  This is done this way because the in-core symbol table
 * does not contain static symbols.  At each clock interrupt a binary 
 * search locates the counter for the interval containing the captured 
 * PC and increments it.  The last counter is used to hold the User mode 
 * counts.
 */

#include <util/types.h>
#include <svc/errno.h>
#include <io/uio.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/prf/prf.h>
#include <util/plocal.h>	/* pick up per-processor definitions,	*/
#define ENG_NUM	l.eng_num	/* and set the id for use in prfintr()	*/

STATIC struct mprf *prfdata;	/* The prf data space:			*/
				/*					*/
				/* prfdata contains all the user	*/
				/* visible profiling information in one	*/
				/* kmem_alloc'd buffer, to facilitate	*/
				/* the use of mmap. Nengine and prfstat	*/
				/* are not included in prfdata, because	*/
				/* they need to be defined in other	*/
				/* modules, and the prf driver might	*/
				/* not be loaded.			*/
				/*					*/
				/* The first struct has number of 	*/
				/* symbols (prfmax), and size of the 	*/
				/* list of symbol names.		*/
				/* 					*/
				/* The next prfmax structs have a	*/
				/* symbol addr and an offset into the	*/
				/* the name table.  The name tbl is	*/
				/* padded out to a word boundary.	*/
				/* 					*/
				/* Following the name table are sets of	*/
				/* counters, one for each engine.  Each	*/
				/* set has prfmax+1 counters, with the 	*/
				/* last one being used for usr mode.	*/
				/* 					*/
				/* The space for the counters is not	*/
				/* zeroed before use, since the only 	*/
				/* meaningful to do with them is to	*/
				/* take differences.			*/

STATIC struct mprf *prfmoddata;	/* same layout as above, but holds	*/
				/* information about loaded modules	*/
				/*					*/
				/* modprf() allocates prfmoddata, and 	*/
				/* fills in the addresses, and offsets.	*/
				/* The name space contains the paths	*/

STATIC struct mprf *prfaddr;	/* Pointer to the function addresses.	*/
				/* prfaddr should always be prfdata+1.	*/
				/* This is done to keep from having	*/
				/* to add 1 to prfaddr during every 	*/
				/* iteration of the search in prfintr,	*/
				/* since prfintr is called from the 	*/
				/* clock handler many times per second.	*/

STATIC unsigned **prfctr_p;	/* an array of pointers, (one pointer 	*/
				/* for each engine), that point the 	*/
				/* appropriate counter array in prfdata	*/
				/* This is done for the same reason as	*/
				/* as prfaddr, to pre-compute as much	*/
				/* as possible, so the work does not	*/
				/* have to been every clock tick.	*/

unsigned prfstat;		/* Flag indicates whether profiling is	*/
				/* enabled.  Defined and checked in the	*/
				/* clock handler.			*/

STATIC unsigned prf_size;	/* The total size of prfdata in bytes	*/
STATIC unsigned prfmod_size;	/* The total size of prfmoddata 	*/

extern time_t time;		/* The time in sec since Jan. 1 1970.	*/

STATIC time_t prf_time;		/* Timestamp of the last load of the	*/
				/* profiling addresses.  		*/

STATIC int prfld = 0;		/* Indicates whether modules are 	*/
				/* locked, and whether profiling	*/
				/* addresses are being loaded/unloaded	*/
				/* 0: starting state, modules unlocked	*/
				/* 1: modules being locked		*/
				/* 2: modules locked,ready for addresses*/
				/* 3: addresses being loaded		*/
				/* 4: addresses loaded, ready to enable	*/
				/* 5: addresses being unloaded		*/
				/* 6+(even values): addresses being read*/

extern Nengine;			/* number of processors	in system	*/

int prfdevflag = D_MP;		/* don't bind to a particular cpu	*/

#define PRF_HIER ((uchar_t)17)	/* arbitrary number within driver range	*/

/*
 *+ prf_mutex is a global spin lock that protects the prfld flag in
 *+ prfwrite, which in turn protects the buffer allocations and contents
 */
lock_t prf_mutex;
LKINFO_DECL(prf_lkinfo, "PRF::prf_mutex", 0);

/*
 * void prfinit()
 *
 *	Initialize lock for prfload/prfunload, set prfdata to NULL,
 *	and set prfstat to 0.
 *
 * Calling/Exit State:
 *
 *	Called at boot time.
 *
 * Description:
 *
 *	This function initializes prf_mutex for use by prfwrite()
 *	to protect against multiple processes trying to write
 *	the profiling addresses concurrently.
 */

void
prfinit() {

	LOCK_INIT( &prf_mutex, PRF_HIER, PL0, &prf_lkinfo, KM_NOSLEEP );
	prfctr_p = (unsigned **) kmem_alloc(Nengine*sizeof(int),KM_SLEEP);
	prfdata = (struct mprf *)0;
	prfstat = 0;
}

/*
 * int prfopen()
 *
 *	Stub for open of /dev/prf.
 *
 * Calling/Exit State:
 *
 *	Called when /dev/prf is opened.
 *
 * Description:
 *
 *	Does nothing of importance, but allows file descriptors, etc.
 *	to be initialized.
 */
/* ARGSUSED */
int
prfopen(devp, mode, otyp, cr)
	dev_t *devp;
	int mode;
	int otyp;
	struct cred *cr;
{
	return 0;
}

/*
 * int prfclose()
 *
 *	Stub for close of /dev/prf.
 *
 * Calling/Exit State:
 *
 *	Called when /dev/prf is closed.
 *
 * Description:
 *
 *	Does nothing of importance, but allows file descriptors, etc.
 *	to be closed.
 */
/* ARGSUSED */
int
prfclose(dev, mode, otyp, cr)
	dev_t dev;
	int mode;
	struct cred *cr;
{
	return 0;
}

/*
 * int prfread()
 *
 *	Read profiling counters.
 *
 * Calling/Exit State:
 *
 *	Called upon read of /dev/prf.
 *
 * Description:
 *
 *	Checks value of prfmax to determine that valid addresses have been
 *	loaded, and then copies profiling addresses and counters to user space.
 */
/* ARGSUSED */
int
prfread(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	int error;
	pl_t pl;

	pl = LOCK( &prf_mutex, PL0);
	if( prfld & 1 ) {
		UNLOCK( &prf_mutex, pl );
		return ENXIO;
	}

	if (prfdata) {
		prfld += 2;
		UNLOCK( &prf_mutex, pl );
		error = uiomove((caddr_t) prfdata, 
		  (unsigned)min(uiop->uio_resid, prf_size), UIO_READ, uiop);
		pl = LOCK( &prf_mutex, PL0);
		prfld -= 2;
		UNLOCK( &prf_mutex, pl );
		return error;
	}
	if( prfmoddata  ) {
		prfld += 2;
		UNLOCK( &prf_mutex, pl );
		error = uiomove((caddr_t) prfmoddata, 
		    (unsigned)min(uiop->uio_resid, prfmod_size),
		    UIO_READ, uiop);
		pl = LOCK( &prf_mutex, PL0);
		prfld -= 2;
		UNLOCK( &prf_mutex, pl );
		return error;
	}
	UNLOCK( &prf_mutex, pl );
	return ENXIO;
}

/*
 * int prfwrite()
 *
 *     Load profiling addresses.
 *
 * Calling/Exit State:
 *
 *      Called upon write of /dev/prf.
 *
 * Description:
 *
 *	Checks that at least 3 addresses are being loaded.
 *	Checks that profiling is not currently enabled, and that 
 *	addresses are not currently being loaded.
 *
 *	Allocates space for names, addresses, and counters, 
 *	and sets prfctr_p to point to counter space.
 *
 *	Does sanity check on addresses to make sure they are sorted.
 *
 *	Lastly sets prfmax to the number of addresses loaded.
 *	Sets prfaddr to be prfdata + 1, sets prfctr_p to point 
 *	to the counter portion of prfdata, and saves time in prf_time.
 *
 */

/* ARGSUSED */
int
prfwrite(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	register struct mprf  *ip;
	register unsigned *prfctr;
	register int i;
	int err;
	int symcnt;			/* number of text symbols	*/
	int namesz;			/* size of symbol names		*/
	pl_t pl;
	int cnt=uiop->uio_resid;
	struct mprf firstprf;

	if (prfstat)
		return EBUSY;

	pl = LOCK( &prf_mutex, PL0);
	if( prfld >= 6 ) {	/* disabled, but still draining */
		UNLOCK( &prf_mutex, pl );
		return( EBUSY );
	}
	if( prfld & 1 )  {	/* being loaded / written / unloaded */
		UNLOCK( &prf_mutex, pl );
		return( EAGAIN );
	}
	if( prfld == 0 ) {
		UNLOCK( &prf_mutex, pl );
		return EINVAL;
	}
	prfld = 3;
	UNLOCK( &prf_mutex, pl );
	err = uiomove((caddr_t) &firstprf, sizeof(struct mprf), 
	    UIO_WRITE, uiop);

	if (err) {
		pl = LOCK( &prf_mutex, PL0);
		prfld = 2;
		UNLOCK( &prf_mutex, pl );
		return err;
	}
	namesz = firstprf.mprf_offset;
	symcnt = firstprf.mprf_addr;

	if( symcnt < 3 ) {
		pl = LOCK( &prf_mutex, PL0);
		prfld = 2;
		UNLOCK( &prf_mutex, pl );		
		return EINVAL;
	}

	if( namesz % sizeof(int) ) 	/* check for word alignment	*/
		namesz += sizeof(int) - (namesz % sizeof(int));

	prf_size = (symcnt+1)*sizeof(struct mprf) + namesz 
	  + (symcnt+1)*Nengine*sizeof(unsigned int);

	prfdata = (struct mprf *) kmem_alloc( prf_size , KM_SLEEP );

	prfdata->mprf_offset = namesz;
	prfdata->mprf_addr = symcnt;

	err = uiomove((caddr_t) (prfdata+1), cnt-sizeof(struct mprf),
	   UIO_WRITE, uiop);
	if (err) {
		pl = LOCK( &prf_mutex, PL0);	
		prfld = 2;
		kmem_free( prfdata, prf_size );
		UNLOCK( &prf_mutex, pl );
		return err;
	}

	for (ip = prfdata+1; ip < prfdata+symcnt; ip++)
		if (ip->mprf_addr > (ip+1)->mprf_addr) {
			pl = LOCK( &prf_mutex, PL0);	
			prfld = 2;
			kmem_free( prfdata, prf_size );
			UNLOCK( &prf_mutex, pl );
			return EINVAL;
		}

/*	LINTED pointer alignment */
	prfctr = (unsigned *)((char *)( prfdata + (prfdata->mprf_addr+1) )
	  + prfdata->mprf_offset );
	
	prfaddr = prfdata + 1;
	for( i = 0; i < Nengine; i++ )
		prfctr_p[i] = prfctr+i*(prfdata->mprf_addr+1);

	prf_time = time;
	pl = LOCK( &prf_mutex, PL0);	
	prfld = 4;
	UNLOCK( &prf_mutex, pl );
	return 0;
}

/*
 * STATIC int prfload()
 *
 *	Get module names and addresses.
 *
 * Calling/Exit State:
 *
 *	Called from ioctl of /dev/prf.
 *
 * Description:
 *
 *	Calls modprf() to get kernel module names and addresses.  
 *	modprf() allocates space for prfmoddata, which holds all of the 
 *	pertinent info.  See above for layout of data within prfmoddata.
 *	modprf() also locks all currently (and subsequently) loaded 
 *	modules into memory.
 *
 */
STATIC int
prfload() {
	register int err;

	pl_t pl;
	int mod_prf();

	pl = LOCK( &prf_mutex, PL0);
	if( prfld != 0 ) {
		if( prfld & 1 ) {
			UNLOCK( &prf_mutex, pl );
			return EAGAIN;
		}
		UNLOCK( &prf_mutex, pl );
		return EINVAL;
	}
	prfld = 1;
/*
 *	check that the addresses have not already been loaded,
 *	call mod_prf to load addresses and lock modules.  
 *	mod_prf will fail if modules are already locked, which
 *	should only be the case if prfdata is non-NULL.
 */
	if( prfdata ) {	/* This shouldn't happen if prfld is 0 */
			/* it means that modprf has failed previously */
		prfld = 0;
		UNLOCK( &prf_mutex, pl );
		return EBUSY;
	}

	UNLOCK( &prf_mutex, pl );
	if( (err = mod_prf( &prfmoddata )) != 0 ) {
		pl = LOCK( &prf_mutex, PL0);
		prfld = 0;
		UNLOCK( &prf_mutex, pl );
		return err;
	}
/*
 *	prfmod_size is mprf_addr+1 addresses + namesz;
 */
	prfmod_size = (prfmoddata->mprf_addr + 1) * sizeof(struct mprf)
	 + prfmoddata->mprf_offset;

	pl = LOCK( &prf_mutex, PL0);
	prfld = 2;
	UNLOCK( &prf_mutex, pl );
	return 0;
}

/*
 * STATIC int prfunload()
 *
 *	Unload profiling addresses.
 *
 * Calling/Exit State:
 *
 *	Called from ioctl of /dev/prf.
 *
 * Description:
 *
 *	Calls modprf() to release locked-in loadable modules, then
 *	free's prfdata, and sets prfdata and prfaddr to NULL.
 */
STATIC int
prfunload() {
	pl_t pl;
	int mod_prf();

	if( prfstat )
		return EBUSY;		/* profiling is in use	*/

	pl = LOCK( &prf_mutex, PL0);
	if( prfld & 1 ) {
		UNLOCK( &prf_mutex, pl );
		return EAGAIN;
	}
	if( prfld >= 6 ) {	/* turned off, but still draining */
		UNLOCK( &prf_mutex, pl );
		return EBUSY;
	}
	prfld = 5;
	UNLOCK( &prf_mutex, pl );
	(void) mod_prf( (struct mprf **)0 );
	pl = LOCK( &prf_mutex, PL0);
	if( prfdata )
		kmem_free( prfdata, prf_size );
	if( prfmoddata )
		kmem_free( prfmoddata, prfmod_size );
	prfdata = (struct mprf *)0;
	prfaddr = (struct mprf *)0;
	prf_size = 0;
	prfmoddata = (struct mprf *)0;
	prfmod_size = 0;
	prfld = 0;
	prf_time = (time_t)0;
	UNLOCK( &prf_mutex, pl );

	return 0;
}
/*
 * int prfioctl()
 *
 *	Miscellaneous support for user processes to do kernel profiling.
 *
 * Calling/Exit State:
 *
 *	Called upon ioctl of /dev/prf.
 *
 * Description:
 *
 *	Depending upon value of cmd, does one of the following:
 *
 *		enables[disables] profiling
 *		loads[unloads] function names/addresses
 *		returns one of:
 *			number of processors
 *			prf status (enabled or disabled)
 *			number of text symbols
 *			size of name table
 */

/* ARGSUSED */
int
prfioctl(dev, cmd, arg, mode, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	register int error = 0;
	pl_t pl;

	switch (cmd) {

	case PRF_ENGINE:
		*rvalp = Nengine;
		break;
	case PRF_STAT:
		pl = LOCK( &prf_mutex, PL0);
		if( prfstat )
			*rvalp = prf_time;
		else
			*rvalp = 0;
		UNLOCK( &prf_mutex, pl );
		break;
	case PRF_MAX:
		pl = LOCK( &prf_mutex, PL0);
		if( prfld != 3 && prfdata )
			*rvalp = prfdata->mprf_addr;
		else 
			*rvalp = 0;
		UNLOCK( &prf_mutex, pl );
		break;
	case PRF_NAMESZ:
		pl = LOCK( &prf_mutex, PL0);
		if( prfld != 3 && prfdata )
			*rvalp = prfdata->mprf_offset;
		else if ( prfld != 1 && prfmoddata)
			*rvalp = prfmoddata->mprf_offset;
		else 
			*rvalp = 0;
		UNLOCK( &prf_mutex, pl );
		break;
	case PRF_SIZE:
		pl = LOCK( &prf_mutex, PL0);
		if( prfld != 3 && prfdata )
			*rvalp = prf_size;
		else if (prfld != 1 && prfmoddata)
			*rvalp = prfmod_size;
		else 
			*rvalp = 0;
		UNLOCK( &prf_mutex, pl );
		break;
	case PRF_ENABLE:
		pl = LOCK( &prf_mutex, PL0);
		if ( prfld != 3 && prfdata ) 
			prfstat = 1;
		else
			error = EINVAL;
		UNLOCK( &prf_mutex, pl );
		break;
	case PRF_DISABLE:
		pl = LOCK( &prf_mutex, PL0);
		if ( prfstat ) {
			prfstat = 0;
			UNLOCK( &prf_mutex, pl );
			error = prfunload();
		} else {
			UNLOCK( &prf_mutex, pl );
			(void) prfunload();
			error = EINVAL;
		}
		break;
	case PRF_LOAD:
		error = prfload();
		break;
	case PRF_UNLOAD:
		error = prfunload();
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * void prfintr( pc, usermode )
 * 
 *	Kernel profiler "interrupt" handler - increments profiling 
 *	counters based on valued of program counter (pc).
 *
 * Calling/Exit State:
 *
 *	Called from per processor local clock handlers.
 *
 * Description:
 *
 *	Each processor has a reserved portion of prfctr space which is
 *	indexed via l.eng_num.  prfctr_p[l.eng_num] points to the start
 *	of each processor's counter space.
 *
 *	If usermode is set, increment location that corresponds to prfmax,
 *	otherwise, search array of symbol addresses for nearest address,
 *	and increment counter value associated with that address.
 */
/* ARGSUSED */
void
prfintr(pc, usermode)
	register  unsigned  pc;
	int  usermode;
{
	register int  hh, ll, mm;

	if (usermode)
		(*(prfctr_p[ENG_NUM]+prfdata->mprf_addr))++;
	else {
		ll = 0;
		hh = prfdata->mprf_addr;
		while ((mm = (ll + hh) / 2) != ll)
			if (pc >= (prfaddr+mm)->mprf_addr)
				ll = mm;
			else
				hh = mm;
		(*(prfctr_p[ENG_NUM]+mm))++;
	}
}
