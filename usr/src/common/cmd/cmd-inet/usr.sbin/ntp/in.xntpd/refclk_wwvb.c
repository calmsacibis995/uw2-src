/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/refclk_wwvb.c	1.2"
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
 * refclock_wwvb - clock driver for the Spectracom WWVB receiver
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sgtty.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_refclock.h"
#include "ntp_unixtime.h"

#if defined(REFCLOCK) && defined(WWVB)
/*
 * This should support the use of a Spectracom Model 8170 WWVB
 * clock under Unix and on a Gizmo board.  The Spectracom clock
 * has an RS232 time-of-year output (the code assumes you don't
 * have the year support option) and a TTL-level PPS signal.
 *
 * The RS232 output is quite simple.  You send the clock an ASCII
 * letter `T' and it returns the time in the following format:
 *
 * <cr><lf>I<sp><sp>DDD<sp>HH:MM:SS<sp><sp>TZ=nn<cr><lf>
 *
 * The `I' is actually a <sp> when the clock is synchronized, and
 * a `?' when it isn't (it could also be a `*' if we set the time
 * manually, but we don't do this).  The start bit of the first <cr>
 * is supposed to be synchronized to the on-time second.
 *
 * Unfortunately, the CPU in the 8170 which generates the time code
 * is very sloppy, and hence the serial code is inaccurate.  To get
 * real good time out of the box you must synchronize to the PPS
 * signal.  This is a 100 ms positive-going TTL-level pulse with the
 * rising edge corresponding to the on-time second.  On the Gizmo
 * board there is an appropriate input, so this is easy.  On a Unix
 * machine, however, it must be massaged so that it can be received
 * on a serial port.  To do this you need to make a device consisting
 * of a one-shot (74123 or similar) to shorten the pulse to something
 * that can pass for a 19200 bps character, and an MC1488 or equivalent
 * to convert to RS232 signal levels.  It doesn't matter which character,
 * though the length of the pulse should be an integral multiple of
 * 52 us.
 *
 * We time stamp all PPS inputs, and attempt to receive 64 of them
 * to make up our time sample.  If the sloppy clock flag is one we
 * simply total these and compute an average.  Otherwise we pick the
 * sample we think is most representative.  After the 16th and 48th
 * sample we send a T and read the time code, this providing us with
 * the seconds-and-above part of the time.
 */

/*
 * Definitions
 */
#define	MAXUNITS	4	/* maximum number of WWVB units permitted */
#ifdef GIZMO
#define	WWVB232	"/dev/tty%d"	/* %d is the unit number for all of these */
#define	WWVBPPS	"/dev/pirq%d"
#else
#define	WWVB232	"/dev/wwvb%d"
#define	WWVBPPS	"/dev/wwvbpps%d"
#endif

/*
 * When WWVB is operating optimally we want the primary clock distance
 * to come out at 100 ms.  Thus, peer.distance in the WWVB peer structure
 * is set to 90 ms and we compute delays which are at least 10 ms long.
 * The following are 90 ms and 10 ms expressed in u_fp format
 */
#define	WWVBDISTANCE	0x0000170a
#define	WWVBBASEDELAY	0x0000028f

/*
 * Other constant stuff
 */
#define	WWVBPRECISION	(-10)		/* what the heck */
#define	WWVBREFID	"WWVB"
#define	WWVBHSREFID	0x7f7f040a	/* 127.127.4.10 refid for hi strata */

/*
 * Description of clock
 */
#define	WWVBDESCRIPTION		"Spectracom 8170 WWVB Receiver"

/*
 * Speeds we run the clock ports at.
 */
#define	SPEED232	B9600	/* 9600 baud */
#define	SPEEDPPS	EXTA	/* should be 19200 baud */

/*
 * Constants for the serial port correction.  This assumes the
 * RS232 code is received at 9600 bps and the PPS code at 19200.
 * Since your average uart interrupts sort of at the center of the
 * first stop bit, these are 9.5 bit times, respectively, as l_fp
 * fractions.
 */
#define	UARTDELAY232	0x0040da74	/* about 0.990 ms */
#define	UARTDELAYPPS	0x00206d3a	/* about 0.495 ms */

/*
 * The number of PPS raw samples which we acquire to derive
 * a single estimate.
 */
#define	NPPS	64

/*
 * Samples after which we ask for a time code on the RS232 port, and
 * the sample numbers after which we start to worry that it won't
 * arrive.
 */
#define	NCODES		2
#define	FIRSTCODE	16
#define	FIRSTWORRY	19
#define	SECONDCODE	48
#define	SECONDWORRY	51


/*
 * Length of the serial time code, in characters.  The second length
 * is less the carriage returns and line feeds.
 */
#define	LENWWVBCODE	26
#define	LENWWVBPRT	22

/*
 * Code state.
 */
#define	WWVBCS_NOCODE	0	/* not expecting anything */
#define	WWVBCS_SENTT	1	/* have sent a T, received nothing */
#define	WWVBCS_GOTCR	2	/* got the first <cr> in the time code */
#define	WWVBCS_GOTSOME	3	/* have an incomplete time code buffered */

/*
 * Default fudge factor and character to receive
 */
#define	DEFFUDGETIME	0	/* we have no particular preferences */
#define	DEFMAGICCHAR	0xf0	/* corresponds to a pulse of 0.260 ms */

/*
 * Leap hold time.  After a leap second the clock will no longer be
 * reliable until it resynchronizes.  Hope 40 minutes is enough.
 */
#define	WWVBLEAPHOLD	(40 * 60)

/*
 * Hack to avoid excercising the multiplier.  I have no pride.
 */
#define	MULBY10(x)	(((x)<<3) + ((x)<<1))

/*
 * WWVB unit control structure.
 */
struct wwvbunit {
	struct peer *peer;		/* associated peer structure */
	struct refclockio io;		/* given to the I/O handler */
	struct refclockio ppsio;	/* ibid. */
	u_long samples[NPPS];		/* the PPS time samples */
	l_fp lastsampletime;		/* time of last estimate */
	l_fp codeoffsets[NCODES];	/* the time of arrival of 232 codes */
	l_fp codetime;			/* temporary for arrival times */
	char lastcode[LENWWVBCODE+6];	/* last time code we received */
	u_long lasttime;		/* last time clock heard from */
	u_char lencode;			/* length of code in buffer */
	u_char nsamples;		/* number of samples we've collected */
	u_char codestate;		/* state of 232 code reception */
	u_char unit;			/* unit number for this guy */
	u_char unsynccount;		/* count down of unsynchronized data */
	u_char status;			/* clock status */
	u_char lastevent;		/* last clock event */
	u_char reason;			/* reason for last abort */
	u_char hour;			/* hour of day */
	u_char minute;			/* minute of hour */
	u_char second;			/* seconds of minute */
	u_char tz;			/* timezone from clock */
	u_short day;			/* day of year from last code */
	u_long yearstart;		/* start of current year */
	u_long leaphold;		/* time of leap hold expiry */
	u_long polls;			/* number of RS232 polls */
	u_long noresponse;		/* number of nonresponses */
	u_long badformat;		/* number of bad format responses */
	u_long baddata;			/* number of invalid time codes */
	u_long bigcodeoff;		/* # of rejects from big code offset */
	u_long toosloppy;		/* too few remain after slop removal */
	u_long timestarted;		/* time we started this */
};

#define	WWVBUNSYNCCNT	3		/* need to see 3 synchronized samples */

/*
 * We demand that consecutive PPS samples are more than 0.875 seconds
 * and less than 1.125 seconds apart.
 */
#define	PPSLODIFF_UI	0		/* 0.875 as an l_fp */
#define	PPSLODIFF_UF	0xe0000000

#define	PPSHIDIFF_UI	1		/* 1.125 as an l_fp */
#define	PPSHIDIFF_UF	0x20000000

/*
 * We also require that the code offsets are no more that 0.125 seconds
 * apart, and that none of the PPS sample offsets are more than
 * 0.125 seconds from our averaged code offset.
 */
#define	CODEDIFF	0x20000000	/* 0.125 seconds as an l_fp fraction */
#define	PPSMARGINLO	0x20000000	/* 0.125 seconds as an l_fp fraction */
#define	PPSMARGINHI	0xe0000000	/* 0.875 seconds as an l_fp fraction */

/*
 * Some other l_fp fraction values used by the code
 */
#define	ZEROPT75	0xc0000000
#define	ZEROPT5		0x80000000
#define	ZEROPT25	0x40000000


/*
 * Limits on things.  If we have less than SAMPLELIMIT samples after
 * gross filtering, reject the whole thing.  Otherwise reduce the number
 * of samples to SAMPLEREDUCE by median elimination.  If we're running
 * with an accurate clock, chose the BESTSAMPLE as the estimated offset,
 * otherwise average the remainder.
 */
#define	SAMPLELIMIT	56
#define	SAMPLEREDUCE	32
#define	REDUCESHIFT	5	/* SAMPLEREDUCE root 2 */
#define	BESTSAMPLE	24	/* Towards the high end of half */

/*
 * reason codes
 */
#define	PPSREASON	20
#define	CODEREASON	40
#define	PROCREASON	60


/*
 * Data space for the unit structures.  Note that we allocate these on
 * the fly, but never give them back.
 */
static struct wwvbunit *wwvbunits[MAXUNITS];
static u_char unitinuse[MAXUNITS];

/*
 * Keep the fudge factors separately so they can be set even
 * when no clock is configured.
 */
static l_fp fudgefactor[MAXUNITS];
static u_char stratumtouse[MAXUNITS];
static u_char magicchar[MAXUNITS];
static u_char sloppyclockflag[MAXUNITS];

/*
 * Imported from the timer module
 */
extern u_long current_time;
extern struct event timerqueue[];

/*
 * wwvb_reset - reset the count back to zero
 */
#define	wwvb_reset(wwvb) \
	do { \
		(wwvb)->nsamples = 0; \
		(wwvb)->codestate = WWVBCS_NOCODE; \
	} while (0)

/*
 * wwvb_event - record and report an event
 */
#define	wwvb_event(wwvb, evcode) \
	do { \
		if ((wwvb)->status != (u_char)(evcode)) \
			wwvb_report_event((wwvb), (evcode)); \
	} while (0)


/*
 * Time conversion tables imported from the library
 */
extern u_long ustotslo[];
extern u_long ustotsmid[];
extern u_long ustotshi[];


/*
 * wwvb_init - initialize internal wwvb driver data
 */
void
wwvb_init()
{
	register int i;
	/*
	 * Just zero the data arrays
	 */
	bzero((char *)wwvbunits, sizeof wwvbunits);
	bzero((char *)unitinuse, sizeof unitinuse);

	/*
	 * Initialize fudge factors to default.
	 */
	for (i = 0; i < MAXUNITS; i++) {
		fudgefactor[i].l_ui = 0;
		fudgefactor[i].l_uf = DEFFUDGETIME;
		magicchar[i] = DEFMAGICCHAR;
		stratumtouse[i] = 0;
		sloppyclockflag[i] = 0;
	}
}


/*
 * wwvb_start - open the WWVB devices and initialize data for processing
 */
int
wwvb_start(unit, peer)
	u_int unit;
	struct peer *peer;
{
	register struct wwvbunit *wwvb;
	register int i;
	int fd232;
	int fdpps;
	int ldisc;
	char wwvbdev[20];
	char wwvbppsdev[20];
	struct sgttyb ttyb;
	void wwvb_receive();
	void wwvb_pps_receive();
	int wwvb_setmagic();
	extern int io_addclock();
	extern void io_closeclock();
	extern char *emalloc();

	if (unit >= MAXUNITS) {
		syslog(LOG_ERR, "wwvb clock: unit number %d invalid (max 3)",
		    unit);
		return 0;
	}
	if (unitinuse[unit]) {
		syslog(LOG_ERR, "wwvb clock: unit number %d in use", unit);
		return 0;
	}

	/*
	 * Unit okay, attempt to open the devices.  We do them both at
	 * once to make sure we can
	 */
	(void) sprintf(wwvbdev, WWVB232, unit);
	(void) sprintf(wwvbppsdev, WWVBPPS, unit);

	fd232 = open(wwvbdev, O_RDWR, 0777);
	if (fd232 == -1) {
		syslog(LOG_ERR, "wwvb clock: open of %s failed: %m", wwvbdev);
		return 0;
	}
	fdpps = open(wwvbppsdev, O_RDONLY, 0777);
	if (fdpps == -1) {
		syslog(LOG_ERR, "wwvb clock: open of %s failed: %m",
		    wwvbppsdev);
		(void) close(fd232);
		return 0;
	}

	/*
	 * Set for exclusive use
	 */
	if (ioctl(fd232, TIOCEXCL, (char *)0) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCEXCL): %m", wwvbdev);
		goto screwed;
	}
#ifndef GIZMO
	if (ioctl(fdpps, TIOCEXCL, (char *)0) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCEXCL): %m",
		    wwvbppsdev);
		goto screwed;
	}

	/*
	 * Set in the magic character on the pps port
	 */
	if (!wwvb_setmagic(magicchar[unit], fdpps, wwvbppsdev))
		goto screwed;
#endif

	/*
	 * If we have the clock discipline, set the port to raw.  Otherwise
	 * we run cooked.
	 */
	ttyb.sg_ispeed = ttyb.sg_ospeed = SPEED232;
#ifdef CLKLDISC
	ttyb.sg_erase = ttyb.sg_kill = '\r';
	ttyb.sg_flags = EVENP|ODDP|RAW|CRMOD;
	if (ioctl(fd232, TIOCSETP, (char *)&ttyb) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCSETP): %m", wwvbdev);
		goto screwed;
	}
#else
	ttyb.sg_erase = ttyb.sg_kill = '\0377';
	ttyb.sg_flags = EVENP|ODDP|CRMOD;
	if (ioctl(fd232, TIOCSETP, (char *)&ttyb) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCSETP): %m", wwvbdev);
		goto screwed;
	}
#endif

	/*
	 * Looks like this might succeed.  Find memory for the structure.
	 * Look to see if there are any unused ones, if not we malloc()
	 * one.
	 */
	if (wwvbunits[unit] != 0) {
		wwvb = wwvbunits[unit];	/* The one we want is okay */
	} else {
		for (i = 0; i < MAXUNITS; i++) {
			if (!unitinuse[i] && wwvbunits[i] != 0)
				break;
		}
		if (i < MAXUNITS) {
			/*
			 * Reclaim this one
			 */
			wwvb = wwvbunits[i];
			wwvbunits[i] = 0;
		} else {
			wwvb = (struct wwvbunit *)
			    emalloc(sizeof(struct wwvbunit));
		}
	}
	bzero((char *)wwvb, sizeof(struct wwvbunit));
	wwvbunits[unit] = wwvb;

	/*
	 * Set up the structures
	 */
	wwvb->peer = peer;
	wwvb->unit = (u_char)unit;
	wwvb->timestarted = current_time;

	wwvb->io.clock_recv = wwvb_receive;
	wwvb->io.srcclock = (caddr_t)wwvb;
	wwvb->io.datalen = 0;
	wwvb->io.fd = fd232;

	wwvb->ppsio.clock_recv = wwvb_pps_receive;
	wwvb->ppsio.srcclock = (caddr_t)wwvb;
	wwvb->ppsio.datalen = 0;
	wwvb->ppsio.fd = fdpps;

	/*
	 * Okay.  Set the line discipline to the clock line discipline, then
	 * give it to the I/O code to start receiving stuff.  Note the
	 * change of line discipline will clear the read buffers, which
	 * makes the change clean if done quickly.
	 */
#ifdef CLKLDISC
	ldisc = CLKLDISC;
	if (ioctl(fd232, TIOCSETD, (char *)&ldisc) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCSETD): %m", wwvbdev);
		goto screwed;
	}
#else
	ldisc = 0;	/* just flush the buffers */
	if (ioctl(fd232, TIOCFLUSH, (char *)&ldisc) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCFLUSH): %m",
		    wwvbdev);
		goto screwed;
	}
#endif
	if (!io_addclock(&wwvb->io)) {
		/*
		 * Oh shit.  Just close and return.
		 */
		goto screwed;
	}

#ifdef CLKLDISC
	ldisc = CLKLDISC;
	if (ioctl(fdpps, TIOCSETD, (char *)&ldisc) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCSETD): %m",
		    wwvbppsdev);
		goto screwed;
	}
#else
	ldisc = 0;	/* just flush the buffers */
	if (ioctl(fdpps, TIOCFLUSH, (char *)&ldisc) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCFLUSH): %m",
		    wwvbppsdev);
		goto screwed;
	}
#endif
	if (!io_addclock(&wwvb->ppsio)) {
		/*
		 * Remove the other one and return
		 */
		io_closeclock(&wwvb->io);
		(void) close(fdpps);
		return 0;
	}

	/*
	 * All done.  Initialize a few random peer variables, then
	 * return success.
	 */
	peer->distance = WWVBDISTANCE;
	peer->precision = WWVBPRECISION;
	peer->stratum = stratumtouse[unit];
	if (stratumtouse[unit] <= 1)
		bcopy(WWVBREFID, (char *)&peer->refid, 4);
	else
		peer->refid = htonl(WWVBHSREFID);
	unitinuse[unit] = 1;
	return 1;

screwed:
	(void) close(fd232);
	(void) close(fdpps);
	return 0;
}


/*
 * wwvb_setmagic - set the magic characters into the line discipline
 */
int
wwvb_setmagic(magic, fd, device)
	int magic;
	int fd;
	char *device;
{
	struct sgttyb ttyb;

	ttyb.sg_ispeed = ttyb.sg_ospeed = SPEEDPPS;
	ttyb.sg_flags = EVENP|ODDP|RAW;
#ifdef CLKLDISC
	ttyb.sg_erase = ttyb.sg_kill = (char)magic;
#else
	ttyb.sg_erase = ttyb.sg_kill = '\0377';
#endif
	if (ioctl(fd, TIOCSETP, (char *)&ttyb) < 0) {
		syslog(LOG_ERR, "wwvb clock: ioctl(%s, TIOCSETP): %m", device);
		return 0;
	}
	return 1;
}


/*
 * wwvb_shutdown - shut down a WWVB clock
 */
void
wwvb_shutdown(unit)
	int unit;
{
	register struct wwvbunit *wwvb;
	extern void io_closeclock();

	if (unit >= MAXUNITS) {
		syslog(LOG_ERR,
		  "wwvb clock: INTERNAL ERROR, unit number %d invalid (max 4)",
		    unit);
		return;
	}
	if (!unitinuse[unit]) {
		syslog(LOG_ERR,
		 "wwvb clock: INTERNAL ERROR, unit number %d not in use", unit);
		return;
	}

	/*
	 * Tell the I/O module to turn us off.  We're history.
	 */
	wwvb = wwvbunits[unit];
	io_closeclock(&wwvb->ppsio);
	io_closeclock(&wwvb->io);
	unitinuse[unit] = 0;
}


/*
 * wwvb_report_event - note the occurance of an event
 */
wwvb_report_event(wwvb, code)
	struct wwvbunit *wwvb;
	int code;
{
	if (wwvb->status != (u_char)code) {
		wwvb->status = (u_char)code;
		if (code != CEVNT_NOMINAL)
			wwvb->lastevent = (u_char)code;
		/*
		 * Should report event to trap handler in here.
		 * Soon...
		 */
	}
}


/*
 * wwvb_receive - receive data from the serial interface on a Spectraom clock
 */
void
wwvb_receive(rbufp)
	struct recvbuf *rbufp;
{
	register int i;
	register struct wwvbunit *wwvb;
	register u_char *dpt;
	register char *cp;
	register u_char *dpend;

	/*
	 * Get the clock this applies to and a pointer to the data
	 */
	wwvb = (struct wwvbunit *)rbufp->recv_srcclock;
	dpt = (u_char *)&rbufp->recv_space;
	dpend = dpt + rbufp->recv_length;

	/*
	 * Check out our state and process appropriately
	 */
	switch (wwvb->codestate) {
	case WWVBCS_NOCODE:
		/*
		 * Not expecting anything.  Just return.
		 */
		return;

	case WWVBCS_SENTT:
		/*
		 * The first thing we receive back from the
		 * radio will be the on-time second synchronized
		 * <cr>.  I'm not sure whether some of the following
		 * ASCII could get included or not, so we'll assume
		 * it might.  We do this a little differently
		 * depending on whether we're running in cooked
		 * (sloppy) mode or with the line discipline.
		 */
#ifdef CLKLDISC
		while (dpt < dpend && *dpt != '\r')
			dpt++;
		if (dpt == dpend) {
			/*
			 * Garbage.  Just return for now.
			 */
			return;
		}
		dpt++;		/* past the \r */
		if (dpend - dpt < 8) {
			syslog(LOG_ERR,
    "wwvb clock: CLKLDISC returns `\\r' with too few trailing characters (%d)",
			    (dpend - dpt));
			wwvb->reason = CODEREASON + 1;
			wwvb_event(wwvb, CEVNT_FAULT);
			wwvb_reset(wwvb);
			return;
		}
		if (!buftvtots(dpt, &wwvb->codetime)) {
			syslog(LOG_ERR,
			  "wwvb clock: CLKLDISC returned a screwy time value");
			wwvb->reason = CODEREASON + 2;
			wwvb_event(wwvb, CEVNT_FAULT);
			wwvb_reset(wwvb);
			return;
		}
		dpt += 8;
#else
		/*
		 * Here we space past anything up to the first nl.
		 * Anything past this is considered to possibly be
		 * part of the time code itself.  The timestamp is
		 * that recorded by the I/O module when the data was
		 * input.
		 */
		while (dpt < dpend && (*dpt & 0x7f) != '\n')
			dpt++;
		if (dpt == dpend) {
			/*
			 * What the heck?  Forget about this one.
			 */
			syslog(LOG_ERR,
		    "wwvb clock: data returned without a trailing newline");
			wwvb->reason = CODEREASON + 3;
			wwvb_event(wwvb, CEVNT_FAULT);
			wwvb_reset(wwvb);
		}
		dpt++;

		wwvb->codetime = rbufp->recv_time;
#endif
		/*
		 * Here we have our timestamp.  Change the state to
		 * reflect this.
		 */
		wwvb->codestate = WWVBCS_GOTCR;
		if (dpt == dpend)
			return;
		
		/*FALLSTHROUGH*/
	case WWVBCS_GOTCR:
		/*
		 * Here we have the timestamp, but have yet to
		 * receive any of the time code.  Delete leading
		 * \n's.
		 */
		while (dpt < dpend && (*dpt & 0x7f) == '\n')
			dpt++;
		if (dpt == dpend)
			return;		/* no state transition */
		if (!isprint(*dpt & 0x7f)) {
			wwvb->badformat++;
			wwvb->reason = CODEREASON + 4;
			wwvb_event(wwvb, CEVNT_BADREPLY);
			wwvb_reset(wwvb);
			return;
		}

		wwvb->codestate = WWVBCS_GOTSOME;
		wwvb->lencode = 0;

		/*FALLSTHROUGH*/
	case WWVBCS_GOTSOME:
		cp = &(wwvb->lastcode[wwvb->lencode]);
		while (dpt < dpend && isprint(*dpt & 0x7f)) {
			*cp++ = (char)(*dpt++ & 0x7f);
			wwvb->lencode++;
			if (wwvb->lencode > LENWWVBCODE) {
				wwvb->badformat++;
				wwvb->reason = CODEREASON + 5;
				wwvb_event(wwvb, CEVNT_BADREPLY);
				wwvb_reset(wwvb);
				return;
			}
		}
		if (dpt == dpend) {
			/*
			 * Incomplete.  Wait for more.
			 */
			return;
		}
#ifdef CLKLDISC
		if (*dpt != '\r') {
#else
		if ((*dpt & 0x7f) != '\n') {
#endif
			wwvb->badformat++;
			wwvb->reason = CODEREASON + 6;
			wwvb_event(wwvb, CEVNT_BADREPLY);
			wwvb_reset(wwvb);
			return;
		}

		/*
		 * Finally, got a complete buffer.  Mainline code will
		 * continue on.
		 */
		break;
	
	default:
		syslog(LOG_ERR, "wwvb clock: INTERNAL ERROR: state %d",
		    wwvb->codestate);
		wwvb->reason = CODEREASON + 7;
		wwvb_event(wwvb, CEVNT_FAULT);
		wwvb_reset(wwvb);
		return;
	}

	/*
	 * Boy!  After all that crap, the lastcode buffer now contains
	 * something we hope will be a valid time code.  Do length
	 * checks and sanity checks on constant data.
	 */
	wwvb->codestate = WWVBCS_NOCODE;
	if (wwvb->lencode != LENWWVBPRT) {
		wwvb->badformat++;
		wwvb->reason = CODEREASON + 8;
		wwvb_event(wwvb, CEVNT_BADREPLY);
		wwvb_reset(wwvb);
		return;
	}

	cp = wwvb->lastcode;
	if (cp[ 1] != ' ' || cp[ 2] != ' ' ||
	    cp[ 6] != ' ' || cp[ 9] != ':' ||
	    cp[12] != ':' || cp[15] != ' ' ||
	    cp[16] != ' ' || cp[17] != 'T' ||
	    cp[18] != 'Z' || cp[19] != '=') {
		wwvb->badformat++;
		wwvb->reason = CODEREASON + 9;
		wwvb_event(wwvb, CEVNT_BADREPLY);
		wwvb_reset(wwvb);
		return;
	}

	/*
	 * Check sync character and numeric data
	 */
	if ((*cp != ' ' && *cp != '*' && *cp != '?') ||
	    !isdigit(cp[ 3]) || !isdigit(cp[ 4]) ||
	    !isdigit(cp[ 5]) || !isdigit(cp[ 7]) ||
	    !isdigit(cp[ 8]) || !isdigit(cp[10]) ||
	    !isdigit(cp[11]) || !isdigit(cp[13]) ||
	    !isdigit(cp[14]) || !isdigit(cp[20]) ||
	    !isdigit(cp[21])) {
		wwvb->badformat++;
		wwvb->reason = CODEREASON + 10;
		wwvb_event(wwvb, CEVNT_BADREPLY);
		wwvb_reset(wwvb);
		return;
	}

	/*
	 * Do some finer range checks on the raw data
	 */
	if (cp[ 3] > '3' || cp[ 7] > '2' ||
	    cp[10] > '5' || cp[13] > '5') {
		wwvb->badformat++;
		wwvb->reason = CODEREASON + 11;
		wwvb_event(wwvb, CEVNT_BADREPLY);
		wwvb_reset(wwvb);
		return;
	}


	/*
	 * So far, so good.  Compute days, hours, minutes, seconds,
	 * time zone.  Do range checks on these.
	 */
	wwvb->day = cp[3] - '0';
	wwvb->day = MULBY10(wwvb->day) + cp[4] - '0';
	wwvb->day = MULBY10(wwvb->day) + cp[5] - '0';

	wwvb->hour = MULBY10(cp[7] - '0') + cp[8] - '0';
	wwvb->minute = MULBY10(cp[10] - '0') + cp[11] -  '0';
	wwvb->second = MULBY10(cp[13] - '0') + cp[14] - '0';
	wwvb->tz = MULBY10(cp[20] - '0') + cp[21] - '0';

	if (wwvb->day > 366 || wwvb->tz > 12) {
		wwvb->baddata++;
		wwvb->reason = CODEREASON + 12;
		wwvb_event(wwvb, CEVNT_BADDATE);
		wwvb_reset(wwvb);
		return;
	}

	if (wwvb->hour > 23 || wwvb->minute > 59 || wwvb->second > 59) {
		wwvb->baddata++;
		wwvb->reason = CODEREASON + 13;
		wwvb_event(wwvb, CEVNT_BADDATE);
		wwvb_reset(wwvb);
		return;
	}

	/*
	 * Figure out whether we're computing the early time code or
	 * the late one.
	 */
	if (wwvb->nsamples < SECONDCODE)
		i = 0;
	else
		i = 1;

	/*
	 * Now, compute the reference time value.
	 */
	if (!clocktime(wwvb->day, wwvb->hour, wwvb->minute, wwvb->second,
	    wwvb->tz, rbufp->recv_time.l_ui, &wwvb->yearstart,
	    &wwvb->codeoffsets[i].l_ui)) {
		wwvb->baddata++;
		wwvb->reason = CODEREASON + 14;
		wwvb_event(wwvb, CEVNT_BADDATE);
		wwvb_reset(wwvb);
		return;
	}

	/*
	 * Check for synchronization
	 */
	if (*cp == ' ') {
		if (wwvb->unsynccount > 0)
			wwvb->unsynccount--;
	} else {
		wwvb->unsynccount = WWVBUNSYNCCNT;
	}


	/*
	 * Compute the offset.  For the fractional part of the
	 * offset we use the expected delay for a character at the
	 * baud rate.
	 */
	wwvb->codeoffsets[i].l_uf = UARTDELAY232;
	L_SUB(&wwvb->codeoffsets[i], &wwvb->codetime);

	/*
	 * Done!
	 */
}


/*
 * wwvb_pps_receive - receive pps data
 */
void
wwvb_pps_receive(rbufp)
	struct recvbuf *rbufp;
{
	register struct wwvbunit *wwvb;
	register u_char *dpt;
	register u_long tmp_ui;
	register u_long tmp_uf;
	l_fp ts;
	void wwvb_process();

	/*
	 * Get the clock this applies to and a pointer to the data
	 */
	wwvb = (struct wwvbunit *)rbufp->recv_srcclock;
	dpt = (u_char *)&rbufp->recv_space;

	/*
	 * Record the time of this event
	 */
	wwvb->lasttime = current_time;

	/*
	 * Check the length of the buffer.  Start over if it isn't as
	 * expected.
	 */
#ifdef GIZMO
	if (rbufp->recv_length != 8) {
#else
#ifdef CLKLDISC
	if (rbufp->recv_length != (1+8) || *dpt++ != magicchar[wwvb->unit]) {
#else
	if (rbufp->recv_length != 1 || *dpt++ != magicchar[wwvb->unit]) {
#endif	/* CLKLDISC */
#endif  /* GIZMO */
		wwvb->reason = PPSREASON + 1;
		wwvb_event(wwvb, CEVNT_FAULT);
		wwvb_reset(wwvb);
		return;
	}

#if defined(CLKLDISC) || defined(GIZMO)
	if (!buftvtots(dpt, &ts)) {
		wwvb->reason = PPSREASON + 2;
		wwvb_event(wwvb, CEVNT_FAULT);
		wwvb_reset(wwvb);
		return;
	}
	tmp_ui = ts.l_ui;
	tmp_uf = ts.l_uf;
#else
	tmp_ui = rbufp->recv_time.l_ui;
	tmp_uf = rbufp->recv_time.l_uf;
#endif

#ifndef GIZMO
	/*
	 * Got a timestamp.  Subtract the character delay offset
	 * from it.
	 */
	M_SUBUF(tmp_ui, tmp_uf, UARTDELAYPPS);
#endif

	/*
	 * Make sure this character arrived approximately 1 second
	 * after the previous one.  If not, start over.
	 */
	L_NEG(&wwvb->lastsampletime);
	M_ADD(wwvb->lastsampletime.l_ui, wwvb->lastsampletime.l_uf,
	    tmp_ui, tmp_uf);
	if ( wwvb->lastsampletime.l_ui > PPSHIDIFF_UI ||
	    (wwvb->lastsampletime.l_ui == PPSHIDIFF_UI &&
	     wwvb->lastsampletime.l_uf > PPSHIDIFF_UF) ||
	    (wwvb->lastsampletime.l_ui == PPSLODIFF_UI &&
	     wwvb->lastsampletime.l_uf < PPSLODIFF_UF)) {
		wwvb->reason = PPSREASON + 3;
		wwvb_event(wwvb, CEVNT_TIMEOUT);
		wwvb_reset(wwvb);
	}

	/*
	 * Save the current time.
	 */
	wwvb->lastsampletime.l_ui = tmp_ui;
	wwvb->lastsampletime.l_uf = tmp_uf;

	/*
	 * Also save the subsecond part of the time in the samples
	 * array, to be diddled with later.  Save the negative of
	 * this, since this is the offset.
	 */
	wwvb->samples[wwvb->nsamples++] = -tmp_uf;

	/*
	 * We time the acquisition of RS232 time code samples, and
	 * the detection of a failure, against the sample number.
	 * Also, if we've hit NPPS samples we process the lot.
	 */
	switch (wwvb->nsamples) {
	case FIRSTCODE:
	case SECONDCODE:
		/*
		 * Time to request a time code.  The state of the
		 * code receiver should be NOCODE.
		 */
		if (wwvb->codestate != WWVBCS_NOCODE) {
			syslog(LOG_ERR,
			    "wwvb clock: code state %d in wwvb_pps_receive()",
			    wwvb->codestate);
			wwvb->reason = PPSREASON + 4;
			wwvb_event(wwvb, CEVNT_FAULT);
			wwvb_reset(wwvb);
		} else if (write(wwvb->io.fd, "T", 1) != 1) {
			syslog(LOG_ERR,
			    "wwvb clock: write fails to unit %d: %m",
			    wwvb->unit);
			wwvb->reason = PPSREASON + 5;
			wwvb_event(wwvb, CEVNT_FAULT);
			wwvb_reset(wwvb);
		} else {
			wwvb->codestate = WWVBCS_SENTT;
			wwvb->polls++;
		}
		break;

	case FIRSTWORRY:
	case SECONDWORRY:
		/*
		 * By now the code should have been entirely received
		 * and the state set back to NOCODE.  If this is not
		 * not the case, count it as a non-response and start
		 * over.
		 */
		if (wwvb->codestate != WWVBCS_NOCODE) {
			wwvb->noresponse++;
			wwvb->reason = PPSREASON + 6;
			wwvb_event(wwvb, CEVNT_TIMEOUT);
			wwvb_reset(wwvb);
		}
		break;
	
	case NPPS:
		/*
		 * Here we've managed to complete an entire 64 second
		 * cycle without major mishap.  Process what has been
		 * received.
		 */
		wwvb_process(wwvb);
		break;
	}
}



/*
 * wwvb_process - process a pile of samples from the clock
 */
void
wwvb_process(wwvb)
	struct wwvbunit *wwvb;
{
	register int i, j;
	register u_long tmp_ui, tmp_uf;
	register int noff;
	l_fp offset;
	u_fp delay;
	u_long off[NPPS];
	int addpt5;
	int isinsync;
	extern void refclock_receive();

	/*
	 * Reset things to zero so we don't have to worry later
	 */
	wwvb_reset(wwvb);

	/*
	 * First deal with the two offsets received via the
	 * RS232 code.  If they aren't within 100 ms of each
	 * other, all bets are off.
	 */
	tmp_ui = wwvb->codeoffsets[0].l_ui;
	tmp_uf = wwvb->codeoffsets[0].l_uf;
	M_SUB(tmp_ui, tmp_uf, wwvb->codeoffsets[1].l_ui,
	    wwvb->codeoffsets[1].l_uf);
	i = 1;
	if (M_ISNEG(tmp_ui, tmp_uf)) {
		i = 0;
		M_NEG(tmp_ui, tmp_uf);
	}
	if (tmp_ui != 0 || tmp_uf > CODEDIFF) {
		wwvb->bigcodeoff++;
		wwvb->reason = PROCREASON + 1;
		wwvb_event(wwvb, CEVNT_BADREPLY);
		return;
	}

	/*
	 * When you weren't looking, the above code also computed the
	 * minimum of the two (in i), which is what we use from here on.
	 * Note that we only saved the subsecond part of the PPS
	 * data, which will be combined with the seconds part of
	 * the code offsets.  To avoid ambiguity when the subsecond
	 * part of the offset is close to zero, we add 0.5 to everything
	 * when the code offset subseconds are > 0.75 or < 0.25, and
	 * subtract it off at the end.
	 */
	offset.l_ui = wwvb->codeoffsets[i].l_ui;
	tmp_uf = wwvb->codeoffsets[i].l_uf;
	addpt5 = 0;
	if (tmp_uf > ZEROPT75 || tmp_uf < ZEROPT25) {
		addpt5 = 1;
		M_ADDUF(offset.l_ui, tmp_uf, ZEROPT5);
	}

	/*
	 * Now run through the PPS samples, adding 0.5 if necessary,
	 * and rejecting those that differ from the code time stamp
	 * by more than 0.125.  The rest get stuffed into the offset
	 * array, sorted in ascending order.  The sort is really gross,
	 * I should do this better.
	 */
	noff = 0;
	for (i = 0; i < NPPS; i++) {
		tmp_ui = wwvb->samples[i];
		if (addpt5)
			tmp_ui += ZEROPT5;
		if ((tmp_ui - tmp_uf) > PPSMARGINLO &&
		    (tmp_ui - tmp_uf) < PPSMARGINHI) {
			/*
			 * belch!  This one no good.
			 */
			continue;
		}

		if (noff == 0 || off[noff-1] < tmp_ui) {
			/*
			 * Easy, goes in at the end.
			 */
			off[noff++] = tmp_ui;
		} else {
			/*
			 * Start at the end, shifting everything up
			 * until we find the proper slot.
			 */
			j = noff;
			do {
				j--;
				off[j+1] = off[j];
			} while (j > 0 && off[j-1] > tmp_ui);
			off[j] = tmp_ui;
			noff++;
		}
	}

	/*
	 * If we have less than the prescribed limit, forget this.
	 */
	if (noff < SAMPLELIMIT) {
		wwvb->toosloppy++;
		wwvb->reason = PROCREASON + 2;
		wwvb_event(wwvb, CEVNT_BADDATE);
		return;
	}

	/*
	 * Now reject the end of the list furthest from the median
	 * until we get to SAMPLEREDUCE samples remaining.
	 */
	noff = NPPS;
	i = 0;
	while ((noff - i) > SAMPLEREDUCE) {
		if ((off[noff-1] - off[(noff + i)/2]) >
		    (off[(noff + i)/2] - off[i])) {
			noff--;
		} else {
			i++;
		}
	}

	/*
	 * What we do next depends on the setting of the
	 * sloppy clock flag.  If it is on, average the remainder
	 * to derive our estimate.  Otherwise, just pick a
	 * representative value from the remaining stuff.
	 */
	if (sloppyclockflag[wwvb->unit]) {
		tmp_ui = tmp_uf = 0;
		for (j = i; j < noff; j++)
			M_ADDUF(tmp_ui, tmp_uf, off[j]);
		for (j = REDUCESHIFT; j > 0; j--)
			M_RSHIFTU(tmp_ui, tmp_uf);
	} else {
		tmp_uf = off[i+BESTSAMPLE];
	}
	
	/*
	 * The subsecond part of the offset is now in tmp_uf.
	 * Subtract the 0.5 seconds if we added it, add in
	 * the fudge factor, then store this away.
	 */
	if (addpt5)
		M_SUBUF(offset.l_ui, tmp_uf, ZEROPT5);
	M_ADD(offset.l_ui, tmp_uf, fudgefactor[wwvb->unit].l_ui,
	    fudgefactor[wwvb->unit].l_uf);
	offset.l_uf = tmp_uf;

	/*
	 * Compute the delay as the difference between the
	 * lowest and highest offsets that remain in the
	 * consideration list.
	 */
	delay = MFPTOFP(0, off[noff-1] - off[i]) + WWVBBASEDELAY;

	/*
	 * Determine synchronization status.  Can be unsync'd either
	 * by a report from the clock or by a leap hold.
	 */
	if (wwvb->unsynccount == 0 || wwvb->leaphold > current_time)
		isinsync = 0;
	else
		isinsync = 1;

	/*
	 * Done.  Use codetime as the reference time and lastsampletime
	 * as the receive time.
	 */
	refclock_receive(wwvb->peer, &off, delay, &wwvb->codetime,
	    &wwvb->lastsampletime, isinsync);

	if (wwvb->unsynccount != 0)
		wwvb_event(wwvb, CEVNT_PROP);
	else
		wwvb_event(wwvb, CEVNT_NOMINAL);
}


/*
 * wwvb_poll - called by the transmit procedure
 */
void
wwvb_poll(unit, peer)
	int unit;
	char *peer;
{
	if (unit >= MAXUNITS) {
		syslog(LOG_ERR, "wwvb clock poll: INTERNAL: unit %d invalid",
		    unit);
		return;
	}
	if (!unitinuse[unit]) {
		syslog(LOG_ERR, "wwvb clock poll: INTERNAL: unit %d unused",
		    unit);
		return;
	}

	if ((current_time - wwvbunits[unit]->lasttime) > 150)
		wwvb_event(wwvbunits[unit], CEVNT_FAULT);
}

/*
 * wwvb_leap - called when a leap second occurs
 */
void
wwvb_leap()
{
	register int i;

	/*
	 * This routine should be entered a few seconds after
	 * midnight UTC when a leap second occurs.  To ensure we
	 * don't believe foolish time from the clock(s) we set a
	 * 40 minute hold on them.  It shouldn't take anywhere
	 * near this amount of time to adjust if the clock is getting
	 * data, but doing anything else is complicated.
	 */
	for (i = 0; i < MAXUNITS; i++) {
		if (unitinuse[i])
			wwvbunits[i]->leaphold = current_time + WWVBLEAPHOLD;
	}
}


/*
 * wwvb_control - set fudge factors, return statistics
 */
void
wwvb_control(unit, in, out)
	u_int unit;
	struct refclockstat *in;
	struct refclockstat *out;
{
	register struct wwvbunit *wwvb;

	if (unit >= MAXUNITS) {
		syslog(LOG_ERR, "wwvb clock: unit %d invalid (max %d)",
		    unit, MAXUNITS-1);
		return;
	}

	if (in != 0) {
		if (in->haveflags & CLK_HAVETIME1)
			fudgefactor[unit] = in->fudgetime1;
		if (in->haveflags & CLK_HAVEVAL1) {
			stratumtouse[unit] = (u_char)(in->fudgeval1 & 0xf);
			if (unitinuse[unit]) {
				struct peer *peer;

				/*
				 * Should actually reselect clock, but
				 * will wait for the next timecode
				 */
				wwvb = wwvbunits[unit];
				peer = wwvb->peer;
				peer->stratum = stratumtouse[unit];
				if (stratumtouse[unit] <= 1)
					bcopy(WWVBREFID, (char *)&peer->refid,
					    4);
				else
					peer->refid = htonl(WWVBHSREFID);
			}
		}
		if (in->haveflags & CLK_HAVEFLAG1) {
			sloppyclockflag[unit] = in->flags & CLK_FLAG1;
		}
		if (in->haveflags & CLK_HAVEFLAG2) {
			if (in->flags & CLK_FLAG2 && unitinuse[unit])
				wwvbunits[unit]->leaphold = 0;
		}
		if (in->haveflags & CLK_HAVEVAL2 && unitinuse[unit]) {
#ifdef CLKLDISC
			char buf[20];

			(void) sprintf(buf, WWVBPPS, unit);
			magicchar[unit] = (u_char)(in->fudgeval2 & 0xff);
			(void)wwvb_setmagic(magicchar[unit],
			    wwvbunits[unit]->ppsio.fd, buf);
#else
			magicchar[unit] = (u_char)(in->fudgeval2 & 0xff);
#endif
		}
	}

	if (out != 0) {
		out->type = REFCLK_WWVB_SPECTRACOM;
		out->haveflags
		    = CLK_HAVETIME1|CLK_HAVEVAL1|CLK_HAVEVAL2|CLK_HAVEFLAG1;
		out->clockdesc = WWVBDESCRIPTION;
		out->fudgetime1 = fudgefactor[unit];
		out->fudgetime2.l_ui = 0;
		out->fudgetime2.l_uf = 0;
		out->fudgeval1 = (long)stratumtouse[unit];
		out->fudgeval2 = (long)magicchar[unit];
		out->flags = sloppyclockflag[unit];
		if (unitinuse[unit]) {
			wwvb = wwvbunits[unit];
			out->lencode = wwvb->lencode;
			out->lastcode = wwvb->lastcode;
			out->timereset = current_time - wwvb->timestarted;
			out->polls = wwvb->polls;
			out->noresponse = wwvb->noresponse;
			out->badformat = wwvb->badformat;
			out->baddata = wwvb->baddata;
			out->lastevent = wwvb->lastevent;
			out->currentstatus = wwvb->status;
		} else {
			out->lencode = 0;
			out->lastcode = "";
			out->polls = out->noresponse = 0;
			out->badformat = out->baddata = 0;
			out->timereset = 0;
			out->currentstatus = out->lastevent = CEVNT_NOMINAL;
		}
	}
}


/*
 * wwvb_buginfo - return clock dependent debugging info
 */
void
wwvb_buginfo(unit, bug)
	int unit;
	register struct refclockbug *bug;
{
	register struct wwvbunit *wwvb;
	register int i;
	register int n;

	bug->nvalues = bug->ntimes = 0;

	if (unit >= MAXUNITS) {
		syslog(LOG_ERR, "wwvb clock: unit %d invalid (max %d)",
		    unit, MAXUNITS-1);
		return;
	}

	if (!unitinuse[unit])
		return;
	wwvb = wwvbunits[unit];

	bug->nvalues = 14;
	bug->svalues = 0;
	if (wwvb->lasttime != 0)
		bug->values[0] = current_time - wwvb->lasttime;
	else
		bug->values[0] = 0;
	bug->values[1] = (u_long)wwvb->reason;
	bug->values[2] = (u_long)wwvb->nsamples;
	bug->values[3] = (u_long)wwvb->codestate;
	bug->values[4] = (u_long)wwvb->unsynccount;
	bug->values[5] = (u_long)wwvb->day;
	bug->values[6] = (u_long)wwvb->hour;
	bug->values[7] = (u_long)wwvb->minute;
	bug->values[8] = (u_long)wwvb->second;
	bug->values[9] = (u_long)wwvb->tz;
	bug->values[10] = wwvb->bigcodeoff;
	bug->values[11] = wwvb->toosloppy;
	bug->values[12] = wwvb->yearstart;
	if (wwvb->leaphold > current_time)
		bug->values[13] = wwvb->leaphold - current_time;
	else
		bug->values[13] = 0;

	bug->stimes = 0xc;
	bug->times[0] = wwvb->codetime;
	bug->times[1] = wwvb->lastsampletime;
	bug->times[2] = wwvb->codeoffsets[0];
	bug->times[3] = wwvb->codeoffsets[1];
	n = wwvb->nsamples - (NCLKBUGTIMES-4);
	if (n < 0)
		n = 0;
	i = 4;
	bug->ntimes += wwvb->nsamples - n + 4;
	while (n < wwvb->nsamples) {
		bug->stimes |= (1<<i);
		if (wwvb->samples[n] & 0x80000000)
			bug->times[i].l_i = -1;
		else
			bug->times[i].l_i = 0;
		bug->times[i++].l_uf = wwvb->samples[n++];
	}
}
#endif
