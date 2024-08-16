/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/interface.c	1.3.2.7"
#ident	"$Header: $"

/*	interface( label )
	provide alternate definitions for the I/O functions through global
	interfaces.
*/
#include <unistd.h>
#include	"uucp.h"

#ifdef TLI
#include	<xti.h>
#endif /*  TLI  */

#ifdef DATAKIT
#include	"dk.h"

static int	dksetup();
static int	dkteardown();
#endif	/* DATAKIT */

EXTERN void	sethup();
EXTERN int	restline();
extern int	read(), write();
static int	usetup(), uteardown();

GLOBAL int	(*Read)() = read,
	(*Write)() = write,
#if defined(__STDC__)
	(*Ioctl)(int,int,...) = ioctl,
#else
	(*Ioctl)() = ioctl,
#endif
	(*Setup)() = usetup,
	(*Teardown)() = uteardown;

#ifdef TLI
EXTERN void tfaillog(), show_tlook();
static int	tread(), twrite(),	/* TLI i/o */
#ifdef __STDC__
		tioctl(int, int, ...),
#else
		tioctl(),		/* TLI i/o control */
#endif
		tsetup(),		/* TLI setup without streams module */
		tssetup(),		/* TLI setup with streams module */
		tteardown();		/* TLI teardown, works with either setup
					*/
#endif /*  TLI  */
/*	The IN_label in Interface[] imply different caller routines:
	e.g. tlicall().
	If so, the names here and the names in callers.c must match.
*/

static
  struct Interface {
	char	*IN_label;		/* interface name */
	int	(*IN_read)();		/* read function */
	int	(*IN_write)();		/* write function */
#ifdef __STDC__
	int	(*IN_ioctl)(int,int,...);
#else
	int	(*IN_ioctl)();		/* ioctl function */
#endif
	int	(*IN_setup)();		/* setup function, called before first
					i/o operation */
	int	(*IN_teardown)();	/* teardown function, called after last
					i/o operation */
} Interface[] = {
			/* vanilla UNIX */
		{ "UNIX", read, write, ioctl, usetup, uteardown },
#ifdef SYTEK
			/* Sytek network */
		{ "Sytek", read, write, ioctl, usetup, uteardown },
#endif /* Sytek network */
#ifdef DIAL801
			/* 801 auto dialers */
		{ "801", read, write, ioctl, usetup, uteardown },
#endif /* DIAL801 */
#ifdef DIAL801
			/* 212 auto dialers */
		{ "212", read, write, ioctl, usetup, uteardown },
#endif /* DIAL801 */
#ifdef TLI
			/* Transport Interface Library WITHOUT streams */
		{ "TLI", tread, twrite, tioctl, tsetup, tteardown },
#ifdef TLIS
			/* Transport Interface Library WITH streams */
		{ "TLIS", read, write, ioctl, tssetup, uteardown },
#endif /*  TLIS  */
#endif /*  TLI  */
#ifdef CS
			/* Transport Interface Library WITHOUT streams */
		{ "CS", tread, twrite, tioctl, tsetup, tteardown },
#endif /* CS */
#ifdef DATAKIT
		{ "DK", read, write, ioctl, dksetup, dkteardown },
#endif /* DATAKIT */
#ifdef UNET 
		{ "TCP", read, write, ioctl, usetup, uteardown },
#endif
#ifdef UNET
		{ "Unetserver", read, write, ioctl, usetup, uteardown },
#endif		
		{ 0, 0, 0, 0, 0, 0 }
	};


GLOBAL int
interface(label)
char	*label;
{
	register int	i;

	for ( i = 0;  Interface[i].IN_label;  ++i ) {
		if( !strcmp( Interface[i].IN_label, label ) ) {
			Read = Interface[i].IN_read;
			Write = Interface[i].IN_write;
			Ioctl = Interface[i].IN_ioctl;
			Setup = Interface[i].IN_setup;
			Teardown = Interface[i].IN_teardown;
			DEBUG(5, gettxt(":95", "setting interface: %s\n"), label);
			return( 0 );
		}
	}
	return( FAIL );
}

/*
 *	usetup - vanilla unix setup routine
 */
static int
usetup( role, fdreadp, fdwritep )
int	*fdreadp, *fdwritep;
{
	if ( role == SLAVE )
	{
		*fdreadp = 0;
		*fdwritep = 1;
		/* 2 has been re-opened to RMTDEBUG in main() */
	}
	return(SUCCESS);
}

/*
 *	uteardown - vanilla unix teardown routine
 */
static int
uteardown( role, fdread, fdwrite )
{
	int ret;
	char *ttyn;

	if (role == SLAVE) {
		ret = restline();
		DEBUG(4, gettxt(":313", "restline() returns - %d\n"), ret);
		sethup(0);
	}
	if (fdread != -1) {
		ttyn = ttyname(fdread);
		if (ttyn != NULL)
			chmod(ttyn, Dev_mode);	/* can fail, but who cares? */
		(void) close(fdread);
		(void) close(fdwrite);
	}
	return(SUCCESS);
}

#ifdef DATAKIT
/*
 *	dksetup - DATAKIT setup routine
 *
 * Put line in block mode.
 */

static int
dksetup (role, fdreadp, fdwritep)

int	role;
int *	fdreadp;
int *	fdwritep;

{
	static short dkrmode[3] = { DKR_BLOCK | DKR_TIME, 0, 0 };
	int	ret;

	(void) usetup(role, fdreadp, fdwritep);
	if((ret = (*Ioctl)(*fdreadp, DIOCRMODE, dkrmode)) < 0) {
		DEBUG(4, gettxt(":314", "dksetup: failed to set block mode; Ioctl returns %d\n"), ret);
		DEBUG(4, gettxt(":315", "(reading from fd=%d), "), *fdreadp);
		DEBUG(4, "errno=%d\n", errno);
		return(FAIL);
	}
	return(SUCCESS);
}

/*
 *	dkteardown  -  DATAKIT teardown routine
 */
static int
dkteardown( role, fdread, fdwrite )
int	role, fdread, fdwrite;
{
	char	*ttyn;

	if ( role == MASTER ) {
		ttyn = ttyname(fdread);
		if ( ttyn != NULL )
			chmod(ttyn, Dev_mode);	/* can fail, but who cares? */
	}

	/*	must flush fd's for datakit	*/
	/*	else close can hang		*/
	if ( ioctl(fdread, DIOCFLUSH, NULL) != 0 ) {
		DEBUG(4, gettxt(":316", "dkteardown: ioctl(DIOCFLUSH) of input fd=%d failed"), fdread);
		DEBUG(4, "dkteardown: errno %d", errno);
	}
	if ( ioctl(fdwrite, DIOCFLUSH, NULL) != 0 ) {
		DEBUG(4, gettxt(":317", "dkteardown: ioctl(DIOCFLUSH) of output fd=%d failed"), fdwrite);
		DEBUG(4, "dkteardown: errno %d", errno);
	}

	(void)close(fdread);
	(void)close(fdwrite);
	return(SUCCESS);
}
#endif /* DATAKIT */


#ifdef TLI
/*
 *	tread - tli read routine
 */
static int
tread(fd, buf, nbytes)
int		fd;
char		*buf;
unsigned	nbytes;
{
	int		rcvflags;

	return(t_rcv(fd, buf, nbytes, &rcvflags));
}

/*
 *	twrite - tli write routine
 */
#define	N_CHECK	100
static int
twrite(fd, buf, nbytes)
int		fd;
char		*buf;
unsigned	nbytes;
{
	register int		i, ret;
	static int		n_writ, got_info;
	static struct t_info	info;

	if ( got_info == 0 ) {
		if ( t_getinfo(fd, &info) != 0 ) {
			tfaillog(fd, "twrite: t_getinfo\n");
			return(FAIL);
		}
		got_info = 1;
	}

	/* on every N_CHECKth call, check that are still in DATAXFER state */
	if ( ++n_writ == N_CHECK ) {
		n_writ = 0;
		if ( t_getstate(fd) != T_DATAXFER )
			return(FAIL);
	}	

	if ( info.tsdu <= 0 || nbytes <= info.tsdu )
		return(t_snd(fd, buf, nbytes, NULL));

	/* if get here, then there is a limit on transmit size	*/
	/* (info.tsdu > 0) and buf exceeds it			*/
	i = ret = 0;
	while ( nbytes >= info.tsdu ) {
		if ( (ret = t_snd(fd,  &buf[i], info.tsdu, NULL)) != info.tsdu )
			return( ( ret >= 0 ? (i + ret) : ret ) );
		i += info.tsdu;
		nbytes -= info.tsdu;
	}
	if ( nbytes != 0 ) {
		if ( (ret = t_snd(fd,  &buf[i], nbytes, NULL)) != nbytes )
			return( ( ret >= 0 ? (i + ret) : ret ) );
		i += nbytes;
	}
	return(i);
}


/*
 *	tioctl - stub for tli ioctl routine
 */
/*ARGSUSED*/
static int
#ifdef __STDC__
tioctl(int fd, int request, ...)
#else
tioctl(fd, request, arg)
int	fd, request;
#endif
{
	return(SUCCESS);
}

/*
 *	tsetup - tli setup routine
 *	note blatant assumption that *fdreadp == *fdwritep == 0
 */
static int
tsetup( role, fdreadp, fdwritep )
int	*fdreadp, *fdwritep;
{
	extern int	t_errno;

	if ( role == SLAVE ) {
		*fdreadp = 0;
		*fdwritep = 1;
		/* 2 has been re-opened to RMTDEBUG in main() */
		errno = t_errno = 0;
		if ( t_sync(*fdreadp) == -1 || t_sync(*fdwritep) == -1 ) {
			tfaillog(*fdreadp, "tsetup: t_sync\n");
			return(FAIL);
		}
	}
	return(SUCCESS);
}

/*
 *	tteardown - tli shutdown routine
 */
/*ARGSUSED*/
static int
tteardown( role, fdread, fdwrite )
{
	(void)t_unbind(fdread);
	(void)t_close(fdread);
	return(SUCCESS);
}

#ifdef TLIS
/*
 *	tssetup - tli, with streams module, setup routine
 *	note blatant assumption that *fdreadp == *fdwritep
 */
static int
tssetup( role, fdreadp, fdwritep )
int	role;
int	*fdreadp;
int	*fdwritep;
{
	if ( role == SLAVE ) {
		*fdreadp = 0;
		*fdwritep = 1;
		/* 2 has been re-opened to RMTDEBUG in main() */
		DEBUG(5, gettxt(":318", "tssetup: SLAVE mode: leaving ok\n"), "");
		return(SUCCESS);
	}

	DEBUG(4, gettxt(":319", "tssetup: MASTER mode: leaving ok\n"), "");
	return(SUCCESS);
}

/*
 *	Report why a TLI call failed.
 */
GLOBAL void
tfaillog(fd, s)
int	fd;
char	*s;
{
	extern int	t_errno;
	char	fmt[ BUFSIZ ];

	sprintf( fmt, "%s: %s\n", s, t_strerror(t_errno));
	DEBUG(5, fmt, "");
	if ( t_errno == TLOOK ) {
		show_tlook(fd);
	}
	return;
}

GLOBAL void
show_tlook(fd)
int fd;
{
	register int reason;
	register char *msg;
	extern int	t_errno;
/*
 * Find out the current state of the interface.
 */
	errno = t_errno = 0;
	switch( reason = t_getstate(fd) ) {
	case T_UNBND:		msg = "T_UNBIND";	break;
	case T_IDLE:		msg = "T_IDLE";		break;
	case T_OUTCON:		msg = "T_OUTCON";	break;
	case T_INCON:		msg = "T_INCON";	break;
	case T_DATAXFER:	msg = "T_DATAXFER";	break;
	case T_OUTREL:		msg = "T_OUTREL";	break;
	case T_INREL:		msg = "T_INREL";	break;
	default:		msg = NULL;		break;
	}
	if( msg == NULL )
		return;

	DEBUG(5, gettxt(":320", "TLI state is %s"), msg);
	switch( reason = t_look(fd) ) {
	case -1:		msg = ""; break;
	case 0:			msg = "NO ERROR"; break;
	case T_LISTEN:		msg = "T_LISTEN"; break;
	case T_CONNECT:		msg = "T_CONNECT"; break;
	case T_DATA:		msg = "T_DATA";	 break;
	case T_EXDATA:		msg = "T_EXDATA"; break;
	case T_DISCONNECT:	msg = "T_DISCONNECT"; break;
	case T_ORDREL:		msg = "T_ORDREL"; break;
	case T_ERROR:		msg = "T_ERROR"; break;
	case T_UDERR:		msg = "T_UDERR"; break;
	default:		msg = "UNKNOWN ERROR"; break;
	}
	DEBUG(4, gettxt(":321", " event is %s\n"), msg);

	if ( reason == T_DISCONNECT )
	{
		struct t_discon	*dropped;

		if((dropped = (struct t_discon *)t_alloc(fd,T_DIS,T_ALL)) == 0)
			return;
		
		if (t_rcvdis(fd, dropped) == -1) {
			t_free((char *)dropped, T_DIS);
			return;
		}
		DEBUG(5, gettxt(":322", "disconnect reason %d\n"), dropped->reason);
		t_free((char *)dropped, T_DIS);
	}
	return;
}

#endif /*  TLIS  */

#endif /*  TLI  */
