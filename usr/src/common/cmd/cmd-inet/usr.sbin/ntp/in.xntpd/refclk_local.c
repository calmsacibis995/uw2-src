/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/refclk_local.c	1.2"
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
 * refclock_local - local pseudo-clock driver
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_refclock.h"

#if defined(REFCLOCK) && defined(LOCAL_CLOCK)
/*
 * This is a hack to allow a machine to use its own system clock as
 * a "reference clock", i.e. to free run against its own clock at
 * a non-infinity stratum.  This is certainly useful if you want to
 * use NTP in an isolated environment with no radio clock (not that
 * this is a good idea) to synchronize the machines together.  Pick
 * a machine that you figure has a good clock and configure it with
 * a local reference clock running at stratum 0 (i.e. 127.127.1.0).
 * Then point all the other machines at the one you're using as the
 * reference.
 *
 * The other thing this is good for is if you want to use a particular
 * server's clock as the last resort, when all radio time has gone
 * away.  This is especially good if that server has an ovenized
 * oscillator or something which will keep the time stable for extended
 * periods, since then all the other machines can benefit from this.
 * For this you would configure a local clock at a higher stratum (say
 * 3 or 4) to prevent the server's stratum from falling below here.
 */

/*
 * Definitions
 */
#define	NUMUNITS	16	/* 127.127.1.[0-15] */

/*
 * Some constant values we stick in the peer structure
 */
#define	LCLDISTANCE	(FP_SECOND*15)	/* 15 seconds */
#define	LCLDELAY	(FP_SECOND*5)	/* 5 second delay */
#define	LCLREFOFFSET	20		/* reftime is 20s behind */
#define	LCLPRECISION	(-5)		/* what the heck */
#define	LCLREFID	"LCL\0"
#define	LCLHSREFID	0x7f7f0101	/* 127.127.1.1 refid for hi stratum */

/*
 * Description of clock
 */
#define	LCLDESCRIPTION	"Free running against local system clock"

/*
 * Local clock unit control structure.
 */
struct lclunit {
	struct peer *peer;		/* associated peer structure */
	u_char status;			/* clock status */
	u_char lastevent;		/* last clock event */
	u_char unit;			/* unit number */
	u_char unused;
	u_long lastupdate;		/* last time data received */
	u_long polls;			/* number of polls */
	u_long timestarted;		/* time we started this */
};


/*
 * Data space for the unit structures.  Note that we allocate these on
 * the fly, but never give them back.
 */
static struct lclunit *lclunits[NUMUNITS];
static u_char unitinuse[NUMUNITS];

/*
 * Imported from the timer module
 */
extern u_long current_time;


/*
 * local_init - initialize internal local clock driver data
 */
void
local_init()
{
	/*
	 * Just zero the data arrays
	 */
	bzero((char *)lclunits, sizeof lclunits);
	bzero((char *)unitinuse, sizeof unitinuse);
}


/*
 * local_start - start up a local reference clock
 */
int
local_start(unit, peer)
	u_int unit;
	struct peer *peer;
{
	register int i;
	register struct lclunit *lcl;
	extern char *emalloc();

	if (unit >= NUMUNITS) {
		syslog(LOG_ERR, "local clock: unit number %d invalid (max 15)",
		    unit);
		return 0;
	}
	if (unitinuse[unit]) {
		syslog(LOG_ERR, "local clock: unit number %d in use", unit);
		return 0;
	}

	/*
	 * Looks like this might succeed.  Find memory for the structure.
	 * Look to see if there are any unused ones, if not we malloc()
	 * one.
	 */
	if (lclunits[unit] != 0) {
		lcl = lclunits[unit];	/* The one we want is okay */
	} else {
		for (i = 0; i < NUMUNITS; i++) {
			if (!unitinuse[i] && lclunits[i] != 0)
				break;
		}
		if (i < NUMUNITS) {
			/*
			 * Reclaim this one
			 */
			lcl = lclunits[i];
			lclunits[i] = 0;
		} else {
			lcl = (struct lclunit *)emalloc(sizeof(struct lclunit));
		}
	}
	bzero((char *)lcl, sizeof(struct lclunit));
	lclunits[unit] = lcl;

	/*
	 * Set up the structure
	 */
	lcl->peer = peer;
	lcl->unit = (u_char)unit;
	lcl->timestarted = lcl->lastupdate = current_time;

	/*
	 * That was easy.  Diddle the peer variables and return success.
	 */
	peer->distance = LCLDISTANCE;
	peer->precision = LCLPRECISION;
	peer->stratum = (u_char)unit;
	if (unit <= 1)
		bcopy(LCLREFID, (char *)&peer->refid, 4);
	else
		peer->refid = htonl(LCLHSREFID);
	unitinuse[unit] = 1;
	return 1;
}


/*
 * local_shutdown - shut down a local clock
 */
void
local_shutdown(unit)
	int unit;
{
	if (unit >= NUMUNITS) {
		syslog(LOG_ERR,
		"local clock: INTERNAL ERROR, unit number %d invalid (max 15)",
		    unit);
		return;
	}
	if (!unitinuse[unit]) {
		syslog(LOG_ERR,
		"local clock: INTERNAL ERROR, unit number %d not in use", unit);
		return;
	}

	unitinuse[unit] = 0;
}


/*
 * local_poll - called by the transmit procedure
 */
void
local_poll(unit, peer)
	int unit;
	struct peer *peer;
{
	l_fp off;
	l_fp ts;
	extern void get_systime();

	if (unit >= NUMUNITS) {
		syslog(LOG_ERR, "local clock poll: INTERNAL: unit %d invalid",
		    unit);
		return;
	}
	if (!unitinuse[unit]) {
		syslog(LOG_ERR, "local clock poll: INTERNAL: unit %d unused",
		    unit);
		return;
	}
	if (peer != lclunits[unit]->peer) {
		syslog(LOG_ERR,
		    "local clock poll: INTERNAL: peer incorrect for unit %d",
		    unit);
		return;
	}

	/*
	 * Update clock stat counters
	 */
	lclunits[unit]->polls++;
	lclunits[unit]->lastupdate = current_time;

	/*
	 * This is pretty easy.  Give the reference clock support
	 * a zero offset and our fixed delay.  Use peer->xmt for
	 * our receive time.  Use peer->xmt - 20 seconds for our
	 * reference time.
	 */
	off.l_ui = off.l_uf = 0;
	ts = peer->xmt;
	ts.l_ui -= LCLREFOFFSET;
	refclock_receive(peer, &off, LCLDELAY, &ts, &peer->xmt, 1);
}



/*
 * local_control - set fudge factors, return statistics
 */
void
local_control(unit, in, out)
	u_int unit;
	struct refclockstat *in;
	struct refclockstat *out;
{
	extern l_fp drift_comp;

	if (unit >= NUMUNITS) {
		syslog(LOG_ERR, "local clock: unit %d invalid (max %d)",
		    unit, NUMUNITS-1);
		return;
	}

	/*
	 * We only use the time1 fudge factor.  The drift compensation
	 * register is set to this if it is included in the incoming
	 * stuff and is shown in the outgoing stuff.
	 */
	if (in != 0) {
		if (in->haveflags & CLK_HAVETIME1)
			drift_comp = in->fudgetime1;
	}
	if (out != 0) {
		out->type = REFCLK_LOCALCLOCK;
		out->flags = 0;
		out->haveflags = CLK_HAVETIME1;
		out->clockdesc = LCLDESCRIPTION;
		out->fudgetime1 = drift_comp;
		out->fudgetime2.l_ui = out->fudgetime2.l_uf = 0;
		out->fudgeval1 = out->fudgeval2 = 0;
		out->lencode = 0;
		out->lastcode = "";
		out->badformat = 0;
		out->baddata = 0;
		out->noresponse = 0;
		if (unitinuse[unit]) {
			out->polls = lclunits[unit]->polls;
			out->timereset =
			    current_time - lclunits[unit]->timestarted;
			out->lastevent = lclunits[unit]->lastevent;
			out->currentstatus = lclunits[unit]->status;
		} else {
			out->polls = 0;
			out->timereset = 0;
			out->currentstatus = out->lastevent = CEVNT_NOMINAL;
		}
	}
}
#endif
