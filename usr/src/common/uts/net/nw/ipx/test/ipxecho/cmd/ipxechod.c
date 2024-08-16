/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/cmd/ipxechod.c	1.1"
/*  Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.  */
/*  Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Righ
ts Reserved.    */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.  */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "$Id"
/*****************************************************************************
 *	NAME: ipxechod.c
 *
 *	DATE: 18 MAY 99		AUTHOR: 
 *
 *	USAGE: ipxechod \
 *		<number_sockets_to_bind> 		# Default is 2
 *
 *	DESCRIPTION: 	Echo back all ipx packets received. 
 *
 *	ipxechod: Phase I (basic functionality).
 *
 ******************************************************************************/
#include "ipxe.h"

char ModName[24];					/* this module's name */
char ErrorStr[80];						/* error string for perror */

static int OpenSocket(int *, uint16 *);
static void Daemonize(void);
static int InitKernIpxEcho(void);
static int InitUserIpxEcho(void);
static void sigfunc(int);
static void IpxUserEcho(int);
static int OpenIpxDevice(void);

static int KernIpxFd, UserIpxFd;	/* global file descriptors */

main( int argc, char *argv[] )
{
	int status;						/* main engine start loop status */
	int	numSockets;					/* number of engines to start */
	int	i;

	strcpy( ModName, argv[0] );		/* copy in name of this module */

	if ( argc < 2 )					/* check for proper num of args */
		numSockets = 0;				/* default 0 - don't bind extra sockets */
	else {
		numSockets = atoi(argv[1]);	/* convert num of sockets to integer */
		numSockets -= 2;
		if( numSockets < 0)
			numSockets = 0;
	}

	Daemonize();					/* turn into a daemon */


	for( i = 0; i < numSockets; i++) {
		if( OpenIpxDevice() == -1 )
			return(-1);
	}

	if ( (status = InitKernIpxEcho()) < 0 )	{
		fprintf( stderr, "%s:%d: Failure Initializing IpxEcho module\n",
			ModName, getpid(), ModName);
		fflush( stderr );
		exit( status );
	}

	if ( (status = InitUserIpxEcho()) < 0 )	{
		fprintf( stderr, "%s:%d: Failure Initializing IpxEcho module\n",
			ModName, getpid(), ModName);
		fflush( stderr );
		exit( status );
	}

	IpxUserEcho( UserIpxFd);

#ifdef NEVER
	fprintf( stderr, "%s:%d: exiting with status[%d].\n",
		ModName, getpid(), status );
#endif
	return ( status );

} /* end main() */


/*==============================================================================
 * InitKernIpxEcho()
 *
 *		- Open IPX stream (/dev/ipx). 			
 *		- Bind IPX socket.
 *		- Push IPXECHO on IPX
 *		- Return IPX's file descriptor.
 *============================================================================*/
static int		
InitKernIpxEcho( void )
{

	uint16 kSocket = KERNEL_SOCKET;

    if ( OpenSocket( &KernIpxFd, &kSocket ) == -1) {
        return ( -1 ); 
    }

	if( ioctl( KernIpxFd, I_PUSH, ECHO_MODULE) == -1) {
		sprintf( ErrorStr, "%s:%d: push module %s on IPX failed",
			ModName, getpid(), ECHO_MODULE );
		perror( ErrorStr);					
        return ( -1 ); 
	}
	printf("%s:%d: Bound socket 0x%x as kernel echo socket\n",
		ModName, getpid(), KERNEL_SOCKET);
	return ( KernIpxFd );		/* return file descriptor of control device */

} /* end InitKernIpxEcho() */

/*==============================================================================
 * InitUserIpxEcho()
 *
 *		- Open IPX stream (/dev/ipx). 			
 *		- Bind IPX socket.
 *		- Return IPX's file descriptor.
 *============================================================================*/
static int		
InitUserIpxEcho( void )
{

	uint16 kSocket = USER_SOCKET;

    if ( OpenSocket( &UserIpxFd, &kSocket ) == -1) {
        return ( -1 ); 
    }

	printf("%s:%d: Bound socket 0x%x as user echo socket\n",
		ModName, getpid(), USER_SOCKET);
	return ( UserIpxFd );		/* return file descriptor of control device */

} /* end InitKernIpxEcho() */

/*==============================================================================
 * Daemonize()
 *
 *	Make this process a daemon.
 *
 *	No need to make a real daemon yet because we still need to write output 
 *	to a terminal.
 * 
 *	This is taken out of UNIX network programing by Stevens.
 *============================================================================*/
static void
Daemonize( void )
{
	int	childPid;

	/*
	** Temporary until this becomes a daemon who doesn't output to a terminal.
	*/

	if ( ( childPid = fork() ) < 0 )
		fprintf( stderr, "%s:%d: can't fork 1st child.\n", ModName, getpid());
	else if ( childPid > 0 )
		exit ( 0 );				/* parent */

	if ( setsid() == -1 ) {
		fprintf( stderr, "%s:%d: can't set session leader.\n", 
			ModName, getpid());
	}
	if ( setpgrp() == -1 ) {
		fprintf( stderr, "%s:%d: can't set process leader.\n", 
			ModName, getpid());
	}

	signal( SIGTERM, sigfunc );
	signal( SIGQUIT, sigfunc );
	signal( SIGHUP, sigfunc );
	signal( SIGCLD, SIG_IGN );		/* ignore dying children */
    return;

} /* end Daemonize() */

static void
sigfunc(int	sig)
{
	switch(sig) {
	case SIGHUP:
		fprintf(stderr,"%s:%d: Got signal SIGHUP\n", ModName, getpid());
		break;
	case SIGQUIT:
		fprintf(stderr,"%s:%d: Got signal SIGQUIT\n", ModName, getpid());
		break;
	case SIGTERM:
		fprintf(stderr,"%s:%d: Got signal SIGTERM\n", ModName, getpid());
		break;
	default:
		fprintf(stderr,"%d:%d: Got unknown signal %d\n", ModName, getpid(),sig);
		break;
	}
	exit(-1);
}

static int
OpenSocket(int *fd, uint16 *socket)
{
	int		IpxFd;
	struct strioctl ioc;

	if ( (IpxFd = open(IPX, O_RDWR )) == -1 ) 		/* open IPX */
	{
        sprintf( ErrorStr,"%s:%d: open of %s failed", ModName, getpid(), IPX);
		perror( ErrorStr);						
        return ( -1 ); 
    }

    /* build an ioctl to set the socket */
	
    ioc.ic_dp = (char *)socket;
    ioc.ic_len = sizeof(uint16);
    ioc.ic_cmd = IPX_SET_SOCKET;
    ioc.ic_timout = 0;
    if (ioctl(IpxFd, I_STR, &ioc) == -1) {
        sprintf( ErrorStr,"%s:%d: IOCTL IPX_SET_SOCKET failed", 
			ModName, getpid());
		perror( ErrorStr);						
        return ( -1 ); 
    }
	if (ioc.ic_len != sizeof(uint16)) {
        sprintf( ErrorStr,"%s:%d: IOCTL IPX_SET_SOCKET returned bad length", 
			ModName, getpid());
        return ( -1 ); 
	}

	*fd = IpxFd;

	return ( 0 );
}

static void
IpxUserEcho(int fd)
{
	int flags;						/* streams flags */
	struct strbuf dbuf;				/* streams cntl/data buffer */
	char		  data[1500];		/* Buf for data if any */
	ipxHdr_t	*i;					/* Ipx header */
	ipxAddr_t	tmpAddr;

	i = (ipxHdr_t *)data;

	for( ;; ) {
		/*
		** Block while reading messages from the stream head (control device).
		*/
		dbuf.maxlen = sizeof(data);
		dbuf.len = 0;
		dbuf.buf = (char *)&data;
		flags = 0;
		if ( getmsg( fd, NULL, &dbuf, &flags ) == -1 ) {
			sprintf( ErrorStr, "%s:%d: getmsg on %s failed",
				ModName, getpid(), IPX );
			perror( ErrorStr );
			return;
		}

		IPXCOPYADDR( &i->dest, &tmpAddr);
		IPXCOPYADDR( &i->src, &i->dest);
		IPXCOPYADDR( &tmpAddr, &i->src);

		flags = 0;
		if ( putmsg( fd, NULL, &dbuf, flags ) == -1 ) {
			sprintf( ErrorStr, "%s:%d: putmsg on %s failed",
				ModName, getpid(), IPX );
			perror( ErrorStr );
			return;
		}
	}
} /* IpxUserEcho() */

static int		
OpenIpxDevice( void )
{
	uint16 Socket = 0;

    if ( OpenSocket( &UserIpxFd, &Socket ) == -1) {
        return ( -1 ); 
    }
	return ( 0 );

} /* end InitKernIpxEcho() */
/******************************  NWd.c  ***************************************/
