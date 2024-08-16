/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/refclk_conf.c	1.2"
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
 * refclock_conf.c - reference clock configuration
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_refclock.h"

#ifdef REFCLOCK

#ifdef LOCAL_CLOCK
extern int local_start();
extern void local_shutdown(), local_poll(), local_control();
extern void local_init();
#endif

#ifdef PST
extern int pst_start();
extern void pst_shutdown(), pst_leap(), pst_control();
extern void pst_init(), pst_buginfo();
#endif

#ifdef CHU
extern int chu_start();
extern void chu_shutdown(), chu_poll(), chu_control();
extern void chu_init();
#endif

#ifdef WWVB
extern int wwvb_start();
extern void wwvb_shutdown(), wwvb_poll(), wwvb_leap(), wwvb_control();
extern void wwvb_init(), wwvb_buginfo();
#endif


/*
 * Order is clock_start(), clock_shutdown(), clock_poll(), clock_leap(),
 * clock_control(), clock_init(), clock_xmitinterval, clock_flags;
 *
 * Types are defined in ntp.h.  The index must match this.
 */
struct refclock refclock_conf[] = {
	{ noentry, noentry, noentry, noentry,	/* 0 REFCLOCK_NONE */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },

#ifdef LOCAL_CLOCK
	{ local_start, local_shutdown, local_poll, noentry,
	  local_control, local_init, noentry, STDPOLL, NOFLAGS },
#else
	{ noentry, noentry, noentry, noentry,	/* 1 REFCLOCK_LOCALCLOCK */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },
#endif

	{ noentry, noentry, noentry, noentry,	/* 2 REFCLOCK_WWV_HEATH */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },

#ifdef PST
	{ pst_start, pst_shutdown, noentry, pst_leap,
	  pst_control, pst_init, pst_buginfo, STDPOLL, NOFLAGS },
#else
	{ noentry, noentry, noentry, noentry,	/* 3 REFCLOCK_WWV_PST */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },
#endif

#ifdef WWVB
	{ wwvb_start, wwvb_shutdown, wwvb_poll, wwvb_leap,
	  wwvb_control, wwvb_init, wwvb_buginfo, STDPOLL, NOFLAGS },
#else
	{ noentry, noentry, noentry, noentry,	/* 4 REFCLOCK_WWVB_SPECTRACOM */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },
#endif

	{ noentry, noentry, noentry, noentry,	/* 5 REFCLOCK_GOES_TRUETIME */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },

	{ noentry, noentry, noentry, noentry,	/* 6 REFCLOCK_GOES_TRAK */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },

#ifdef CHU
	{ chu_start, chu_shutdown, chu_poll, noentry,
	  chu_control, chu_init, noentry, STDPOLL, NOFLAGS },
#else
	{ noentry, noentry, noentry, noentry,	/* 7 REFCLOCK_CHU */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS },
#endif

	{ noentry, noentry, noentry, noentry,	/* extra, no comma for ANSI */
	  noentry, noentry, noentry, NOPOLL, NOFLAGS }
};

int num_refclock_conf = sizeof(refclock_conf)/sizeof(struct refclock);

#endif
