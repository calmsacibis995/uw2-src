/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/ntpdate/ntpdate.h	1.2"
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
 * ntpdate.h - declarations for the ntpdate program
 */

/*
 * The server structure is a much simplified version of the
 * peer structure, for ntpdate's use.  Since we always send
 * in client mode and expect to receive in server mode, this
 * leaves only a very limited number of things we need to
 * remember about the server.
 */
struct server {
	struct sockaddr_in srcadr;	/* address of remote host */
	u_char leap;			/* leap indicator */
	u_char stratum;			/* stratum of remote server */
	s_char precision;		/* server's clock precision */
	u_char trust;			/* trustability of the filtered data */
	u_fp distance;			/* distance from primary clock */
	u_fp dispersion;		/* peer clock dispersion */
	u_long refid;			/* peer reference ID */
	l_fp reftime;			/* time of peer's last update */
	u_long event_time;		/* time for next timeout */
	u_short xmtcnt;			/* number of packets transmitted */
	u_short filter_nextpt;		/* index into filter shift register */
	u_fp filter_delay[PEER_SHIFT];	/* delay part of shift register */
	l_fp filter_offset[PEER_SHIFT];	/* offset part of shift register */
	s_fp filter_soffset[PEER_SHIFT]; /* offset in s_fp format, for disp */
	l_fp org;			/* peer's originate time stamp */
	l_fp xmt;			/* transmit time stamp */
	u_fp estdelay;			/* filter estimated delay */
	u_fp estdisp;			/* filter estimated dispersion */
	l_fp estoffset;			/* filter estimated clock offset */
	s_fp estsoffset;		/* fp version of above */
};


/*
 * ntpdate runs everything on a simple, short timeout.  It sends a
 * packet and sets the timeout (by default, to a small value suitable
 * for a LAN).  If it receives a response it sends another request.
 * If it times out it shifts zeroes into the filter and sends another
 * request.
 *
 * The timer routine is run often (once every 1/5 second currently)
 * so that time outs are done with reasonable precision.
 */
#define TIMER_HZ	(5)		/* 5 per second */

/*
 * ntpdate will make a long adjustment using adjtime() if the times
 * are close, or step the time if the times are farther apart.  The
 * following defines what is "close".
 */
#define	NTPDATE_THRESHOLD	(FP_SECOND >> 1)	/* 1/2 second */

/*
 * When doing adjustments, ntpdate actually overadjusts (currently
 * by 50%, though this may change).  While this will make it take longer
 * to reach a steady state condition, it will typically result in
 * the clock keeping more accurate time, on average.  The amount of
 * overshoot is limited.
 */
/* #define	ADJ_OVERSHOOT	1/2	/* this is hard coded */
#define	ADJ_MAXOVERSHOOT	0x10000000	/* 50 ms as a ts fraction */

/*
 * Since ntpdate isn't aware of some of the things that normally get
 * put in an NTP packet, we fix some values.
 */
#define	NTPDATE_PRECISION	(-6)		/* use this precision */
#define	NTPDATE_DISTANCE	FP_SECOND	/* distance is 1 sec */
#define	NTPDATE_DISP		FP_SECOND	/* so is the dispersion */
#define	NTPDATE_REFID		(0)		/* reference ID to use */


/*
 * Some defaults
 */
#define	DEFTIMEOUT	5		/* 5 timer increments */
#define	DEFSAMPLES	4		/* get 4 samples per server */
#define	DEFPRECISION	(-5)		/* the precision we claim */
