/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:server/os/connection.c	1.8"
/* $XConsortium: connection.c,v 1.19 92/05/18 13:51:29 gildea Exp $ */
/*
 * handles connections
 */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this protoype software
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TCPCONN
#define TCPCONN
#endif

#include	<X11/Xos.h>
#include	<stdio.h>
#include	<errno.h>
#include	<sys/param.h>
#include	<sys/socket.h>
#include	<sys/uio.h>
#include	<signal.h>

#ifdef TCPCONN
#include	<netinet/in.h>
#include	<netinet/tcp.h>
#include        <netdb.h>
#endif

#include	"FS.h"
#include	"FSproto.h"
#include	"clientstr.h"
#include	"osdep.h"
#include	"globals.h"
#include	"osstruct.h"
#include	"servermd.h"

extern int  errno;

int         ListenPort = DEFAULT_FS_PORT;   /* port to listen on */
int         lastfdesc;

extern int  ListenSock;

long        WellKnownConnections;
long        AllSockets[mskcnt];
long        AllClients[mskcnt];
#ifdef SVR4
fd_set      LastSelectMask;
#else
long        LastSelectMask[mskcnt];
#endif /* SVR4 */
long        ClientsWithInput[mskcnt];
long        ClientsWriteBlocked[mskcnt];
long        OutputPending[mskcnt];
extern long MaxClients;
long        OutputBufferSize = BUFSIZE;

Bool        NewOutputPending;
Bool        AnyClientsWriteBlocked;

int         ConnectionTranslation[MAXSOCKS];
char *	    PIDfile; /* to support interaction with the SVR4.2 FontMgr */
Bool        HasTrustedClients = FALSE;
char **	    TrustedClientListPtr;
int	    NumberOfTrustedClients;
extern ClientPtr NextAvailableClient();

#ifdef SIGNALRETURNSINT
#define SIGVAL int
#else
#define SIGVAL void
#endif

extern SIGVAL AutoResetServer();
extern SIGVAL GiveUp();
extern SIGVAL ServerReconfig();
extern SIGVAL ServerCacheFlush();

extern void FreeOsBuffers();

static void error_conn_max();
static void close_fd();
#ifdef SVR4
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/stat.h>
#include <sys/select.h>
#define FS_NAMED_PATH    "/dev/X/xfont."
#define FS_PID_FILE	 "/dev/X/fs."
#define XLOCAL_NAMED 1
int namedFd = -1;
Bool localStreamsConnection;
#endif

#ifdef SVR4

static int
open_named_local()
{
    int fd,fld[2];
    char path[64];
    char port[10];
    struct stat sbuf;

#ifdef XLOCAL
    xlocalMsg("NAMED_LOCAL open...\n");
    mkdir (X_STREAMS_DIR, 0777);
    chmod (X_STREAMS_DIR, 0777);
#endif 
    strcpy (path, FS_NAMED_PATH);
    sprintf(port, "%d", ListenPort);
    strcat (path, port);
  
    if (stat(path, &sbuf) != 0) {
	if (errno == ENOENT) {
	    if ((fd = creat(path, (mode_t)0666)) == -1) {
		ErrorF("AT&T NAMED: Can't create %s\n",path);
		return(-1);
	    }
	    close(fd);
	    if (chmod(path, (mode_t)0666) < 0) {
		ErrorF("AT&T NAMED: Can't chmod %s\n",path);
		return(-1);
	    }
	} else {
	    ErrorF("AT&T NAMED: Unknown stat error, %d\n",errno);	
	    return(-1);
	}
    } 

    if (pipe(fld) != 0) {
	ErrorF("AT&T NAMED: pipe failed, errno=%d\n",errno);
	return(-1);
    }

    if (ioctl(fld[0], I_PUSH, "connld") != 0) {
	ErrorF("AT&T NAMED: ioctl error %d\n",errno);
	return(-1);
    }

    if (fattach(fld[0], path) != 0) {
	ErrorF("AT&T NAMED: fattach failed, errno=%d\n",errno);
	return(-1);
    } 
#ifdef XLOCAL
    xlocalMsg("NAMED_LOCAL available at [%s]\n",path);
#endif

    return(fld[1]);
}

#endif

#ifdef	TCPCONN
static int
open_tcp_socket()
{
    struct sockaddr_in insock;
    int         request;
    int         retry;

#ifndef SO_DONTLINGER

#ifdef SO_LINGER
    static int  linger[2] = {0, 0};

#endif				/* SO_LINGER */

#endif				/* SO_DONTLINGER */

    if ((request = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	Error("Creating TCP socket");
	return -1;
    }

#ifdef SO_REUSEADDR
    {
	int         one = 1;

	setsockopt(request, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
    }
#endif				/* SO_REUSEADDR */

    bzero((char *) &insock, sizeof(insock));
    insock.sin_family = AF_INET;
    insock.sin_port = htons((unsigned short) (ListenPort));
    insock.sin_addr.s_addr = htonl(INADDR_ANY);
    retry = 20;
    while (bind(request, (struct sockaddr *) & insock, sizeof(insock))) {
	if (--retry == 0) {
	    Error("Binding TCP socket");
	    close(request);
	    return -1;
	}

#ifdef SO_REUSEADDR
	sleep(1);
#else
	sleep(10);
#endif				/* SO_REUSEADDR */
    }

#ifdef SO_DONTLINGER
    if (setsockopt(request, SOL_SOCKET, SO_DONTLINGER, (char *) NULL, 0))
#ifdef DEBUG
	Error("Setting TCP SO_DONTLINGER");
else
	;
#endif /* DEBUG */
#else

#ifdef SO_LINGER
    if (setsockopt(request, SOL_SOCKET, SO_LINGER,
		   (char *) linger, sizeof(linger)))
	Error("Setting TCP SO_LINGER");
#endif				/* SO_LINGER */

#endif				/* SO_DONTLINGER */

    if (listen(request, 5)) {
	Error("TCP listening");
	close(request);
	return -1;
    }
    ListenSock = request;
    return request;
}

#endif				/* TCPCONN */

StopListening()
{
    BITCLEAR(AllSockets, ListenSock);
    close(ListenSock);
    BITCLEAR(AllSockets, namedFd);
    close(namedFd);
}

/*
 * creates the sockets for listening to clients
 *
 * only called when server first started
 */
void
CreateSockets(oldsock, oldpipe)
    int         oldsock;
    int         oldpipe;
{
    int         request,
                i;

     if( ListenSock < 0 ) { 
       struct stat buf;
       char filename[50], port[20];
       sprintf(port, "%d", ListenPort);
       strcpy(filename, FS_NAMED_PATH );
       strcat(filename, port);
       if(stat( filename, &buf) == 0 && ( buf.st_mode & S_IFIFO ))
	  FatalError("Font Server already running on port %s\n", port);
     }

    CLEARBITS(AllSockets);
    CLEARBITS(AllClients);
#ifdef SVR4
    FD_ZERO(&LastSelectMask);
#else
    CLEARBITS(LastSelectMask);
#endif /* SVR4 */
    CLEARBITS(ClientsWithInput);

    for (i = 0; i < MAXSOCKS; i++)
	ConnectionTranslation[i] = 0;

#if defined(hpux) || defined(SVR4)
        /*
        * _NFILE : initial number of streams
        * ES/MP doesn't have this definition - so if it is not defined
        * define it here
        */
#ifndef _NFILE
#define _NFILE 60
#endif

    lastfdesc = _NFILE - 1;
#else
    lastfdesc = getdtablesize() - 1;
#endif				/* hpux */

    if (lastfdesc > MAXSOCKS) {
	lastfdesc = MAXSOCKS;
    }
    WellKnownConnections = 0;

#ifdef TCPCONN
    if (oldsock >= 0) {		/* must be forked, and have a different socket
				 * to listen to */
	if (listen(oldsock, 5)) {
	    Error("TCP listening");
	    close(oldsock);
	    FatalError("Cannot re-establish the listening socket");
	    return;
	}
	NoticeF("Reusing existing file descriptor %d\n", oldsock);
	WellKnownConnections |= (1L << oldsock);
	WellKnownConnections |= (1L << oldpipe);
    } else {
	if ((request = open_tcp_socket()) != -1) {
	    WellKnownConnections |= (1L << request);
	}
        if ((request = open_named_local()) != -1) {
	    WellKnownConnections |= (1L << request);
	    namedFd = request;
	}
    }
#endif				/* TCPCONN */

    if (WellKnownConnections == 0)
	FatalError("Cannot establish any listening sockets");


    /* set up all the signal handlers */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, AutoResetServer);
    signal(SIGINT, GiveUp);
    signal(SIGTERM, GiveUp);
    signal(SIGUSR1, ServerReconfig);
    signal(SIGUSR2, ServerCacheFlush);

    AllSockets[0] = WellKnownConnections;


	/* write pid to /dev/X/fs.ListenPort.pid */
	if(WellKnownConnections)
        { 
        int fd, ipid;
	char pid[10], filename[50], port[20];
	sprintf(port, "%d", ListenPort);
	strcpy(filename, FS_PID_FILE );
	strcat(filename, port);
	strcat(filename, ".pid");
	if((fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644)) == -1)
	perror(filename); 
	ipid = getpid();
	sprintf(pid, "%d\n", ipid);
	write( fd, pid, strlen(pid));
        PIDfile = strdup(filename);
	/* now change the process uid for fontmanager signal communication  */
	setuid(2);
	}
}

/*
 * called when server cycles
 */
ResetSockets()
{
}

#ifdef SVR4
int
xlocal_accept(fd, from, fromlen)
  int fd;
  struct sockaddr *from;
  int *fromlen;
{
    int ret;
extern char * sys_errlist[];
#ifdef XLOCAL_NAMED
    if (fd == namedFd) return accept_named_local();
#endif
#if defined(TCPCONN) || defined(DNETCONN) || defined(XLOCAL_UNIX)
    errno = 0;
    ret = accept (fd, from, fromlen); 
    localStreamsConnection = FALSE;
    return(ret);
}
#define	accept		xlocal_accept


static int
accept_named_local()
{
     struct strrecvfd str; 
    if (ioctl(namedFd, I_RECVFD, &str) < 0) {
	fprintf(stderr, "accept_named_local(): I_RECVFD failed\n");
	return(-1);
    }
    localStreamsConnection = TRUE;
    return(str.fd);
}
#endif
#endif /* SVR4 */
/*
 * accepts new connections
 */
void
MakeNewConnections()
{
    long        readyconnections;
    int         curconn;
    int         newconn;
    long        connect_time;
    int         i;
    ClientPtr   client;
    OsCommPtr   oc;

#ifdef TCP_NODELAY
    union {
	struct sockaddr sa;

#ifdef TCPCONN
	struct sockaddr_in in;
#endif				/* TCPCONN */
    }           from;
    int         fromlen;

#endif				/* TCP_NODELAY */

#ifdef SVR4
    readyconnections = (LastSelectMask.fds_bits[0] & WellKnownConnections);
#else
    readyconnections = (LastSelectMask[0] & WellKnownConnections);
#endif /* SVR4 */
    if (!readyconnections)
	return;
    connect_time = GetTimeInMillis();

    /* kill off stragglers */
    for (i = MINCLIENT; i < currentMaxClients; i++) {
	if ((client = clients[i]) != NullClient) {
	    oc = (OsCommPtr) client->osPrivate;
	    if (oc && (oc->conn_time != 0) &&
		    (connect_time - oc->conn_time) >= TimeOutValue)
		CloseDownClient(client);
	}
    }

    while (readyconnections) {
	curconn = ffs(readyconnections) - 1;
	readyconnections &= ~(1 << curconn);
	if ((newconn = accept(curconn, (struct sockaddr *) NULL,
			      (int *) NULL)) < 0)
	    continue;

#ifdef TCP_NODELAY
    if( localStreamsConnection == FALSE ) {
        fromlen = sizeof(from);

	if (!getpeername(newconn, &from.in, &fromlen)) 
	    if (fromlen && (from.in.sin_family == AF_INET)) { 
		int         mi = 1;
	        if ( HasTrustedClients ) {
                   char** tmplist = TrustedClientListPtr;
		   struct hostent *client_mach;
	           int i;
                   Bool match = FALSE;
		   client_mach = gethostbyaddr(&from.in.sin_addr, 
			sizeof(from.in.sin_addr), AF_INET);
		   for( i = 0; i < NumberOfTrustedClients; i++ ) 
                     if(strcmp(client_mach->h_name,tmplist[i]) == 0){ 
			match = TRUE;
			break;
         	     } 
		   if ( match == TRUE ) {
		      setsockopt(newconn, IPPROTO_TCP, TCP_NODELAY,
			      (char *) &mi, sizeof(int));
		   } else { 
	              access_denied(newconn);
		      close(newconn);	
		      continue;
	           } 
            } else
		 setsockopt(newconn, IPPROTO_TCP, TCP_NODELAY,
		      (char *) &mi, sizeof(int));
	}
   }
#endif				/* TCP_NODELAY */

	/* ultrix reads hang on Unix sockets, hpux reads fail */

#if defined(O_NONBLOCK) && (!defined(ultrix) && !defined(hpux) && !defined(AIXV3))
	(void) fcntl(newconn, F_SETFL, O_NONBLOCK);
#else
#ifdef FIOSNBIO
	{
	    int         arg = 1;

	    ioctl(newconn, FIOSNBIO, &arg);
	}
#else
#if defined(AIXV3) && defined(FIONBIO)
	{
	    int arg;
	    arg = 1;
	    ioctl(newconn, FIONBIO, &arg);
	}
#else
	(void) fcntl(newconn, F_SETFL, FNDELAY);
#endif
#endif
#endif

	oc = (OsCommPtr) fsalloc(sizeof(OsCommRec));
	if (!oc) {
	    fsfree(oc);
	    error_conn_max(newconn);
	    close(newconn);
	    continue;
	}
	BITSET(AllClients, newconn);
	BITSET(AllSockets, newconn);
	oc->fd = newconn;
	oc->input = (ConnectionInputPtr) NULL;
	oc->output = (ConnectionOutputPtr) NULL;
	oc->conn_time = connect_time;

	if ((newconn < lastfdesc) &&
		(client = NextAvailableClient((pointer) oc))) {
	    ConnectionTranslation[newconn] = client->index;
	} else {
	    error_conn_max(newconn);
	    close_fd(oc);
	}
    }
}

#define	NOROOM	"Maximum number of clients reached"

static void
error_conn_max(fd)
    int         fd;
{
    fsConnSetup conn;
    char        pad[3];
    char        byteOrder = 0;
    int         whichbyte = 1;
    struct timeval waittime;
#ifdef SVR4
    fd_set	mask;
#else
    long        mask[mskcnt];
#endif /* SVR4 */


    waittime.tv_usec = BOTIMEOUT / MILLI_PER_SECOND;
    waittime.tv_usec = (BOTIMEOUT % MILLI_PER_SECOND) *
	(1000000 / MILLI_PER_SECOND);
#ifdef SVR4
    FD_ZERO(&mask);
    FD_SET(fd, &mask);
    (void) select(fd + 1, &mask, (fd_set *) NULL, (fd_set *) NULL, &waittime);
#else
    CLEARBITS(mask);
    BITSET(mask, fd);
    (void) select(fd + 1, (int *) mask, (int *) NULL, (int *) NULL, &waittime);
#endif /* SVR4 */
    /* try to read the byteorder of the connection */
    (void) read(fd, &byteOrder, 1);
    if ((byteOrder == 'l') || (byteOrder == 'B')) {
	int         num_alts;
	AlternateServerPtr altservers,
	            as;
	int         i,
	            altlen = 0;

	num_alts = ListAlternateServers(&altservers);
	conn.status = AuthDenied;
	conn.major_version = FS_PROTOCOL;
	conn.minor_version = FS_PROTOCOL_MINOR;
	conn.num_alternates = num_alts;
	for (i = 0, as = altservers; i < num_alts; i++, as++) {
	    altlen += (2 + as->namelen + 3) >> 2;
	}
	conn.alternate_len = altlen;
	/* blow off the auth info */
	conn.auth_index = 0;
	conn.auth_len = 0;

	if (((*(char *) &whichbyte) && (byteOrder == 'B')) ||
		(!(*(char *) &whichbyte) && (byteOrder == 'l'))) {
	    swaps(&conn.status, whichbyte);
	    swaps(&conn.major_version, whichbyte);
	    swaps(&conn.minor_version, whichbyte);
	    swaps(&conn.alternate_len, whichbyte);
	}
	(void) write(fd, (char *) &conn, sizeof(fsConnSetup));
	/* dump alternates */
	for (i = 0, as = altservers; i < num_alts; i++, as++) {
	    (void) write(fd, (char *) as, 2);	/* XXX */
	    (void) write(fd, (char *) as->name, as->namelen);
	    altlen = 2 + as->namelen;
	    /* pad it */
	    if (altlen & 3)
		(void) write(fd, (char *) pad, ((4 - (altlen & 3)) & 3));
	}
	if (num_alts)
	    fsfree((char *) altservers);
    }
}

static void
close_fd(oc)
    OsCommPtr   oc;
{
    int         fd = oc->fd;

    close(fd);
    FreeOsBuffers(oc);
    BITCLEAR(AllSockets, fd);
    BITCLEAR(AllClients, fd);
    BITCLEAR(ClientsWithInput, fd);
    BITCLEAR(ClientsWriteBlocked, fd);
    if (!ANYSET(ClientsWriteBlocked))
	AnyClientsWriteBlocked = FALSE;
    BITCLEAR(OutputPending, fd);
    fsfree(oc);
}

CheckConnections()
{
    long        mask[mskcnt];
#ifdef SVR4
    fd_set      tmask;
#else
    long        tmask[mskcnt];
#endif
    int         curclient;
    int         i;
    struct timeval notime;
    int         r;

    notime.tv_sec = 0;
    notime.tv_usec = 0;

    COPYBITS(AllClients, mask);
    for (i = 0; i < mskcnt; i++) {
	while (mask[i]) {
	    curclient = ffs(mask[i]) - 1 + (i << 5);
#ifdef SVR4
	    FD_ZERO(&tmask);
	    FD_SET(curclient, &tmask);
	    r = select(curclient + 1, &tmask, (fd_set *) NULL,
		       (fd_set *) NULL, &notime);
#else
	    CLEARBITS(tmask);
	    BITSET(tmask, curclient);
	    r = select(curclient + 1, (int *) tmask, (int *) NULL,
		       (int *) NULL, &notime);
#endif /* SVR4 */
	    if (r < 0)
		CloseDownClient(clients[ConnectionTranslation[curclient]]);
	    BITCLEAR(mask, curclient);
	}
    }
}

CloseDownConnection(client)
    ClientPtr   client;
{
    OsCommPtr   oc = (OsCommPtr) client->osPrivate;
    if (oc->output && oc->output->count)
	FlushClient(client, oc, (char *) NULL, 0, 0);
    ConnectionTranslation[oc->fd] = 0;
    close_fd(oc);
    client->osPrivate = (pointer) NULL;
}


/****************
 * IgnoreClient
 *    Removes one client from input masks.
 *    Must have cooresponding call to AttendClient.
 ****************/

static long IgnoredClientsWithInput[mskcnt];

IgnoreClient(client)
    ClientPtr   client;
{
    OsCommPtr   oc = (OsCommPtr) client->osPrivate;
    int         connection = oc->fd;

    if (GETBIT(ClientsWithInput, connection))
	BITSET(IgnoredClientsWithInput, connection);
    else
	BITCLEAR(IgnoredClientsWithInput, connection);
    BITCLEAR(ClientsWithInput, connection);
    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
#ifdef SVR4
    FD_CLR(connection, &LastSelectMask);
#else
    BITCLEAR(LastSelectMask, connection);
#endif /* SVR4 */
    isItTimeToYield = TRUE;
}

/****************
 * AttendClient
 *    Adds one client back into the input masks.
 ****************/

AttendClient(client)
    ClientPtr   client;
{
    OsCommPtr   oc = (OsCommPtr) client->osPrivate;
    int         connection = oc->fd;

    BITSET(AllClients, connection);
    BITSET(AllSockets, connection);
#ifdef SVR4
    FD_SET(connection, &LastSelectMask);
#else
    BITSET(LastSelectMask, connection);
#endif /* SVR4 */
    if (GETBIT(IgnoredClientsWithInput, connection))
	BITSET(ClientsWithInput, connection);
}

/*
 * figure out which clients need to be toasted
 */
ReapAnyOldClients()
{
    int         i;
    long        cur_time = GetTimeInMillis();
    ClientPtr   client;
    extern void SendKeepAliveEvent();

#ifdef DEBUG
    fprintf(stderr, "Looking for clients to reap\n");
#endif

    for (i = MINCLIENT; i < currentMaxClients; i++) {
	client = clients[i];
	if (client) {
	    if ((cur_time - client->last_request_time) >= ReapClientTime) {
		if (client->clientGone == CLIENT_AGED) {
		    client->clientGone = CLIENT_TIMED_OUT;

#ifdef DEBUG
		    fprintf(stderr, "reaping client #%d\n", i);
#endif

		    CloseDownClient(client);
		} else {
		    client->clientGone = CLIENT_AGED;
		    SendKeepAliveEvent(client);
		}
	    }
	}
    }
}
static int
access_denied(fd)
    int         fd;
{
    fsConnSetup conn;
    char        pad[3];
    char        byteOrder = 0;
    int         whichbyte = 1;
    struct timeval waittime;
    /* try to read the byteorder of the connection */
    (void) read(fd, &byteOrder, 1);
    if ((byteOrder == 'l') || (byteOrder == 'B')) {
	int         i,
	            altlen = 0;

	conn.status = AuthDenied;
	conn.major_version = FS_PROTOCOL;
	conn.minor_version = FS_PROTOCOL_MINOR;
	conn.num_alternates = 0;
	conn.alternate_len = 0;
	/* blow off the auth info */
	conn.auth_index = 0;
	conn.auth_len = 0;

	if (((*(char *) &whichbyte) && (byteOrder == 'B')) ||
		(!(*(char *) &whichbyte) && (byteOrder == 'l'))) {
	    swaps(&conn.status, whichbyte);
	    swaps(&conn.major_version, whichbyte);
	    swaps(&conn.minor_version, whichbyte);
	    swaps(&conn.alternate_len, whichbyte);
	}
	(void) write(fd, (char *) &conn, sizeof(fsConnSetup));
    }
}
