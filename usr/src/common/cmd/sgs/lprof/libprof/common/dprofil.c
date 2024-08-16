/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/dprofil.c	1.4"
/*
*	dprofil - set up and turn on SIGPROF signaling
*/
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>
#include <sys/siginfo.h>
#include "dprof.h"
#include "mach_type.h"

/*
*	Check Return value.
*/

#define CR(c,v,str) if ((c) != (v)) { perror("ERROR"); \
	fprintf(stderr,"\t%s\n", (str)); }

typedef struct timeval TVAL;
typedef struct itimerval ITRVAL;


static TVAL tv_zero = { 0L, 0L };
static TVAL tv_one = { 0L, 1L };
static struct sigaction psig;
static struct sigaction asig;

static ITRVAL venable;
static ITRVAL vdisable;

static clear_sigprof();
static set_sigprof();

#ifdef DEBUG
static unsigned long	incount;
static unsigned long	outcount;
#endif

/*
*	Profil initialization.
*/
void _dprofil(unsigned int sc)
{
	/*
	*	Initialize the timers.
	*/
	venable.it_interval = tv_one;
	venable.it_value = tv_one;
	vdisable.it_interval = tv_zero;
	vdisable.it_value = tv_zero;

	/*
	*	Initialize SIGPROF timer.
	*/
	if (sc <= 0) {
		clear_sigprof();
#ifdef DEBUG
		printf("iprofil: disable sigprof -");
		printf(" incount=%d, outcount=%d\n",incount,outcount);
#endif
	} else {
#ifdef DEBUG
		incount = 0;
		outcount = 0;
		printf("iprofil: enable sigprof -");
		printf(" incount=%d, outcount=%d\n",incount,outcount);
#endif
		set_sigprof();
	}
}

static clear_sigprof()
{
	CR(setitimer(ITIMER_PROF, &vdisable, (ITRVAL *) NULL), 0, "setitimer");
}

static set_sigprof()
{
	void _tick_prof();

	CR(sigemptyset(&psig.sa_mask), 0, "sigemptyset");
	CR(sigaddset(&psig.sa_mask, SIGPROF), 0, "sigaddset");
	CR(sigprocmask(SIG_BLOCK, &psig.sa_mask, 0), 0, "sigprocmask");
	psig.sa_handler = _tick_prof;
	psig.sa_flags = SA_RESTART | SA_SIGINFO;
	CR(setitimer(ITIMER_PROF, &venable, (ITRVAL *) NULL), 0, "setitimer");
	CR(sigaction(SIGPROF, &psig, 0), 0, "sigaction");
	CR(sigprocmask(SIG_UNBLOCK, &psig.sa_mask, 0), 0, "sigprocmask");
}

/*
*	_tick_prof()  Get the pc, check, increment the histogram.
*/


extern SOentry	*_curr_SO;
extern SOentry	*_search();
extern unsigned short	_out_tcnt;

/* _tick_prof gets the pc and see if it's in range 
 * if not, call search program, in any case increment either
 * the correct histogram "bucket" or an out-of-profiling-range
 * counter						 */

void _tick_prof(int sig, siginfo_t *info_p, ucontext_t *cntxt_p)
{
	SOentry	*tmp_SOentry =  _curr_SO;	/* tmp pointer to the current _SOentry */
	register unsigned int	ndx;		/* index for histogram */
	register unsigned long	pc;		/* pc received from system */
	

	pc = cntxt_p->uc_mcontext.gregs[PC];	/* get pc from system */

	if ( ( pc < tmp_SOentry->textstart) || ( pc > tmp_SOentry->endaddr) ) 
		tmp_SOentry = _search(pc);

	 if (tmp_SOentry  != NULL) {

		ndx = pc - tmp_SOentry->textstart;
		ndx >>= 3;
		if (ndx < tmp_SOentry->size) 
			tmp_SOentry->tcnt[ndx]++;
	}
	else 
		_out_tcnt++;

}



