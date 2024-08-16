/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntp_unixclk.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * ntp_unixclock.c - routines for reading and adjusting a 4BSD-style
 *		     system clock
 */

#include <stdio.h>
#ifdef SYSV
#include <sys/fcntl.h>
#ifndef L_SET
#define L_SET 0
#endif /* L_SET */
#endif /* SYSV */
#include <nlist.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp_unixtime.h"
#define	CLOCK_ADJ	4

extern int debug;

/*
 * These routines (init_systime, get_systime, step_systime, adj_systime)
 * implement an interface between the (more or less) system independent
 * bits of NTP and the peculiarities of dealing with the Unix system
 * clock.  These routines will run with good precision fairly independently
 * of your kernel's value of tickadj.  I couldn't tell the difference
 * between tickadj==40 and tickadj==5 on a microvax, though I prefer
 * to set tickadj == 500/hz when in doubt.  At your option you
 * may compile this so that your system's clock is always slewed to the
 * correct time even for large corrections.  Of course, all of this takes
 * a lot of code which wouldn't be needed with a reasonable tickadj and
 * a willingness to let the clock be stepped occasionally.  Oh well.
 */

/*
 * Clock variables.  We round calls to adjtime() to adj_precision
 * microseconds, and limit the adjustment to tvu_maxslew microseconds
 * (tsf_maxslew fractional sec) in one four second period.  As we are
 * thus limited in the speed and precision with which we can adjust the
 * clock, we compensate by keeping the known "error" in the system time
 * in sys_offset.  This is added to timestamps returned by get_systime().
 * We also remember the clock precision we computed from the kernel in
 * case someone asks us.
 */
long adj_precision;	/* adj precision in usec (tickadj) */
long tvu_maxslew;	/* maximum adjust doable in CLOCK_ADJ sec (usec) */

unsigned long tsf_maxslew;	/* same as above, as long format */

static l_fp sys_offset;		/* correction for current system time */

/*
 * Tables for converting between time stamps and struct timeval's
 * are in the library.  Reference them here.
 */
extern u_long ustotslo[];
extern u_long ustotsmid[];
extern u_long ustotshi[];

extern long tstoushi[];
extern long tstousmid[];
extern long tstouslo[];

/*
 * init_systime - initialize the system clock support code, return
 *		  clock precision.
 *
 * Note that this code obtains to kernel variables related to the local
 * clock, tickadj and tick.  The code knows how the Berkeley adjtime
 * call works, and assumes these two variables are obtainable and are
 * used in the same manner.  Tick is supposed to be the number of
 * microseconds which are added to the system clock at clock interrupt
 * time when the time isn't being slewed.  Tickadj is supposed to be
 * the number of microseconds which are added or subtracted from tick when
 * the time is being slewed.
 *
 * If either of these two variables is missing, or is there but is used
 * for a purpose different than that described, you are SOL and may have
 * to do some custom kludging.
 *
 * This really shouldn't be in here.
 */
void
init_systime()
{
	u_long tickadj;
	u_long tick;
	u_long hz;
	void clock_parms();

	/*
	 * Obtain the values
	 */
	clock_parms(&tickadj, &tick);

	/*
	 * If tickadj or hz wasn't found, we're doomed.  If hz is
	 * unreasonably small, forget it.
	 */
	if (tickadj == 0 || tick == 0) {
		syslog(LOG_ERR, "tickadj or tick unknown, exiting");
		exit(3);
	}
	if (tick > 65535) {
		syslog(LOG_ERR, "tick value of %lu is unreasonably large",
		    tick);
		exit(3);
	}

	/*
	 * Estimate hz from tick
	 */
	hz = 1000000L / tick;

	/*
	 * Set adj_precision and the maximum slew based on this.  Note
	 * that maxslew is set slightly shorter than it needs to be as
	 * insurance that all slews requested will complete in CLOCK_ADJ
	 * seconds.
	 */
#ifdef ADJTIME_IS_ACCURATE
	adj_precision = 1;
#else
	adj_precision = tickadj;
#endif /* ADJTIME_IS_ACCURATE */
	tvu_maxslew = tickadj * (hz-1) * CLOCK_ADJ;
	if (tvu_maxslew > 999990) {
		/*
		 * Don't let the maximum slew exceed 1 second in 4.  This
		 * simplifies calculations a lot since we can then deal
		 * with less-than-one-second fractions.
		 */
		tvu_maxslew = (999990/adj_precision) * adj_precision;
	}
	TVUTOTSF(tvu_maxslew, tsf_maxslew);

#ifdef DEBUG
	if (debug)
		printf(
	"adj_precision = %d, tvu_maxslew = %d, tsf_maxslew = 0.%08x\n",
		    adj_precision, tvu_maxslew, tsf_maxslew);
#endif

	/*
	 * Set the current offset to 0
	 */
	sys_offset.l_ui = sys_offset.l_uf = 0;
}


/*
 * get_systime - return the system time in timestamp format
 */
void
get_systime(ts)
	l_fp *ts;
{
	struct timeval tv;

#if !defined(SLEWALWAYS) && !defined(LARGETICKADJ)
	/*
	 * Quickly get the time of day and convert it
	 */
	(void) gettimeofday(&tv, (struct timezone *)NULL);
	TVTOTS(&tv, ts);
	ts->l_uf += TS_ROUNDBIT;	/* guaranteed not to overflow */
#else
	/*
	 * Get the time of day, convert to time stamp format
	 * and add in the current time offset.  Then round
	 * appropriately.
	 */
	(void) gettimeofday(&tv, (struct timezone *)NULL);
	TVTOTS(&tv, ts);
	L_ADD(ts, &sys_offset);
	if (ts->l_uf & TS_ROUNDBIT)
		L_ADDUF(ts, (unsigned long) TS_ROUNDBIT);
#endif	/* !defined(SLEWALWAYS) && !defined(LARGETICKADJ) */
	ts->l_ui += JAN_1970;
	ts->l_uf &= TS_MASK;
}


/*
 * step_systime - do a step adjustment in the system time (at least from
 *		  NTP's point of view.
 */
void
step_systime(ts)
	l_fp *ts;
{
#ifndef SLEWALWAYS
	struct timeval timetv, adjtv;
	int isneg = 0;
	extern char *lfptoa();
	extern char *tvtoa();
	extern char *utvtoa();

	/*
	 * We can afford to be sloppy here since if this is called
	 * the time is really screwed and everything is being reset.
	 */
	L_ADD(&sys_offset, ts);

	if (L_ISNEG(&sys_offset)) {
		isneg = 1;
		L_NEG(&sys_offset);
	}
	TSTOTV(&sys_offset, &adjtv);

	(void) gettimeofday(&timetv, (struct timezone *)NULL);
#ifdef DEBUG
	if (debug > 3)
		printf("step: %s, sys_offset = %s, adjtv = %s, timetv = %s\n",
		    lfptoa(&ts, 9), lfptoa(&sys_offset, 9), tvtoa(&adjtv),
		    utvtoa(&timetv));
#endif
	if (isneg) {
		timetv.tv_sec -= adjtv.tv_sec;
		timetv.tv_usec -= adjtv.tv_usec;
		if (timetv.tv_usec < 0) {
			timetv.tv_sec--;
			timetv.tv_usec += 1000000;
		}
	} else {
		timetv.tv_sec += adjtv.tv_sec;
		timetv.tv_usec += adjtv.tv_usec;
		if (timetv.tv_usec >= 1000000) {
			timetv.tv_sec++;
			timetv.tv_usec -= 1000000;
		}
	}
	if (settimeofday(&timetv, (struct timezone *)NULL) != 0)
		syslog(LOG_ERR, "Can't set time of day: %m");
#ifdef DEBUG
	if (debug > 3)
		printf("step: new timetv = %s\n", utvtoa(&timetv));
#endif
	sys_offset.l_ui = sys_offset.l_uf = 0;
	
#else
	/*
	 * Just add adjustment into the current offset.  The update
	 * routine will take care of bringing the system clock into
	 * line.
	 */
	L_ADD(&sys_offset, ts);
#endif	/* SLEWALWAYS */
}


/*
 * adj_systime - called once every CLOCK_ADJ seconds to make system time
 *		 adjustments.
 */
void
adj_systime(adj)
	long adj;
{
	register unsigned long offset_i, offset_f;
	register long temp;
	register unsigned long residual;
	register int isneg = 0;
	struct timeval adjtv, oadjtv;
	extern char *mfptoa();
	extern char *umfptoa();
	extern char *utvtoa();
	extern char *tvtoa();

	/*
	 * Move the current offset into the registers
	 */
	offset_i = sys_offset.l_ui;
	offset_f = sys_offset.l_uf;
	/*
	 * Add the new adjustment into the system offset.  Adjust the
	 * system clock to minimize this.
	 */
	M_ADDF(offset_i, offset_f, adj);
	if (M_ISNEG(offset_i, offset_f)) {
		isneg = 1;
		M_NEG(offset_i, offset_f);
	}
#ifdef DEBUG
	if (debug > 4)
		printf("adj_systime(%s): offset = %s%s\n",
		    mfptoa((adj<0?-1:0), adj, 9), isneg?"-":"",
		    umfptoa(offset_i, offset_f, 9));
#endif

	adjtv.tv_sec = 0;
	if (offset_i > 0 || offset_f >= tsf_maxslew) {
		/*
		 * Slew is bigger than we can complete in
		 * the adjustment interval.  Make a maximum
		 * sized slew and reduce sys_offset by this
		 * much.
		 */
		M_SUBUF(offset_i, offset_f, tsf_maxslew);
		if (!isneg) {
			adjtv.tv_usec = tvu_maxslew;
		} else {
			adjtv.tv_usec = -tvu_maxslew;
			M_NEG(offset_i, offset_f);
		}

#ifdef DEBUG
		if (debug > 4)
			printf(
			    "maximum slew: %s%s, remainder = %s\n",
			    isneg?"-":"", umfptoa(0, tsf_maxslew, 9),
			    mfptoa(offset_i, offset_f, 9));
#endif
	} else {
		/*
		 * We can do this slew in the time period.  Do our
		 * best approximation (rounded), save residual for
		 * next adjustment.
		 *
		 * Note that offset_i is guaranteed to be 0 here.
		 */
		TSFTOTVU(offset_f, temp);
#ifndef ADJTIME_IS_ACCURATE
		/*
		 * Round value to be an even multiple of adj_precision
		 */
		residual = temp % adj_precision;
		temp -= residual;
		if (residual << 1 >= adj_precision)
			temp += adj_precision;
#endif /* ADJTIME_IS_ACCURATE */
		TVUTOTSF(temp, residual);
		M_SUBUF(offset_i, offset_f, residual);
		if (isneg) {
			adjtv.tv_usec = -temp;
			M_NEG(offset_i, offset_f);
		} else {
			adjtv.tv_usec = temp;
		}

#ifdef DEBUG
		if (debug > 4)
			printf(
		"slew adjtv = %s, adjts = %s, sys_offset = %s\n",
			    tvtoa(&adjtv), umfptoa(0, residual, 9),
			    mfptoa(offset_i, offset_f, 9));
#endif
	}

	sys_offset.l_ui = offset_i;
	sys_offset.l_uf = offset_f;

	if (adjtv.tv_usec == 0)
		return;
	if (adjtime(&adjtv, &oadjtv) != 0)
		syslog(LOG_ERR, "Can't do time adjustment: %m");
	if (oadjtv.tv_sec != 0 || oadjtv.tv_usec != 0) {
		syslog(LOG_ERR, "Previous time adjustment didn't complete");
#ifdef DEBUG
		if (debug > 4)
			printf(
			    "Previous adjtime() incomplete, residual = %s\n",
			    tvtoa(&oadjtv));
#endif
	}
}



#ifdef SVR42
/*
 * clock_parms - return (the SVR4.2 equivalent of) tickadj and tick values
 * In SVR4.2, these are not contained in kernel variables.  The constant
 * SKEW (defined in svc/clock.c) is equivalent to tickadj.  If the time
 * delta is greater than 60s, a larger value (10*SKEW) is used.  Here,
 * we assume that the time delta will always be small, so SKEW is always
 * returned.
 */

void
clock_parms(tickadj, tick)
	u_long *tickadj;
	u_long *tick;
{
#define SKEW	(8000/HZ)	/* standard clock skew per tick, in microsec */

	*tickadj = (u_long) SKEW;
	*tick = (u_long) (1000000L / HZ);
}
#else /* SVR42 */
#ifndef NOKMEM
/*
 * clock_parms - return the local clock tickadj and tick parameters
 *
 * Note that this version grovels about in /dev/kmem to determine
 * these values.  This probably should be elsewhere.
 */
void
clock_parms(tickadj, tick)
	u_long *tickadj;
	u_long *tick;
{
	register int i;
	int kmem;
	static struct nlist nl[] =
	{
#ifdef SYSV
		{"tickadj"},
		{"tick"},
#else
		{"_tickadj"},
		{"_tick"},
#endif
		{""},
	};
	static char *kernelnames[] = {
		"/vmunix",
		"/unix",
		NULL
	};
	struct stat stbuf;
	int vars[2];

#define	K_TICKADJ	0
#define	K_TICK		1
	/*
	 * Read clock parameters from kernel
	 */
	kmem = open("/dev/kmem", O_RDONLY);
	if (kmem < 0) {
		syslog(LOG_ERR, "Can't open /dev/kmem for reading: %m");
#ifdef	DEBUG
		if (debug)
			perror("/dev/kmem");
#endif
		exit(3);
	}

	for (i = 0; kernelnames[i] != NULL; i++) {
		if (stat(kernelnames[i], &stbuf) == -1)
			continue;
		if (nlist(kernelnames[i], nl) >= 0)
			break;
	}
	if (kernelnames[i] == NULL) {
		syslog(LOG_ERR,
		  "Clock init couldn't find kernel as either /vmunix or /unix");
		exit(3);
	}

	for (i = 0; i < (sizeof(vars)/sizeof(vars[0])); i++) {
		long where;

		vars[i] = 0;
		if ((where = nl[i].n_value) == 0) {
			syslog(LOG_ERR, "Unknown kernal var %s",
			       nl[i].n_name);
			continue;
		}
		if (lseek(kmem, where, L_SET) == -1) {
			syslog(LOG_ERR, "lseek for %s fails: %m",
			       nl[i].n_name);
			continue;
		}
		if (read(kmem, &vars[i], sizeof(int)) != sizeof(int)) {
			syslog(LOG_ERR, "read for %s fails: %m",
			       nl[i].n_name);
		}
	}
#ifdef	DEBUG
	if (debug) {
		printf("kernel vars: tickadj = %d, tick = %d\n",
		       vars[K_TICKADJ], vars[K_TICK]);
	}
#endif
	close(kmem);

	*tickadj = (u_long)vars[K_TICKADJ];
	*tick = (u_long)vars[K_TICK];

#undef	K_TICKADJ
#undef	K_TICK
}
#endif /* !NOKMEM */
#endif /* SVR4.2 */
