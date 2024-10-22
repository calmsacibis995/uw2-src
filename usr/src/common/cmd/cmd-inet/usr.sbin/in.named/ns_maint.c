/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/ns_maint.c	1.4.9.2"
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
 * System V STREAMS TCP - Release 4.0
 * 
 * Copyright 1990 Interactive Systems Corporation,(ISC) All Rights Reserved.
 * 
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) All Rights
 * Reserved.
 * 
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code.
 * 
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 * 
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */
/* SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1986, 1988 Regents of the University of California. All
 * rights reserved.
 * 
 * Redistribution and use in source and binary forms are permitted provided that
 * the above copyright notice and this paragraph are duplicated in all such
 * forms and that any documentation, advertising materials, and other
 * materials related to such distribution and use acknowledge that the
 * software was developed by the University of California, Berkeley.  The
 * name of the University may not be used to endorse or promote products
 * derived from this software without specific prior written permission. THIS
 * SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char     sccsid[] = "@(#)ns_maint.c	4.29 (Berkeley) 2/7/89";
#endif				/* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "ns.h"
#include "db.h"
#include "pathnames.h"

extern int      errno;
extern int      maint_interval;
extern int      needzoneload;
extern u_short ns_port;

int             xfers_running;	/* number of xfers running */
int             xfers_deferred;	/* number of needed xfers not run yet */
static int alarm_pending;

/*
 * Invoked at regular intervals by signal interrupt; refresh all secondary
 * zones from primary name server and remove old cache entries.  Also,
 * ifdef'd ALLOW_UPDATES, dump database if it has changed since last
 * dump/bootup.
 */
ns_maint()
{
	register struct zoneinfo *zp;
	int             ival;
	time_t          next_refresh = 0;
	int             zonenum;

#ifdef DEBUG
	if (debug)
		fprintf(ddt, "\nns_maint()\n");
#endif

	gettime(&tt);
	xfers_deferred = 0;
	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {
#ifdef DEBUG
		if (debug >= 2)
			printzoneinfo(zonenum);
#endif
		if (tt.tv_sec >= zp->z_time && zp->z_refresh > 0) {
			/*
			 * Set default time for next action first, so that it
			 * can be changed later if necessary.
			 */
			zp->z_time = tt.tv_sec + zp->z_refresh;

			switch (zp->z_type) {

			case Z_CACHE:
				doachkpt();
				break;

			case Z_SECONDARY:
				if ((zp->z_state & Z_NEED_RELOAD) == 0) {
					if (zp->z_state & Z_XFER_RUNNING)
						abortxfer(zp);
					else if (xfers_running < MAX_XFERS_RUNNING)
						startxfer(zp);
					else {
						zp->z_state |= Z_NEED_XFER;
						++xfers_deferred;
#ifdef DEBUG
						if (debug > 1)
							fprintf(ddt,
								"xfer deferred for %s\n",
							      zp->z_origin);
#endif
					}
				}
				break;
#ifdef ALLOW_UPDATES
			case Z_PRIMARY:
				/*
				 * Checkpoint the zone if it has changed
				 * since we last checkpointed
				 */
				if (zp->hasChanged)
					zonedump(zp);
				break;
#endif				/* ALLOW_UPDATES */
			}
			gettime(&tt);
		}
	}
	sched_maint();
#ifdef DEBUG
        if (debug)
                fprintf(ddt,"exit ns_maint()\n");
#endif
}

/* TPW */

/*
 * Find when the next refresh needs to be and set
 * interrupt time accordingly.
 */
sched_maint()
{
	register struct zoneinfo *zp;
	int ival;
	unsigned        tval;
	time_t next_refresh = 0;
	static time_t next_alarm;

	for (zp = zones; zp < &zones[nzones]; zp++)
		if (zp->z_time != 0 &&
		    (next_refresh == 0 || next_refresh > zp->z_time))
			next_refresh = zp->z_time;
        /*
	 *  Schedule the next call to ns_maint.
	 *  Don't visit any sooner than maint_interval.
	 */
	ival = 0;
	if (next_refresh != 0) {
		if (next_refresh == next_alarm && alarm_pending) {
#ifdef DEBUG
			if (debug)
			    fprintf(ddt,"sched_maint: no schedule change\n");
#endif
			return;
		}
		ival = next_refresh - tt.tv_sec;
		if (ival < maint_interval)
			ival = maint_interval;
		tval = alarm(ival);
	}
#ifdef DEBUG
        if (debug)
                fprintf(ddt, "exit ns_maint() Next interrupt in %d sec\n",
                        ival);
#endif
}
/*
 * Start an asynchronous zone transfer for a zone.
 * Depends on current time being in tt.
 * The caller must set an alarm to reschedule after calling startxfer.
 */
startxfer(zp)
	struct zoneinfo *zp;
{
	static char *argv[NSMAX + 20], argv_ns[NSMAX][MAXDNAME];
	int cnt, argc = 0, argc_ns = 0, pid, omask;
	char debug_str[10];
	char serial_str[10];
	char port_str[10];

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"startxfer() %s\n", zp->z_origin);
#endif

	argv[argc++] = "named-xfer";
	argv[argc++] = "-z";
	argv[argc++] = zp->z_origin;
	argv[argc++] = "-f";
	argv[argc++] = zp->z_source;
	argv[argc++] = "-s";
	sprintf(serial_str, "%d", zp->z_serial);
	argv[argc++] = serial_str;
	if (zp->z_state & Z_SYSLOGGED)
		argv[argc++] = "-q";
	argv[argc++] = "-P";
	sprintf(port_str, "%d", ns_port);
	argv[argc++] = port_str;
#ifdef DEBUG
	if (debug) {
		argv[argc++] = "-d";
		sprintf(debug_str, "%d", debug);
		argv[argc++] = debug_str;
		argv[argc++] = "-l";
		argv[argc++] = "/usr/tmp/xfer.ddt";
		if (debug > 5) {
			argv[argc++] = "-t";
			argv[argc++] = "/usr/tmp/xfer.trace";
		}
	}
#endif
	
	/*
	 * Copy the server ip addresses into argv, after converting
	 * to ascii and saving the static inet_ntoa result
	 */
	for (cnt = 0; cnt < zp->z_addrcnt; cnt++)
		argv[argc++] = strcpy(argv_ns[argc_ns++],
		    inet_ntoa(zp->z_addr[cnt]));

	argv[argc] = 0;

#ifdef DEBUG
#ifdef ECHOARGS
	if (debug) {
		int i;
		for (i = 0; i < argc; i++) 
			fprintf(ddt, "Arg %d=%s\n", i, argv[i]);
        }
#endif /* ECHOARGS */
#endif /* DEBUG */

#ifdef SYSV
#define vfork fork
#endif
	gettime(&tt);
	if ((pid = vfork()) == -1) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "xfer [v]fork: %d\n", errno);
#endif
		syslog(LOG_ERR, "xfer [v]fork: %m");
		zp->z_time = tt.tv_sec + 10;
		return;
	}

	if (pid) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "started xfer child %d\n", pid);
#endif
		zp->z_state &= ~Z_NEED_XFER;
		zp->z_state |= Z_XFER_RUNNING;
		zp->z_xferpid = pid;
		xfers_running++;
		zp->z_time = tt.tv_sec + MAX_XFER_TIME;
	} else {
		execve(_PATH_XFER, argv, NULL);
		syslog(LOG_ERR, "can't exec %s: %m", _PATH_XFER);
		_exit(XFER_FAIL);	/* avoid duplicate buffer flushes */
	}
}

#ifdef DEBUG
printzoneinfo(zonenum)
	int             zonenum;
{
	struct timeval  tt;
	struct zoneinfo *zp = &zones[zonenum];
	char *ZoneType;

	if (!debug)
		return; /* Else fprintf to ddt will bomb */
	fprintf(ddt, "printzoneinfo(%d):\n", zonenum);

	gettime(&tt);
	switch (zp->z_type) {
		case Z_PRIMARY: ZoneType = "Primary"; break;
		case Z_SECONDARY: ZoneType = "Secondary"; break;
		case Z_CACHE: ZoneType = "Cache"; break;
		default: ZoneType = "Unknown";
	}
	if (zp->z_origin[0] == '\0')
		fprintf(ddt,"origin ='.'");
	else
		fprintf(ddt,"origin ='%s'", zp->z_origin);
	fprintf(ddt,", type = %s", ZoneType);
	fprintf(ddt,", source = %s\n", zp->z_source);
	fprintf(ddt,"z_refresh = %ld", zp->z_refresh);
	fprintf(ddt,", retry = %ld", zp->z_retry);
	fprintf(ddt,", expire = %ld", zp->z_expire);
	fprintf(ddt,", minimum = %ld", zp->z_minimum);
	fprintf(ddt,", serial = %ld\n", zp->z_serial);
	fprintf(ddt,"z_time = %d", zp->z_time);
	if (zp->z_time) {
		fprintf(ddt,", now time : %d sec", tt.tv_sec);
		fprintf(ddt,", time left: %d sec", zp->z_time - tt.tv_sec);
	}
	fprintf(ddt,"; state %x\n", zp->z_state);
}
#endif /* DEBUG */

/*
 * remove_zone (htp, zone) --
 *     Delete all RR's in the zone "zone" under specified hash table.
 */
remove_zone(htp, zone)
	register struct hashbuf *htp;
	register int zone;
{
	register struct databuf *dp, *pdp;
	register struct namebuf *np;
	struct namebuf **npp, **nppend;

	nppend = htp->h_tab + htp->h_size;
	for (npp = htp->h_tab; npp < nppend; npp++)
	    for (np = *npp; np != NULL; np = np->n_next) {
		for (pdp = NULL, dp = np->n_data; dp != NULL; ) {
			if (dp->d_zone == zone)
				dp = rm_datum(dp, np, pdp);
			else {
				pdp = dp;
				dp = dp->d_next;
			}
		}
		/* Call recursively to remove subdomains. */
		if (np->n_hash)
			remove_zone(np->n_hash, zone);
	    }
}
   
/*
 * Abort an xfer that has taken too long.
 */
abortxfer(zp)
	register struct zoneinfo *zp;
{

	kill(zp->z_xferpid, SIGKILL); /* don't trust it at all */
#ifdef DEBUG
	if (debug)
	  fprintf(ddt, "Killed child %d (zone %s) due to timeout\n",
	     zp->z_xferpid, zp->z_origin);
#endif /* DEBUG */
	zp->z_time = tt.tv_sec + zp->z_retry;
}

/*
 * SIGCLD signal handler: process exit of xfer's.
 * (Note: also called when outgoing transfer completes.)
 */
VOID
endxfer()
{
    	register struct zoneinfo *zp;   
	int xfers = 0;
	pid_t pid;
	int status;

	gettime(&tt);
	while ((pid = waitpid((pid_t) -1, &status, WNOHANG)) > 0) {
		for (zp = zones; zp < &zones[nzones]; zp++)
		    if (zp->z_xferpid == pid) {
			xfers++;
			xfers_running--;
			zp->z_xferpid = 0;
			zp->z_state &= ~Z_XFER_RUNNING;
#ifdef DEBUG
			if (debug) 
			    fprintf(ddt,
		"\nendxfer: child %d zone %s returned status=%d termsig=%d\n", 
				pid, zp->z_origin, WEXITSTATUS(status),
				WTERMSIG(status));
#endif
			if (WIFSIGNALED(status)) {
				if (WTERMSIG(status) != SIGKILL) {
					syslog(LOG_ERR,
					   "named-xfer exited with signal %d\n",
					   WTERMSIG(status));
#ifdef DEBUG
					if (debug)
					    fprintf(ddt,
					 "\tchild termination with signal %d\n",
						WTERMSIG(status));
#endif
				}
				zp->z_time = tt.tv_sec + zp->z_retry;
			} else switch (WEXITSTATUS(status)) {
				case XFER_UPTODATE:
					zp->z_state &= ~Z_SYSLOGGED;
					zp->z_lastupdate = tt.tv_sec;
					zp->z_time = tt.tv_sec + zp->z_refresh;
					/*
					 * Restore z_auth in case expired,
					 * but only if there were no errors
					 * in the zone file.
					 */
					if ((zp->z_state & Z_DB_BAD) == 0)
						zp->z_auth = 1;
					if (zp->z_source) {
#if defined(SYSV)
						struct utimbuf t;

						t.actime = tt.tv_sec;
						t.modtime = tt.tv_sec;
						(void) utime(zp->z_source, &t);
#else
						struct timeval t[2];

						t[0] = tt;
						t[1] = tt;
						(void) utimes(zp->z_source, t);
#endif /* SYSV */
					}
					break;

				case XFER_SUCCESS:
					zp->z_state |= Z_NEED_RELOAD;
					zp->z_state &= ~Z_SYSLOGGED;
					needzoneload++;
					break;

				case XFER_TIMEOUT:
#ifdef DEBUG
					if (debug) fprintf(ddt,
		    "zoneref: Masters for secondary zone %s unreachable\n",
					    zp->z_origin);
#endif
					if ((zp->z_state & Z_SYSLOGGED) == 0) {
						zp->z_state |= Z_SYSLOGGED;
						syslog(LOG_WARNING,
		      "zoneref: Masters for secondary zone %s unreachable",
						    zp->z_origin);
					}
					zp->z_time = tt.tv_sec + zp->z_retry;
					break;

				default:
					if ((zp->z_state & Z_SYSLOGGED) == 0) {
						zp->z_state |= Z_SYSLOGGED;
						syslog(LOG_ERR,
						    "named-xfer exit code %d",
						    WEXITSTATUS(status));
					}
					/* FALLTHROUGH */
				case XFER_FAIL:
					zp->z_state |= Z_SYSLOGGED;
					zp->z_time = tt.tv_sec + zp->z_retry;
					break;
			}
			break;
		}
	}
	if (xfers) {
		for (zp = zones;
		    xfers_deferred != 0 && xfers_running < MAX_XFERS_RUNNING &&
		    zp < &zones[nzones]; zp++)
			if (zp->z_state & Z_NEED_XFER) {
				xfers_deferred--;
				startxfer(zp);
			}
		sched_maint();
	}
#ifdef SYSV
	(void)sigset(SIGCLD, (void (*)()) endxfer);
#endif
}

/*
 * Reload zones whose transfers have completed.
 */
loadxfer()
{
    	register struct zoneinfo *zp;   

	gettime(&tt);
	for (zp = zones; zp < &zones[nzones]; zp++)
	    if (zp->z_state & Z_NEED_RELOAD) {
#ifdef DEBUG
		if (debug)
			fprintf(ddt, "loadxfer() '%s'\n",
			zp->z_origin[0] ? zp->z_origin : ".");
#endif
		zp->z_state &= ~Z_NEED_RELOAD;
		zp->z_auth = 0;
		remove_zone(hashtab, zp - zones);
		if (db_load(zp->z_source, zp->z_origin, zp, 0) == 0)
			zp->z_auth = 1;
		if (zp->z_state & Z_TMP_FILE)
			(void) unlink(zp->z_source);
	    }
	sched_maint();
}
