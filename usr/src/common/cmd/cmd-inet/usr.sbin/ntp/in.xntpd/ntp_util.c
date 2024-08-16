/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntp_util.c	1.2"
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
 * ntp_util.c - stuff I didn't have any other place for
 */
#include <stdio.h>
#ifdef SYSV
#include <sys/fcntl.h>
#endif /* SYSV */
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#ifdef convex
#include "/sys/sync/queue.h"
#include "/sys/sync/sema.h"
#endif
#include <net/if.h>
#include <netinet/in.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"


/*
 * This contains odds and ends.  Right now the only thing you'll find
 * in here is the hourly stats printer and some code to support rereading
 * the keys file, but I may eventually put other things in here such as
 * code to do something with the leap bits.
 */

/*
 * Name of the keys file
 */
char *key_file_name;

/*
 * The name of the drift_comp file and the temporary.
 */
char *stats_drift_file;
char *stats_temp_file;

/*
 * We query the errno to see what kind of error occured
 * when opening the drift file.
 */
extern int errno;

#ifdef DEBUG
extern int debug;
#endif

/*
 * init_util - initialize the utilities
 */
void
init_util()
{
	stats_drift_file = 0;
	stats_temp_file = 0;
	key_file_name = 0;
}


/*
 * hourly_stats - print some interesting stats
 */
void
hourly_stats()
{
	int fd;
	char *val;
	int vallen;
	extern l_fp drift_comp;
	extern long compliance;
	extern char *lfptoa();
	extern char *mfptoa();

#ifdef DOSYNCTODR
	struct timeval tv;
	int o_prio;

/*
 * Sometimes having a Sun can be a drag.
 *
 * The kernel variable dosynctodr controls whether the system's
 * soft clock is kept in sync with the battery clock. If it
 * is zero, then the soft clock is not synced, and the battery
 * clock is simply left to rot. That means that when the system
 * reboots, the battery clock (which has probably gone wacky)
 * sets the soft clock. That means xntpd starts off with a very
 * confused idea of what time it is. It then takes a large
 * amount of time to figure out just how wacky the battery clock
 * has made things drift, etc, etc. The solution is to make the
 * battery clock sync up to system time. The way to do THAT is
 * to simply set the time of day to the current time of day, but
 * as quickly as possible. This may, or may not be a sensible
 * thing to do.
 */

	o_prio=getpriority(PRIO_PROCESS,0); /* Save setting */
	if (setpriority(PRIO_PROCESS,0,-20) != 0) /* overdrive */
	{
		syslog(LOG_ERR, "can't elevate priority: %m");
		goto skip;
	}
	gettimeofday(&tv,(struct timezone *)NULL);
	if (settimeofday(&tv,(struct timezone *)NULL) != 0)
	{
		syslog(LOG_ERR, "can't sync battery time: %m");
	}
	setpriority(PRIO_PROCESS,0,o_prio); /* downshift */

skip:
#endif

	syslog(LOG_INFO, "hourly check: drift %s compliance %s",
		lfptoa(&drift_comp, 8),
		mfptoa((compliance<0)?(-1):0, compliance, 8));
	
	if (stats_drift_file != 0) {
		fd = open(stats_temp_file, O_WRONLY|O_TRUNC|O_CREAT, 0664);
		if (fd == -1) {
			syslog(LOG_ERR, "can't open %s: %m", stats_temp_file);
			return;
		}

		val = lfptoa(&drift_comp, 9);
		vallen = strlen(val);
		/*
		 * Hack here.  Turn the trailing \0 into a \n and write it.
		 */
		val[vallen] = '\n';
		if (write(fd, val, vallen+1) == -1) {
			syslog(LOG_ERR, "write to %s failed: %m",
			    stats_temp_file);
			(void) close(fd);
			(void) unlink(stats_temp_file);
		} else {
			(void) close(fd);
#if defined(SYSV) && !( defined(M_UNIX) || defined(M_XENIX) )
			/* atomic  - NOT! */
			(void) unlink(stats_drift_file);
			(void) link(stats_temp_file, stats_drift_file);
			(void) unlink(stats_temp_file);
#else
			/* atomic */
			(void) rename(stats_temp_file, stats_drift_file);
#endif
		}
	}
}


/*
 * stats_config - configure the stats operation
 */
void
stats_config(item, value)
	int item;
	char *value;	/* only one type so far */
{
	register char *cp;
	FILE *fp;
	int len;
	char buf[128];
	l_fp old_drift;
	extern char *emalloc();
	extern void loop_config();
	extern char *lfptoa();

	switch(item) {
	case STATS_FREQ_FILE:
		if (stats_drift_file != 0) {
			(void) free(stats_drift_file);
			(void) free(stats_temp_file);
			stats_drift_file = 0;
			stats_temp_file = 0;
		}

		if (value == 0 || (len = strlen(value)) == 0)
			break;

		stats_drift_file = emalloc((u_int)(len + 1));
		stats_temp_file = emalloc((u_int)(len + sizeof(".TEMP")));
		bcopy(value, stats_drift_file, len+1);
		bcopy(value, stats_temp_file, len);
		bcopy(".TEMP", stats_temp_file + len, sizeof(".TEMP"));
#ifdef DEBUG
		if (debug > 1) {
			printf("stats drift file %s\n", stats_drift_file);
			printf("stats temp file %s\n", stats_temp_file);
		}
#endif

		if ((fp = fopen(stats_drift_file, "r")) == NULL) {
			if (errno != ENOENT)
				syslog(LOG_ERR, "can't open %s: %m",
				    stats_drift_file);
			break;
		}

		if (fgets(buf, sizeof buf, fp) == NULL) {
			syslog(LOG_ERR, "can't read %s: %m",
			    stats_drift_file);
			(void) fclose(fp);
			break;
		}

		(void) fclose(fp);

		/*
		 * We allow leading spaces, then the number.  Terminate
		 * at any trailing space or string terminator.
		 */
		cp = buf;
		while (isspace(*cp))
			cp++;
		while (*cp != '\0' && !isspace(*cp))
			cp++;
		*cp = '\0';

		if (!atolfp(buf, &old_drift)) {
			syslog(LOG_ERR, "drift value %s invalid", buf);
			break;
		}

		/*
		 * Finally!  Give value to the loop filter.
		 */
#ifdef DEBUG
		if (debug > 1) {
			printf("loop_config finds old drift of %s\n",
			    lfptoa(&old_drift, 9));
		}
#endif
		loop_config(LOOP_DRIFTCOMP, &old_drift);
		break;
	
	default:
		/* oh well */
		break;
	}
}


/*
 * getauthkeys - read the authentication keys from the specified file
 */
void
getauthkeys(keyfile)
	char *keyfile;
{
	int len;
	extern char *emalloc();

	len = strlen(keyfile);
	if (len == 0)
		return;
	
	if (key_file_name != 0) {
		if (len > (int) strlen(key_file_name)) {
			(void) free(key_file_name);
			key_file_name = 0;
		}
	}

	if (key_file_name == 0)
		key_file_name = emalloc((u_int)(len + 1));
	
	bcopy(keyfile, key_file_name, len+1);

	authreadkeys(key_file_name);
}


/*
 * rereadkeys - read the authentication key file over again.
 */
void
rereadkeys()
{
	if (key_file_name != 0)
		authreadkeys(key_file_name);
}

#include <sys/resource.h>

#define NOFILES 20      /* just in case */

int
getdtablesize()
{
	struct rlimit   rl;
	
	if ( getrlimit(RLIMIT_NOFILE, &rl) == 0 )
		return(rl.rlim_max);
	else
		return(NOFILES);
}
