/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)cs:cs/dkdial.c	1.5.1.5"
#ident	"$Header: $"

/*
 *	create a Datakit connection to a remote destination
 */
/*#ifndef DIAL
	static char	SCCSID[] = "dkdial.c	2.7+BNU DKHOST 87/03/09";
#endif */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <unistd.h>
#include <fcntl.h>
#include "dk.h"
#include <stdio.h>
#include <signal.h>
#define	SIGRTN	void
#include <setjmp.h>
#include "uucp.h"
#include "sysexits.h"
#include <errno.h>


#define DK_DEFWAIT	89	/* default time to wait for dial return */
#define	DK_MAXWAIT	600	/* maximum wait to allow the caller - 10 min */


GLOBAL unsigned int	dk_timewait = DK_DEFWAIT; /* Caller to dkdial might modify */

static SIGRTN	timout();	/* Alarm signal handler */
static void	setalarm(), usralarm();
EXTERN int	dkndial();
static int	Elapsed;	/* Alarm time elapsed during dial */
static int	Timer;		/* Current alarm setting */
static short	TimeErr;	/* Alarm clock rang */
static	size_t tp_errno;
static	char tp_ebuf[80];
extern char	*getenv();
EXTERN int	dk_verbose, dk_errno;

GLOBAL int
dkdial(dest)
	char *dest;
{
	return(dkndial(dest, atoi(getenv("DKINTF"))));
}

GLOBAL int
dkndial(dest, intf)
	char *dest;
{
	short		fd;		/* Channel Descriptor	*/
	SIGRTN		(*SigWas)();	/* Caller's alarm handler */
	unsigned int	TimWas;		/* Caller's alarm clock */
	char		*key;
	struct diocdial {
			struct	diocreq iocb;
			char	dialstring[128];
		}	ioreq;
	char		dial_dev[32];


	sprintf(dial_dev, "/dev/dk/dial%d", intf);

	/*
	** Clear our elapsed time and save caller's alarm stuff.
	*/

	Timer = Elapsed = 0;
	SigWas = signal(SIGALRM, timout);
	TimWas = alarm(0);

	/*
	** If requested timeout interval is unreasonable, use the default.
	*/

	if ((dk_timewait == 0)  || (dk_timewait > DK_MAXWAIT))
		dk_timewait = DK_DEFWAIT;

	/*
	** Do an alarm protected open of the dial device
	*/

	setalarm(dk_timewait);

	if ((fd = dev_open(dial_dev, O_RDWR)) < 0){
		setalarm(0);
		(void) sprintf(Scratch, gettxt(":36", "dkdial: Can't open %s"), dial_dev);
		debug(Scratch);
		usralarm(TimWas, SigWas);
		if (errno == EBUSY)
			return(dk_errno = -EX_TEMPFAIL);
		else
			return(dk_errno = -EX_OSFILE);
	}

	/*
	** If the caller has a DKKEY, use it.
	*/

	if((key = getenv("DKKEY")) != NULL && getuid() == geteuid())
		sprintf(ioreq.dialstring, "%s\n%s", dest, key);
	else
		strcpy(ioreq.dialstring, dest);

	ioreq.iocb.req_traffic = 0;
	ioreq.iocb.req_1param = 0;
	ioreq.iocb.req_2param = 0;

	/*
	** Try to dial the call.  If the alarm expires during the ioctl,
	** the ioctl will return in error.
	*/

	if (ioctl(fd, DKIODIAL, &ioreq) < 0) {
		setalarm(0);
		if (dk_verbose){
			if (TimeErr){
			    (void) sprintf(Scratch,
				gettxt(":37", "Can't connect to %s: %s\n"),
				gettxt(":38", "No response from Datakit"),
				ioreq.dialstring);
			    debug(Scratch);
			}
		} 
		else {
			(void) sprintf(Scratch,
			    gettxt(":37", "Can't connect to %s: %s\n"),
			    ioreq.dialstring,
			    dkerr(ioreq.iocb.req_error));
			debug(Scratch);
		}
		setalarm(2);		/* Don't wait forever on close */
		close(fd);
		usralarm(TimWas, SigWas);
		if (errno == EBUSY)
			return(-dkerrmap(dk_errno = -EX_TEMPFAIL));
		else
			return(-dkerrmap(dk_errno = ioreq.iocb.req_error));
	}
	usralarm(TimWas, SigWas);
	return (fd);
}

/*
** timout() is the alarm clock signal handling routine.  It is called
** whenever the alarm clock expires during dial processing.
*/

/* ARGSUSED */
static SIGRTN
timout(arg)
int arg;
{
	TimeErr++;
}

/*
** setalarm() is called to request an alarm at a future time.  The residual
** from the previous alarm (if any) is added to the elapsed time counter.
*/

static void
setalarm(Seconds)
{
	TimeErr = 0;
	(void) signal(SIGALRM, timout);
	Elapsed += Timer - alarm(Seconds);
	Timer = Seconds;
}

/*
** usralarm() is used to restore the alarm service for the caller.
*/

static void
usralarm(TimWas, SigWas)
	int		TimWas;		/* Caller's alarm clock */
	SIGRTN		(*SigWas)();	/* Caller's alarm handler */
{
	Elapsed += Timer - alarm(0);
	(void) signal(SIGALRM, SigWas);
	if (TimWas > 0) {
		TimWas -= Elapsed;
		if (TimWas < 2)
			TimWas = 2;
	}
	alarm(TimWas);
}
