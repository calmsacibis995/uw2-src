/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:os/connection.c	1.36"
/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: connection.c,v 1.146 92/06/11 10:38:45 rws Exp $ */

/*
 * Copyright (c) 1993 Unix System Laboratories (USL)
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of USL not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  USL makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * USL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL USL
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*=========================================================================*\
|| Copyright (c) 1993 Pittsburgh Powercomputing Corporation (PPc).
|| Copyright (c) 1993 Quarterdeck Office Systems, Inc. (QOS).
||
|| Permission to use, copy, modify, and distribute this software and its
|| documentation for any purpose and without fee is hereby granted, provided
|| that the above copyright notice appear in all copies and that both that
|| copyright notice and this permission notice appear in supporting
|| documentation, and that the names of PPc and QOS not be used in
|| advertising or publicity pertaining to distribution of the software
|| without specific, written prior permission.  Neither PPc nor QOS
|| make any representations about the suitability of this software for any
|| purpose.  It is provided "as is" without express or implied warranty.
||
|| PPc AND QOS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
|| INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
|| EVENT SHALL PPc OR QOS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
|| CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
|| USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
|| OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
|| PERFORMANCE OF THIS SOFTWARE.
\*=========================================================================*/
/***************************************************************** *  Stuff to create connections --- OS dependent
 *
 *      EstablishNewConnections, CreateWellKnownSockets, ResetWellKnownSockets,
 *      CloseDownConnection, CheckConnections, AddEnabledDevice,
 *	RemoveEnabledDevice, OnlyListToOneClient,
 *      ListenToAllClients,
 *
 *      (WaitForSomething is in its own file)
 *
 *      In this implementation, a client socket table is not kept.
 *      Instead, what would be the index into the table is just the
 *      file descriptor of the socket.  This won't work for if the
 *      socket ids aren't small nums (0 - 2^8)
 *
 *****************************************************************/

#include "X.h"
#include "Xproto.h"
#include <sys/param.h>
#include <errno.h>
#include "Xos.h"			/* for strings, file, time */
#ifdef ESIX
#include <lan/socket.h>
#else
#include <sys/socket.h>
#endif

#include <signal.h>
#include <setjmp.h>

#ifdef hpux
#include <sys/utsname.h>
#include <sys/ioctl.h>
#endif

#ifdef SVR4
#include <sys/resource.h>
#include <sys/select.h>
#endif

#ifdef AIXV3
#include <sys/ioctl.h>
#endif

#ifdef TCPCONN
# include <netinet/in.h>
# ifndef hpux
#  ifdef apollo
#   ifndef NO_TCP_H
#    include <netinet/tcp.h>
#   endif
#  else
#   include <netinet/tcp.h>
#  endif
# endif
#endif

#if defined(SO_DONTLINGER) && defined(SO_LINGER)
#undef SO_DONTLINGER
#endif

#ifdef UNIXCONN
/*
 * sites should be careful to have separate /tmp directories for diskless nodes
 */
#include <sys/un.h>
#include <sys/stat.h>
static int unixDomainConnection = -1;
#endif

#include <stdio.h>
#include <sys/uio.h>
#include "osstruct.h"
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"

#ifdef DNETCONN
#include <netdnet/dn.h>
#endif /* DNETCONN */

#ifdef SIGNALRETURNSINT
#define SIGVAL int
#else
#define SIGVAL void
#endif

#ifdef NOVELL
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#endif /* NOVELL */

typedef long CCID;      /* mask of indices into client socket table */

#ifndef X_UNIX_PATH
#ifdef hpux
#define X_UNIX_DIR	"/usr/spool/sockets/X11"
#define X_UNIX_PATH	"/usr/spool/sockets/X11/"
#define OLD_UNIX_DIR	"/tmp/.X11-unix"
#else
#define X_UNIX_DIR	"/tmp/.X11-unix"
#define X_UNIX_PATH	"/tmp/.X11-unix/X"
#endif
#endif

extern char *display;		/* The display number */
int lastfdesc;			/* maximum file descriptor */

long WellKnownConnections;	/* Listener mask */
#ifdef SVR4
fd_set EnabledDevices;  /* mask for input devices that are on */
fd_set AllSockets;      /* select on this */
fd_set AllClients;      /* available clients */
fd_set LastSelectMask;  /* mask returned from last select call */
fd_set ClientsWithInput;        /* clients with FULL requests in buffer */
fd_set ClientsWriteBlocked;/* clients who cannot receive output */
fd_set OutputPending;   /* clients with reply/event data ready to go */
#else
long EnabledDevices[mskcnt];    /* mask for input devices that are on */
long AllSockets[mskcnt];        /* select on this */
long AllClients[mskcnt];        /* available clients */
long LastSelectMask[mskcnt];    /* mask returned from last select call */
long ClientsWithInput[mskcnt];  /* clients with FULL requests in buffer */
long ClientsWriteBlocked[mskcnt];/* clients who cannot receive output */
long OutputPending[mskcnt];     /* clients with reply/event data ready to go */
#endif

long MaxClients = MAXSOCKS ;
long NConnBitArrays = mskcnt;
Bool NewOutputPending;		/* not yet attempted to write some new output */
Bool AnyClientsWriteBlocked;	/* true if some client blocked on write */

Bool RunFromSmartParent;	/* send SIGUSR1 to parent process */
Bool PartialNetwork = TRUE;	/* continue even if unable to bind all addrs */
static int ParentProcess;

static Bool debug_conns = FALSE;

#ifdef SVR4
static fd_set IgnoredClientsWithInput;
static fd_set SavedAllClients;
static fd_set SavedAllSockets;
static fd_set SavedClientsWithInput;
#else
static long IgnoredClientsWithInput[mskcnt];
static long SavedAllClients[mskcnt];
static long SavedAllSockets[mskcnt];
static long SavedClientsWithInput[mskcnt];
#endif

int GrabInProgress = 0;

int ConnectionTranslation[MAXSOCKS];
extern ClientPtr NextAvailableClient();

extern SIGVAL AutoResetServer();
extern SIGVAL GiveUp();
extern XID CheckAuthorization();
static void CloseDownFileDescriptor(), ErrorConnMax();
extern void FreeOsBuffers(), ResetOsBuffers();

#ifdef XDMCP
void XdmcpOpenDisplay(), XdmcpInit(), XdmcpReset(), XdmcpCloseDisplay();
#endif

char * getenv();
#include <string.h>

#ifdef TCPCONN
static int
open_tcp_socket ()
{
    struct sockaddr_in insock;
    int request;
    int retry;
#ifdef SVR4
#undef SO_DONTLINGER
#endif
#ifndef SO_DONTLINGER
#ifdef SO_LINGER
    static int linger[2] = { 0, 0 };
#endif /* SO_LINGER */
#endif /* SO_DONTLINGER */

#ifdef AIXV3
#ifndef FORCE_DISPLAY_NUM
    extern int AIXTCPSocket;
    if (AIXTCPSocket>=0) {
        request= AIXTCPSocket;
    } else
#endif /* FORCE_DISPLAY_NUM */
#endif /* AIX && etc. */
    if ((request = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
    {
#ifdef DEBUG
	Error ("Creating TCP socket");
#endif
	return -1;
    } 
#ifdef SO_REUSEADDR
    /* Necesary to restart the server without a reboot */
    {
	int one = 1;
	setsockopt(request, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof(int));
    }
#endif /* SO_REUSEADDR */
#ifdef AIXV3
#ifndef FORCE_DISPLAY_NUMBER
    if (AIXTCPSocket<0)
#endif
#endif
    {
    bzero ((char *)&insock, sizeof (insock));
#ifdef BSD44SOCKETS
    insock.sin_len = sizeof(insock);
#endif
    insock.sin_family = AF_INET;
    insock.sin_port = htons ((unsigned short)(X_TCP_PORT + atoi (display)));
    insock.sin_addr.s_addr = htonl(INADDR_ANY);
    retry = 20;
    while (bind(request, (struct sockaddr *)&insock, sizeof (insock)))
    {
	if (--retry == 0) {
	    Error ("Binding TCP socket");
	    close (request);
	    return -1;
	}
#ifdef SO_REUSEADDR
	sleep (1);
#else
	sleep (10);
#endif /* SO_REUSEDADDR */
    }
    }
#ifdef SO_DONTLINGER
    if(setsockopt (request, SOL_SOCKET, SO_DONTLINGER, (char *)NULL, 0))
	Error ("Setting TCP SO_DONTLINGER");
#else
#ifdef SO_LINGER
    if(setsockopt (request, SOL_SOCKET, SO_LINGER,
		   (char *)linger, sizeof(linger)))
	Error ("Setting TCP SO_LINGER");
#endif /* SO_LINGER */
#endif /* SO_DONTLINGER */
    if (listen (request, 5)) {
	Error ("TCP Listening");
	close (request);
	return -1;
    }

    return request;
}
#endif /* TCPCONN */

#if defined(UNIXCONN) && !defined(SERVER_LOCALCONN)
static struct sockaddr_un unsock;

static int
open_unix_socket ()
{
    int oldUmask;
    int request;

    bzero ((char *) &unsock, sizeof (unsock));
    unsock.sun_family = AF_UNIX;
    oldUmask = umask (0);
#ifdef X_UNIX_DIR
    if (!mkdir (X_UNIX_DIR, 0777))
	chmod (X_UNIX_DIR, 0777);
#endif
    strcpy (unsock.sun_path, X_UNIX_PATH);
    strcat (unsock.sun_path, display);
#ifdef BSD44SOCKETS
    unsock.sun_len = strlen(unsock.sun_path);
#endif
#ifdef hpux
    {  
        /*    The following is for backwards compatibility
         *    with old HP clients. This old scheme predates the use
 	 *    of the /usr/spool/sockets directory, and uses hostname:display
 	 *    in the /tmp/.X11-unix directory
         */
        struct utsname systemName;
	static char oldLinkName[256];

        uname(&systemName);
        strcpy(oldLinkName, OLD_UNIX_DIR);
        if (!mkdir(oldLinkName, 0777))
	    chown(oldLinkName, 2, 3);
        strcat(oldLinkName, "/");
        strcat(oldLinkName, systemName.nodename);
        strcat(oldLinkName, display);
        unlink(oldLinkName);
        symlink(unsock.sun_path, oldLinkName);
    }
#endif	/* hpux */
    unlink (unsock.sun_path);
    if ((request = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
	Error ("Creating Unix socket");
	return -1;
    } 
#ifdef BSD44SOCKETS
    if (bind(request, (struct sockaddr *)&unsock, SUN_LEN(&unsock)))
#else
    if (bind(request, (struct sockaddr *)&unsock, strlen(unsock.sun_path)+2))
#endif
    {
	Error ("Binding Unix socket");
	close (request);
	return -1;
    }
    if (listen (request, 5))
    {
	Error ("Unix Listening");
	close (request);
	return -1;
    }
    (void)umask(oldUmask);
    return request;
}
#endif /*UNIXCONN */

#ifdef SERVER_LOCALCONN
/*=========================================================================*\
|| The following is an implementation of the X-server-half of a variety
|| of SYSV386 local connection methods.  This includes compatability
|| with ISC STREAMS, SCO STREAMS, ATT pts connections, ATT/USL named-pipes,
|| and SVR4.2 UNIX DOMAIN connections.  To enable the output of various
|| connection-related information messages to stderr, set the environment
|| variable XLOCAL_VERBOSE.
||	XLOCAL_VERBOSE	= 0	print connection availability
||			= 1	also, print message for each connection
||			= 2	also, print cleanup information
||			= 3	even more...
||
|| See note below about the conflict between ISC and UNIX nodes.
\*=========================================================================*/

#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/utsname.h>
#ifndef UNIXCONN
#include <sys/stat.h>
#endif

#define X_STREAMS_DIR	"/dev/X"

#ifdef UNIXCONN
#define XLOCAL_UNIX
#define X_UNIX_DEVDIR	"/dev/X/UNIXCON"
#define X_UNIX_DEVPATH	"/dev/X/UNIXCON/X"
#endif

#if !defined(SVR4) || defined(SVR4_ACP)
#define XLOCAL_ISC
#define XLOCAL_SCO
#define X_ISC_DIR	"/dev/X/ISCCONN"
#define X_ISC_PATH	"/dev/X/ISCCONN/X"
#define X_SCO_PATH	"/dev/X"
#define DEV_SPX		"/dev/spx"
static int iscFd = -1;
static int scoFd = -1;
#endif

#ifdef unix
#define XLOCAL_PTS
#define X_PTS_PATH	"/dev/X/server."
#define DEV_PTMX	"/dev/ptmx"
extern char *ptsname();
static int ptsFd = -1;
#endif

#if defined(SVR4)
#define XLOCAL_NAMED
int namedFd = -1;
#define X_NAMED_PATH	"/dev/X/Nserver."
#endif

#ifdef SVR4
/* BUILTIN static */ fd_set AllStreams;
#else
static long AllStreams[mskcnt]; /* bitmap of STREAMS file descriptors */
#endif
#ifdef NOVELL
struct netconfig   *netconfigp;
struct t_bind tbind_ret;
#define BIND_BUFSIZE 50
char * spx_host;
int netWareFd = -1;
fd_set AllTLIConnections;
extern int t_errno;
#endif /* NOVELL */


/* define NO_XLOCAL_VERBOSE to remove the XLOCAL_VERBOSE functionality */
#ifdef NO_XLOCAL_VERBOSE
#define XLOCAL_MSG(x)	/* as nothing */
#else
static void xlocalMsg();
#define XLOCAL_MSG(x)	xlocalMsg x;
#endif

static int useSlashTmpForUNIX=0;

#ifndef NO_XLOCAL_VERBOSE
/*PRINTFLIKE1*/
static void
xlocalMsg(lvl,str,a,b,c,d,e,f,g,h)
  int lvl;
  char *str;
  int a,b,c,d,e,f,g,h;
{
    static int xlocalMsgLvl = -2;

    if (xlocalMsgLvl == -2) {
	char *tmp;
	if ((tmp = getenv("XLOCAL_VERBOSE")) != NULL) {
	    xlocalMsgLvl = atoi(tmp);
	} else {
	    xlocalMsgLvl = -1; 
	}
    }
    if (xlocalMsgLvl >= lvl) {
	fprintf(stderr,"X: XLOCAL - ");
	fprintf(stderr,str,a,b,c,d,e,f,g,h);
    }
}
#endif /* NO_XLOCAL_MSG */

/*==========================================================================*\
|| We have a conflict between ISC and UNIX connections over the use
|| of the /tmp/.X11-unix/Xn path.  Therefore, whichever connection type
|| is specified first in the XLOCAL environment variable gets to use this
|| path for its own device nodes.  The default is ISC.
||
|| Note that both connection types are always available using their
|| alternate paths at /dev/X/ISCCONN/Xn and /dev/X/UNIXCON/Xn respectively.
||
|| To make an older client or library use these alternate paths, you
|| need to edit the binary and replace /tmp/.X11-unix with either
|| /dev/X/ISCCONN or /dev/X/UNIXCON depending on its preference.
||
|| Why not use the same path as UNIXCONN for all connection types ?? 
|| Diskless workstations may have a common /tmp directory.  As you can
|| imagine, this can cause serious problems.  Since every workstation
|| MUST have it's own /dev directory, we will use the directory /dev/X
|| for as many of our nodes as possible.  All future X-libraries are
|| encouraged to look for and use the alternalte (/dev/X/...) path first.
\*==========================================================================*/
#define WHITE	" :\t\n\r"

static void
ChooseLocalConnectionType()
{
    char *name,*nameList;

    XLOCAL_MSG((3,"Choosing ISC vs. UNIXDOMAIN connections...\n"));

    useSlashTmpForUNIX=0;

    if ((nameList = getenv("XLOCAL")) != NULL) {
	nameList = xstrdup(nameList);
	name = strtok(nameList,WHITE);
	while (name) {
	    if (!strncmp(name,"SP",2) ||
		!strncmp(name,"ISC",3) ||
		!strncmp(name,"STREAMS",7)) {
		break;
	    } else if (!strncmp(name,"UNIX",4)) {
		useSlashTmpForUNIX=1;
		break;
	    }
	    name = strtok(NULL,WHITE);
	}
	xfree(nameList); 
    } else {
	XLOCAL_MSG((3,"XLOCAL not set in environment.\n"));
    }

    XLOCAL_MSG((3,"Using %s for local connections in /tmp/.X11-unix.\n",
      useSlashTmpForUNIX ? "UNIXCONN" : "ISC"));
}
#undef WHITE

int
xlocal_unlink(path)
  char *path;
{
    int ret;

    ret = unlink(path);
    if (ret == -1 && errno == EINTR)
      ret = unlink(path);
    if (ret == -1 && errno == ENOENT)
      ret = 0;
    return(ret);
}

/*=========================================================================*/
#ifdef XLOCAL_UNIX
static struct sockaddr_un unsock;

/* UNIX: UNIX domain sockets as used in SVR4.2 */
static int
open_unix_local()
{
    int oldUmask;
    int request;

    bzero((char *) &unsock, sizeof (unsock));
    unsock.sun_family = AF_UNIX;
    oldUmask = umask (0);

    mkdir(X_STREAMS_DIR, 0777); /* "/dev/X" */
    chmod(X_STREAMS_DIR, 0777);
    if (!mkdir(X_UNIX_DEVDIR, 0777))
      chmod(X_UNIX_DEVDIR, 0777);
    strcpy(unsock.sun_path, X_UNIX_DEVPATH);
    strcat(unsock.sun_path, display);
    xlocal_unlink(unsock.sun_path);
    if ((request = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
	Error("Creating Unix socket");
	return -1;
    } 
    if (bind(request, (struct sockaddr *)&unsock, strlen(unsock.sun_path)+2)) {
	Error("Binding Unix socket");
	close(request);
	return -1;
    }
    if (listen(request, 5)) {
	Error("Unix Listening");
	close(request);
	return -1;
    }
    XLOCAL_MSG((0,"UNIX connections available at [%s]\n", unsock.sun_path));

    if (useSlashTmpForUNIX) {
	char tmpPath[64];
	if (!mkdir(X_UNIX_DIR, 0777))
	  chmod(X_UNIX_DIR, 0777);
	strcpy(tmpPath, X_UNIX_PATH);
	strcat(tmpPath, display);
	xlocal_unlink(tmpPath);
	if (symlink(unsock.sun_path,tmpPath) == 0) {
	    XLOCAL_MSG((0,"UNIX connections available at [%s]\n", tmpPath));
	}
    }
    (void)umask(oldUmask);
    return request;
}

void
close_unix_local()
{
    char path[64];

    if (unixDomainConnection != -1) {
	close(unixDomainConnection);
	WellKnownConnections &= ~(1L << unixDomainConnection);
	unixDomainConnection = -1;
    }
    strcpy(path, X_UNIX_DEVPATH);
    strcat(path, display);
    XLOCAL_MSG((2,"removing [%s] and [%s]\n",path,X_UNIX_DEVDIR)); 
    xlocal_unlink(path);
    xlocal_unlink(X_UNIX_DEVDIR);
    if (useSlashTmpForUNIX) {
	strcpy(path, X_UNIX_PATH);
	strcat(path, display);
	XLOCAL_MSG((2,"removing [%s] and [%s]\n",path,X_UNIX_DIR)); 
	xlocal_unlink(path);
	xlocal_unlink(X_UNIX_DIR);
    }
}

void
reset_unix_local()
{
    char path[64];
    struct stat statb;
    int need_reset = 0;

    if (unixDomainConnection != -1) {
	strcpy(path, X_UNIX_DEVPATH);
	strcat(path, display);
	if ((stat(path, &statb) == -1) ||
	    (statb.st_mode & S_IFMT) != S_IFSOCK)
	  need_reset = 1;

	if (useSlashTmpForUNIX) {
	    strcpy(path, X_UNIX_PATH);
	    strcat(path, display);
	    if ((stat(path, &statb) == -1) ||
		(statb.st_mode & S_IFMT) != S_IFSOCK)
	      need_reset = 1;
	}

	if (need_reset) {
	    close_unix_local();
	    unixDomainConnection = open_unix_local();
	    if (unixDomainConnection != -1)
	      WellKnownConnections |= (1L << unixDomainConnection);
	}
    }
}

#endif /* XLOCAL_UNIX */

/*=========================================================================*/
#if defined(XLOCAL_ISC) || defined(XLOCAL_SCO)
static int
connect_spipe(fd1, fd2)
  int fd1, fd2;
{
    long temp;
    struct strfdinsert sbuf;

    sbuf.databuf.maxlen = -1;
    sbuf.databuf.len = -1;
    sbuf.databuf.buf = NULL;
    sbuf.ctlbuf.maxlen = sizeof(long);
    sbuf.ctlbuf.len = sizeof(long);
    sbuf.ctlbuf.buf = (caddr_t)&temp;
    sbuf.offset = 0;
    sbuf.fildes = fd2;
    sbuf.flags = 0;
    if (ioctl(fd1, I_FDINSERT, &sbuf) == -1) return (-1);
  
    return(0);
}

static int
named_spipe(fd, path)
  int fd;
  char *path;
{
    int oldUmask, ret;
    struct stat sbuf;

    oldUmask = umask(0);

    (void) fstat(fd, &sbuf);
    ret = mknod(path, 0020666, sbuf.st_rdev);

    umask(oldUmask);

    if (ret < 0) {
	ret = -1;
    } else {
	ret = fd;
    }
    return(ret);
}
#endif /* defined(XLOCAL_ISC) || defined(XLOCAL_SCO) */

/*=========================================================================*/
#ifdef XLOCAL_ISC
/*
|| ISC: ISC UNIX V.3.2 compatible streams at /dev/X/ISCCONN/Xn.
|| It will link this to /tmp/.X11-unix/Xn if there are no UNIXDOMAIN
|| connection nodes required.
*/
static int
open_isc_local()
{
    int fd = -1,fds = -1;
    char pathISC[64],pathX11[64];

    mkdir(X_STREAMS_DIR, 0777); /* "/dev/X" */
    chmod(X_STREAMS_DIR, 0777);
    mkdir(X_ISC_DIR, 0777); /* "/dev/X/ISCCONN" */
    chmod(X_ISC_DIR, 0777);

    strcpy(pathISC, X_ISC_PATH);
    strcat(pathISC, display);
  
    if (xlocal_unlink(pathISC) < 0) {
	ErrorF("open_isc_local(): Can't unlink node (%s),\n", pathISC);
	return(-1);
    }

    if ((fds = open(DEV_SPX, O_RDWR)) >= 0 &&
	(fd  = open(DEV_SPX, O_RDWR)) >= 0 ) {

	if (connect_spipe(fds, fd) != -1 &&
	    named_spipe(fds, pathISC) != -1) {

	    XLOCAL_MSG((0,"ISC connections available at [%s]\n", pathISC));

	    if (!useSlashTmpForUNIX) {
		mkdir(X_UNIX_DIR, 0777);
		chmod(X_UNIX_DIR, 0777);
		strcpy(pathX11, X_UNIX_PATH);
		strcat(pathX11, display);
		if (xlocal_unlink(pathX11) < 0) {
		    /*EMPTY*/
		    /* can't get the /tmp node, just return the good one...*/
		} else {
#ifdef SVR4
		    /* we prefer symbolic links as hard links can't
		       cross filesystems */
		    if (symlink(pathISC,pathX11) == 0) {
			XLOCAL_MSG((0,"ISC connections available at [%s]\n",
				    pathX11));
		    }
#else
		    if (symlink(pathISC,pathX11) == 0) {
			XLOCAL_MSG((0,"ISC connections available at [%s]\n",
				    pathX11));
		    }
		      
#endif
		}
	    }
	    return(fd);
	} else {
	    Error("open_isc_local(): Can't set up listener pipes");
	}
    } else {
	XLOCAL_MSG((0,"open_isc_local(): can't open %s\n",DEV_SPX));
#ifndef SVR4
	/*
	 * At this point, most SVR4 versions will fail on this, so leave out the
	 * warning
	 */
	Error("open_isc_local(): can't open \"%s\"",DEV_SPX);
	return(-1);
#endif
    }

    (void) close(fds);
    (void) close(fd);
    return(-1);
}

static int
accept_isc_local()
{
    struct strrecvfd buf;

    while (ioctl(iscFd, I_RECVFD, &buf) < 0)
      if (errno != EAGAIN) {
	  Error("accept_isc_local(): Can't read fildes");
	  return(-1);
      }

    XLOCAL_MSG((1,"new ISC connection accepted (%d)\n",buf.fd));
#ifdef SVR4
    FD_SET(buf.fd, &AllStreams);
#else
    BITSET(AllStreams, buf.fd);
#endif
    return(buf.fd);
}

void
close_isc_local()
{
    char path[64];

    if (iscFd != -1) {
	close(iscFd);
	WellKnownConnections &= ~(1L << iscFd);
	iscFd = -1;
    }
    strcpy(path, X_ISC_PATH);
    strcat(path, display);
    XLOCAL_MSG((2,"removing [%s] and [%s]\n",path,X_ISC_DIR)); 
    xlocal_unlink(path);
    xlocal_unlink(X_ISC_DIR);
    if (!useSlashTmpForUNIX) {
	strcpy(path, X_UNIX_PATH);
	strcat(path, display);
	XLOCAL_MSG((2,"removing [%s] and [%s]\n",path,X_UNIX_DIR)); 
	xlocal_unlink(path);
	xlocal_unlink(X_UNIX_DIR);
    }
}
#endif /* XLOCAL_ISC */

/*=========================================================================*/
#ifdef XLOCAL_SCO
/* SCO: SCO/XSIGHT style using /dev/spx */
static int
open_sco_local()
{
    int fds = -1,fdr = -1;
    char pathS[64], pathR[64];
    struct stat bufS, bufR;
    Bool sco_nodes_exist = FALSE;

    sprintf(pathS, "%s%sS",X_SCO_PATH, display);
    sprintf(pathR, "%s%sR",X_SCO_PATH, display);
    if(!stat(pathS, &bufS) && !stat(pathS, &bufR)) {
        if( (bufS.st_mode&S_IFCHR) && (bufR.st_mode&S_IFCHR) ) 
             sco_nodes_exist = TRUE ; 
        else if ((xlocal_unlink(pathS) < 0) || (xlocal_unlink(pathR) < 0)) {
	     ErrorF("open_sco_local(): can't unlink node (%s)\n",pathR);
	     return(-1);
        }
    }
  
    if ((fds = open(DEV_SPX, O_RDWR)) >= 0 &&
	(fdr = open(DEV_SPX, O_RDWR)) >= 0 ) {

	if ((connect_spipe(fds, fdr) != -1 && sco_nodes_exist) ||
	    (named_spipe(fds, pathS) != -1 &&
	     named_spipe(fdr, pathR) != -1)) {
      
	    XLOCAL_MSG((0,"SCO connections available at [%s]\n",pathR));

	    return(fds);
	} else {
	    Error("open_sco_local(): can't set up listener pipes");
	}  
    } else {
	XLOCAL_MSG((0,"open_sco_local(): can't open %s",DEV_SPX));
#ifndef SVR4
	/*
	 * At this point, most SVR4 versions will fail on this, so
	 * leave out the warning
	 */
	Error("open_sco_local(): can't open \"%s\"",DEV_SPX);
	return(-1);
#endif
    }

    (void) close(fds);
    (void) close(fdr);
    return(-1);
}


static int
accept_sco_local()
{
    char c;
    int fd;

    if (read(scoFd, &c, 1) < 0) {
	Error("accept_sco_local(): can't read from client");
	return(-1);
    }

    if ((fd = open(DEV_SPX, O_RDWR)) < 0) {
	Error("accept_sco_local(): can't open \"%s\"",DEV_SPX);
	return(-1);
    }

    if (connect_spipe(scoFd, fd) < 0) {
	Error("accept_sco_local(): can't connect pipes");
	(void) close(fd);
	return(-1);
    }

    XLOCAL_MSG((1,"new SCO connection accepted (%d)\n",fd));
#ifdef SVR4
  FD_SET(fd, &AllStreams);
#else
  BITSET(AllStreams, fd);
#endif
    return(fd);
}

static void
close_sco_local()
{
    if (scoFd != -1) {
	close(scoFd);
	WellKnownConnections &= ~(1L << scoFd);
	scoFd = -1;
    }
}

#endif /* XLOCAL_SCO */

/*=========================================================================*/
#ifdef XLOCAL_PTS
/* PTS: AT&T style using /dev/ptmx */
static int
open_pts_local()
{
    char *slave;
    int fd;
    char path[64];

    mkdir(X_STREAMS_DIR, 0777);
    chmod(X_STREAMS_DIR, 0777);
  
    strcpy(path, X_PTS_PATH);
    strcat(path, display);
  
    if (open(path, O_RDWR) >= 0 || (xlocal_unlink(path) < 0)) {
	ErrorF("open_pts_local(): server is already running (%s)\n", path);
	return(-1);
    }
  
    if ((fd = open(DEV_PTMX, O_RDWR)) < 0) {
	Error("open_pts_local(): can't open \"%s\"",DEV_PTMX);
	return(-1);
    }
  
    grantpt(fd);
    unlockpt(fd);
    slave = ptsname(fd);
    if (symlink(slave, path) < 0 || chmod(path, 0666) < 0) {
	Error("open_pts_local(): can't set up local listener");
	return(-1);
    }

    if (open(path, O_RDWR) < 0) {
	ErrorF("open_pts_local(): can't open %s\n", path);
	close(fd);
	return(-1);
    }
    XLOCAL_MSG((0,"PTS connections available at [%s]\n",path));

    return(fd);
}

static int
accept_pts_local()
{
    int newconn;
    char length;
    char path[64];

    /*
     * first get device-name
     */
    if(read(ptsFd, &length, 1) <= 0 ) {
	Error("accept_pts_local(): can't read slave name length");
	return(-1);
    }

    if(read(ptsFd, path, length) <= 0 ) {
	Error("accept_pts_local(): can't read slave name");
	return(-1);
    }

    path[ length ] = '\0';
      
    if((newconn = open(path,O_RDWR)) < 0) {
	Error("accept_pts_local(): can't open slave");
	return(-1);
    }

    (void) write(newconn, "1", 1); /* send an acknowledge to the client */

    XLOCAL_MSG((1,"new PTS connection accepted (%d)\n",newconn));
#ifdef SVR4
  FD_SET(newconn, &AllStreams);
#else
  BITSET(AllStreams, newconn);
#endif

    return(newconn);
}

void
close_pts_local()
{
    char path[64];

    if (ptsFd != -1) {
	close(ptsFd);
	WellKnownConnections &= ~(1L << ptsFd);
	ptsFd = -1;
    }
    strcpy(path, X_PTS_PATH);
    strcat(path, display);
    XLOCAL_MSG((2,"removing [%s]\n",path)); 
    xlocal_unlink(path);
}
#endif /* XLOCAL_PTS */

/*=========================================================================*/
#ifdef XLOCAL_NAMED
/* NAMED: USL style using bi-directional named pipes */
static int
open_named_local()
{
    int fd,fld[2];
    char path[64];
    struct stat sbuf;

    mkdir(X_STREAMS_DIR, 0777);
    chmod(X_STREAMS_DIR, 0777);
  
    strcpy(path, X_NAMED_PATH);
    strcat(path, display);
  
    if (stat(path, &sbuf) != 0) {
	if (errno == ENOENT) {
	    if ((fd = creat(path, (mode_t)0666)) == -1) {
		ErrorF("open_named_local(): can't create %s\n",path);
		return(-1);
	    }
	    close(fd);
	    if (chmod(path, (mode_t)0666) < 0) {
		ErrorF("open_named_local(): can't chmod %s\n",path);
		return(-1);
	    }
	} else {
	    ErrorF("open_named_local(): unknown stat error, %d\n",errno);	
	    return(-1);
	}
    }

    if (pipe(fld) != 0) {
	ErrorF("open_named_local(): pipe failed, errno=%d\n",errno);
	return(-1);
    }

    if (ioctl(fld[0], I_PUSH, "connld") != 0) {
	ErrorF("open_named_local(): ioctl error %d\n",errno);
	return(-1);
    }

    fdetach(path);
    if (fattach(fld[0], path) != 0) {
	ErrorF("open_named_local(): fattach failed, errno=%d\n",errno);
	return(-1);
    } 

    XLOCAL_MSG((0,"NAMED connections available at [%s]\n", path));

    return(fld[1]);
}

static int
accept_named_local()
{
    struct strrecvfd str;

    if (ioctl(namedFd, I_RECVFD, &str) < 0) {
	ErrorF("accept_named_local(): I_RECVFD failed\n");
	return(-1);
    }

    XLOCAL_MSG((1,"new NAMED connection accepted (%d)\n",str.fd));
#ifdef SVR4
    FD_SET(str.fd, &AllStreams);
#else
    BITSET(AllStreams, str.fd);
#endif
    return(str.fd);
}

void
close_named_local()
{
    char path[64];

    if (namedFd != -1) {
	close(namedFd);
	WellKnownConnections &= ~(1L << namedFd);
	namedFd = -1;
    }
    strcpy(path, X_NAMED_PATH);
    strcat(path, display);
    XLOCAL_MSG((2,"removing [%s]\n",path)); 
    xlocal_unlink(path);
}
#endif /* XLOCAL_NAMED */


#ifdef NOVELL
static char *spx_device;
#define MAX_AUTO_BUF_LEN 50
static int
open_tli_spx()
{
        int     i;
        char    service[MAX_AUTO_BUF_LEN];
        int     fd, type;
        struct nd_hostserv  nd_hostserv;
        struct nd_addrlist *nd_addrlistp = NULL;
        struct netbuf      *netbufp = NULL;
        void *handlep;
        struct utsname systemName;

        uname(&systemName);

        if((handlep = setnetpath()) == NULL )
        {
#ifdef DEBUG
           t_error("Error in \"spx\" network initialization");
#endif
           return -1;
        }
        strcpy(service ,  "xserver");
        strcat(service ,  display );
        nd_hostserv.h_host = systemName.nodename; 
        nd_hostserv.h_serv = service;
        while((netconfigp = getnetpath(handlep)) != NULL)
        {
            if(strcmp(netconfigp->nc_proto, "spx"))
                   continue;
            if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp)
                       == 0) {
                    netbufp = nd_addrlistp->n_addrs;
                    for(i=0; i< nd_addrlistp->n_cnt; i++)
                    {
                         if((fd = 
                           BindAndListen(netconfigp->nc_device, netbufp)) < 0){
                              netbufp++;
                              continue;
                          }
		          else {
                          spx_device = strdup(netconfigp->nc_device);
                          (void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
                          return(fd);
                          }
                     }
                }
         }
    return -1;
}
static int
BindAndListen(clonedev, netbufp)
char    *clonedev;
struct   netbuf *netbufp;
{
        int     fd;
        struct  t_bind bindbuf;

        bindbuf.addr.buf = netbufp->buf;
        bindbuf.addr.len = netbufp->len;
        bindbuf.addr.maxlen = netbufp->maxlen;

        if ((fd = t_open(clonedev,  O_RDWR, NULL)) < 0)
        {
#ifdef DEBUG
            fprintf(stderr, "Cannot open %s\n", clonedev);
#endif
            return -1 ;
        }
        bindbuf.qlen = 8;
        if(t_bind (fd, &bindbuf, NULL) < 0)
        {
#ifdef DEBUG
                t_error("t_bind failed");
#endif
                close(fd);
                return -1;
        }
        return(fd);
}

static int
accept_tli_spx()
{
   struct t_call *call;
   int ret;
   static int sequential_failures;
   if(( call = (struct t_call*)t_alloc(netWareFd, T_CALL, T_ALL))
			== NULL ) {
       sequential_failures++; /* protection from stopnps(1M) */
#ifdef DEBUG
       t_error("t_alloc of t_call struct failed");
#endif
       if( sequential_failures == 2 ) {  /* protection from stopnps(M) */
       t_close(netWareFd);
       netWareFd = -1;
       sequential_failures = 0;
       AutoResetServer(SIGHUP);
       }
       else
        return(-1); 
   }
   else
      sequential_failures = 0;


   if (t_listen(netWareFd, call) < 0 )  {
#ifdef DEBUG
       t_error("t_listen failed for netWareFd");
#endif
       return(-1);
   }

   if (( ret = t_open(spx_device, O_RDWR, NULL)) < 0) {
#ifdef DEBUG
	t_error(" t_open for /dev/nspx failed");
#endif
	return(-1);
   }
   if(tbind_ret.addr.buf == NULL) {
        tbind_ret.addr.buf = (char *) xalloc(BIND_BUFSIZE);
        tbind_ret.addr.maxlen = BIND_BUFSIZE;
   }
   if(t_bind(ret, NULL, &tbind_ret) < 0) {
#ifdef DEBUG
        t_error("t_bind failed in accept");
#endif
	return(-1);
   }

   if( t_accept(netWareFd, ret, call) < 0 ) {
#ifdef DEBUG
        t_error("t_accept failed for ipx/spx connection");
#endif
	return( -1 );
   }

   FD_SET(ret, &AllTLIConnections); 
   return (ret);
}

static int 
tli_getpeername(fd)
int fd;
{
   struct nd_hostservlist * list;
   extern int t_errno;

   if(ioctl(fd, I_POP, "tirdwr") < 0) {
        t_close(fd);
        return(-1);
   }
   if(ioctl(fd, I_PUSH, "timod") < 0) {
        t_close(fd);
        return(-1);
   }

   if(t_getprotaddr( fd, NULL, &tbind_ret) == -1) {
#ifdef DEBUG
	t_error("t_getprotaddr failed: ");
#endif
        return -1;
   }
   if(netdir_getbyaddr( netconfigp, &list, &tbind_ret.addr )) {
#ifdef DEBUG
	netdir_perror("netdir_getbyaddr failed:");
#endif
	return -1;
   }
   if(ioctl(fd, I_POP, "timod") < 0) {
        t_close(fd);
        return(-1);
   }
   if(ioctl(fd, I_PUSH, "tirdwr") < 0) {
        t_close(fd);
        return(-1);
   }
   spx_host = strdup (list->h_hostservs->h_host);
   return 0;
}
#endif /* NOVELL */

/*=========================================================================*/
xlocal_create_sockets()
{
    int request;

    ChooseLocalConnectionType();
#ifdef SVR4
    FD_ZERO(&AllStreams);
#else
    CLEARBITS(AllStreams);
#endif
#ifdef XLOCAL_PTS
    if ((ptsFd = open_pts_local()) == -1) {
	XLOCAL_MSG((0,"open_pts_local(): failed.\n"));
    } else {
	WellKnownConnections |= (1L << ptsFd);
    }
#endif
#ifdef XLOCAL_NAMED
    if ((namedFd = open_named_local()) == -1) {
	XLOCAL_MSG((0,"open_named_local(): failed.\n"));
    } else {
	WellKnownConnections |= (1L << namedFd);
    }
#endif
#ifdef XLOCAL_ISC
    if ((iscFd = open_isc_local()) == -1) {
	XLOCAL_MSG((0,"open_isc_local(): failed.\n"));
    } else {
	WellKnownConnections |= (1L << iscFd);
    }
#endif
#ifdef XLOCAL_SCO
    if ((scoFd = open_sco_local()) == -1) {
	XLOCAL_MSG((0,"open_sco_local(): failed.\n"));
    } else {
	WellKnownConnections |= (1L << scoFd);
    } 
#endif
#ifdef XLOCAL_UNIX
    if ((request = open_unix_local()) == -1) {
	XLOCAL_MSG((0,"open_unix_local(): failed.\n"));
    } else {
	WellKnownConnections |= (1L << request);
	unixDomainConnection = request;
    }
#endif
#ifdef TCPCONN
    if ((request = open_tcp_socket()) == -1) {
	XLOCAL_MSG((0,"open_tcp_socket(): failed.\n"));
	if (PartialNetwork == FALSE) {
	    FatalError("Cannot establish tcp listening socket");
	}
    } else {
	XLOCAL_MSG((0,"TCP connections available at port %d\n",
		    X_TCP_PORT + atoi(display)));
	WellKnownConnections |= (1L << request);
	DefineSelf(request);
    }
#endif /* TCPCONN */
}

void
xlocal_reset_sockets()
{
#ifdef XLOCAL_UNIX
    reset_unix_local();
#endif
#ifdef SVR4
    FD_ZERO(&AllStreams);
#ifdef NOVELL
    FD_ZERO(&AllTLIConnections);
#endif /* NOVELL */
#else
    CLEARBITS(AllStreams);
#endif
}

int
xlocal_close_sockets()
{
#ifdef XLOCAL_UNIX
    close_unix_local();
#endif /* XLOCAL_UNIX */
#ifdef XLOCAL_ISC
    close_isc_local();
#endif /* XLOCAL_ISC */
#ifdef XLOCAL_SCO
    close_sco_local();
#endif /* XLOCAL_SCO */
#ifdef XLOCAL_PTS
    close_pts_local();
#endif /* XLOCAL_PTS */
#ifdef XLOCAL_NAMED
    close_named_local();
#endif /* XLOCAL_NAMED */
    return(0);
}

int
xlocal_getpeername(fd, from, fromlen)
  int fd;
  struct sockaddr *from;
  int *fromlen;
{
 /* special case for local connections ( ISC, SCO, PTS, NAMED... ) */

#ifdef SVR4
  if (FD_ISSET(fd, &AllStreams))                                                
#else
  if (GETBIT(AllStreams, fd))
#endif
    {
	from->sa_family = AF_UNSPEC;
	*fromlen = 0;
	return 0;
    }
#ifdef NOVELL
    if (FD_ISSET(fd, &AllTLIConnections))
    {
        if(! tli_getpeername(fd)) {
           from->sa_family = AF_UNSPEC;
           *fromlen = 0;
           return 0;
        }
    }
#endif /* NOVELL */

#if defined(TCPCONN) || defined(DNETCONN) || defined(UNIXCONN)
 return( getpeername(fd, from, (size_t *)fromlen) );
#else
    return(-1);
#endif
}
#define	getpeername	xlocal_getpeername

int
xlocal_accept(fd, from, fromlen)
  int fd;
  struct sockaddr *from;
  int *fromlen;
{
    int ret;

#ifdef XLOCAL_PTS
    if (fd == ptsFd) return accept_pts_local();
#endif
#ifdef XLOCAL_NAMED
    if (fd == namedFd) return accept_named_local();
#endif
#ifdef XLOCAL_ISC
    if (fd == iscFd) return accept_isc_local();
#endif
#ifdef XLOCAL_SCO
    if (fd == scoFd)  return accept_sco_local(); 
#endif
#ifdef NOVELL
     if (fd == netWareFd) 
          return accept_tli_spx();
#endif /* NOVELL */

    /*
     * else we are handling the normal accept case
     */
#if defined(TCPCONN) || defined(DNETCONN) || defined(XLOCAL_UNIX)
    ret = accept (fd, from, (size_t *)fromlen);

#ifdef XLOCAL_UNIX
    if (fd == unixDomainConnection) {
	XLOCAL_MSG((1,"new UNIX connection accepted (%d)\n",ret));
    } else
#endif
      {
	  XLOCAL_MSG((1,"new TCP connection accepted (%d)\n",ret));
      }
#endif
    return(ret);
}
#define	accept		xlocal_accept

#endif /* SERVER_LOCALCONN */
/*=========================================================================*/


#ifdef hpux
/*
 * hpux returns EOPNOTSUPP when using getpeername on a unix-domain
 * socket.  In this case, smash the socket address with the address
 * used to bind the connection socket and return success.
 */
hpux_getpeername(fd, from, fromlen)
    int	fd;
    struct sockaddr *from;
    int		    *fromlen;
{
    int	    ret;
    int	    len;

    ret = getpeername(fd, from, fromlen);
    if (ret == -1 && errno == EOPNOTSUPP)
    {
	ret = 0;
	len = strlen(unsock.sun_path)+2;
	if (len > *fromlen)
	    len = *fromlen;
	bcopy ((char *) &unsock, (char *) from, len);
	*fromlen = len;
    }
    return ret;
}

#define getpeername(fd, from, fromlen)	hpux_getpeername(fd, from, fromlen)

#endif

#ifdef DNETCONN
static int
open_dnet_socket ()
{
    int request;
    struct sockaddr_dn dnsock;

    if ((request = socket (AF_DECnet, SOCK_STREAM, 0)) < 0) 
    {
	Error ("Creating DECnet socket");
	return -1;
    } 
    bzero ((char *)&dnsock, sizeof (dnsock));
    dnsock.sdn_family = AF_DECnet;
    sprintf(dnsock.sdn_objname, "X$X%d", atoi (display));
    dnsock.sdn_objnamel = strlen(dnsock.sdn_objname);
    if (bind (request, (struct sockaddr *) &dnsock, sizeof (dnsock)))
    {
	Error ("Binding DECnet socket");
	close (request);
	return -1;
    }
    if (listen (request, 5))
    {
	Error ("DECnet Listening");
	close (request);
	return -1;
    }
    return request;
}
#endif /* DNETCONN */

/*****************
 * CreateWellKnownSockets
 *    At initialization, create the sockets to listen on for new clients.
 *****************/

void
CreateWellKnownSockets()
{
    int		request, i;
#ifdef SVR4
    struct rlimit Rlimit;
#endif

#ifdef SVR4
    FD_ZERO(&AllSockets);
    FD_ZERO(&AllClients);
    FD_ZERO(&LastSelectMask);
    FD_ZERO(&ClientsWithInput);
#else
    CLEARBITS(AllSockets);
    CLEARBITS(AllClients);
    CLEARBITS(LastSelectMask);
    CLEARBITS(ClientsWithInput);
#endif

    for (i=0; i<MAXSOCKS; i++) ConnectionTranslation[i] = 0;
    
#if defined(hpux)
    lastfdesc = _NFILE - 1;
#else   /* hpux */
#ifdef SVR4
#ifndef _NFILE
#define _NFILE 60
#endif /* _NFILE */
    if (getrlimit(RLIMIT_NOFILE, &Rlimit) != 0) {
	lastfdesc = _NFILE - 1;
    } else {
	lastfdesc = Rlimit.rlim_cur;
    }
#else   /* SVR4 */
    lastfdesc = getdtablesize() - 1;
#endif  /* SVR4 */
#endif	/* hpux */

    if (lastfdesc > MAXSOCKS)
    {
	lastfdesc = MAXSOCKS;
	if (debug_conns)
	    ErrorF( "GOT TO END OF SOCKETS %d\n", MAXSOCKS);
    }

    WellKnownConnections = 0;

#ifdef SERVER_LOCALCONN
    xlocal_create_sockets();
#else /* !SERVER_LOCALCONN */
#ifdef TCPCONN
    if ((request = open_tcp_socket ()) != -1) {
	WellKnownConnections |= (1L << request);
	DefineSelf (request);
    }
    else if (PartialNetwork == FALSE)
    {
	FatalError ("Cannot establish tcp listening socket");
    }
#endif /* TCPCONN */
#ifdef DNETCONN
    if ((request = open_dnet_socket ()) != -1) {
	WellKnownConnections |= (1L << request);
	DefineSelf (request);
    }
    else if (PartialNetwork == FALSE) 
    {
	FatalError ("Cannot establish dnet listening socket");
    }
#endif /* DNETCONN */
#ifdef UNIXCONN
    if ((request = open_unix_socket ()) != -1) {
	WellKnownConnections |= (1L << request);
	unixDomainConnection = request;
    }
    else if (PartialNetwork == FALSE) 
    {
      FatalError ("Cannot establish unix listening socket");
    }
#endif /* UNIXCONN */
#endif /* SERVER_LOCALCONN */
#ifdef NOVELL
    if ((netWareFd = open_tli_spx()) != -1) {
        WellKnownConnections |= (1L << netWareFd);
    }
#endif /* NOVELL */

#ifdef SVR4
    if (openvt() < 0)	/* Open the console device */
      attexit(-1);
#endif

    if (WellKnownConnections == 0)
        FatalError ("Cannot establish any listening sockets");
    signal (SIGPIPE, SIG_IGN);
    signal (SIGHUP, AutoResetServer);
    signal (SIGINT, GiveUp);
    signal (SIGTERM, GiveUp);
#ifdef SVR4
    AllSockets.fds_bits[0] = WellKnownConnections;
#else
    AllSockets[0] = WellKnownConnections;
#endif
    ResetHosts(display);
    /*
     * Magic:  If SIGUSR1 was set to SIG_IGN when
     * the server started, assume that either
     *
     *  a- The parent process is ignoring SIGUSR1
     *
     * or
     *
     *  b- The parent process is expecting a SIGUSR1
     *     when the server is ready to accept connections
     *
     * In the first case, the signal will be harmless,
     * in the second case, the signal will be quite
     * useful
     */
    if (signal (SIGUSR1, SIG_IGN) == SIG_IGN)
	RunFromSmartParent = TRUE;
    ParentProcess = getppid ();
    if (RunFromSmartParent) {
	if (ParentProcess > 0) {
	    kill (ParentProcess, SIGUSR1);
	}
    }
#ifdef XDMCP
    XdmcpInit ();
#endif
}

void
ResetWellKnownSockets ()
{
    ResetOsBuffers();

#ifdef SERVER_LOCALCONN
    xlocal_reset_sockets();
#else /* SERVER_LOCALCONN */
#ifdef UNIXCONN
    if (unixDomainConnection != -1)
    {
	/*
	 * see if the unix domain socket has disappeared
	 */
	struct stat	statb;

	if (stat (unsock.sun_path, &statb) == -1 ||
	    (statb.st_mode & S_IFMT) != S_IFSOCK)
	{
	    ErrorF ("Unix domain socket %s trashed, recreating\n",
	    	unsock.sun_path);
	    (void) unlink (unsock.sun_path);
	    (void) close (unixDomainConnection);
	    WellKnownConnections &= ~(1L << unixDomainConnection);
	    unixDomainConnection = open_unix_socket ();
	    if (unixDomainConnection != -1)
		WellKnownConnections |= (1L << unixDomainConnection);
	}
    }
#endif /* UNIXCONN */
#endif /* SERVER_LOCALCONN */
    ResetAuthorization ();
    ResetHosts(display);
    /*
     * See above in CreateWellKnownSockets about SIGUSR1
     */
    if (RunFromSmartParent) {
	if (ParentProcess > 0) {
	    kill (ParentProcess, SIGUSR1);
	}
    }
    /*
     * restart XDMCP
     */
#ifdef XDMCP
    XdmcpReset ();
#endif
}

/*****************************************************************
 * ClientAuthorized
 *
 *    Sent by the client at connection setup:
 *                typedef struct _xConnClientPrefix {
 *                   CARD8	byteOrder;
 *                   BYTE	pad;
 *                   CARD16	majorVersion, minorVersion;
 *                   CARD16	nbytesAuthProto;    
 *                   CARD16	nbytesAuthString;   
 *                 } xConnClientPrefix;
 *
 *     	It is hoped that eventually one protocol will be agreed upon.  In the
 *        mean time, a server that implements a different protocol than the
 *        client expects, or a server that only implements the host-based
 *        mechanism, will simply ignore this information.
 *
 *****************************************************************/

char * 
ClientAuthorized(client, proto_n, auth_proto, string_n, auth_string)
    ClientPtr client;
    char *auth_proto, *auth_string;
    unsigned short proto_n, string_n;
{
    register OsCommPtr priv; 
    union {
	struct sockaddr sa;
#ifdef UNIXCONN
	struct sockaddr_un un;
#endif /* UNIXCONN */
#ifdef TCPCONN
	struct sockaddr_in in;
#endif /* TCPCONN */
#ifdef DNETCONN
	struct sockaddr_dn dn;
#endif /* DNETCONN */
    } from;
    int	fromlen = sizeof (from);
    XID	 auth_id;

#ifdef SERVER_LOCALCONN
    /*
    || A bypass to match the functionality from
    || USLs R4 server.  This can probably be done better at
    || some other level...
    */
    /*
     * it's better not to bypass this, we use local authentication. 
     * In addition, with this bypass conn_time doesn't get initialized and
     * that causes major problems.
     * If you want to bypass this, define BYPASS here; so that you
     * can disable this, but by default, we don't want it disabled.
     */
    int clientfd =  ((OsCommPtr)client->osPrivate)->fd;
#ifdef BYPASS
    if (!GETBIT(AllStreams, clientfd))
    {
#endif /* BYPASS */
#endif

    auth_id = CheckAuthorization (proto_n, auth_proto,
				  string_n, auth_string);

    priv = (OsCommPtr)client->osPrivate;
    if (auth_id == (XID) ~0L && 
#ifdef BUILTIN
        (BIIsServerFd(clientfd) || (
#endif
   	getpeername (clientfd, &from.sa, &fromlen) != -1 &&
#ifdef NOVELL
        !InvalidHost (&from.sa, fromlen, clientfd))
#else
        !InvalidHost (&from.sa, fromlen))
#endif NOVELL
#ifdef BUILTIN
         ))
#endif

    {
	auth_id = (XID) 0;
    }
    if (auth_id == (XID) ~0L)
	return "Client is not authorized to connect to Server";

    priv->auth_id = auth_id;
    priv->conn_time = 0;

#ifdef SERVER_LOCALCONN
#ifdef BYPASS
    } /* GETBIT - AllStreams */
#endif /* BYPASS */
#endif

#ifdef XDMCP
    /* indicate to Xdmcp protocol that we've opened new client */
    XdmcpOpenDisplay(priv->fd);
#endif /* XDMCP */
    /* At this point, if the client is authorized to change the access control
     * list, we should getpeername() information, and add the client to
     * the selfhosts list.  It's not really the host machine, but the
     * true purpose of the selfhosts list is to see who may change the
     * access control list.
     */
    return((char *)NULL);
}

/*****************
 * EstablishNewConnections
 *    If anyone is waiting on listened sockets, accept them.
 *    Returns a mask with indices of new clients.  Updates AllClients
 *    and AllSockets.
 *****************/

/*ARGSUSED*/
Bool
EstablishNewConnections(clientUnused, closure)
    ClientPtr clientUnused;
    pointer closure;
{
    long readyconnections;     /* mask of listeners that are ready */
    int curconn;                  /* fd of listener that's ready */
    register int newconn;         /* fd of new client */
    long connect_time;
    register int i;
    register ClientPtr client;
    register OsCommPtr oc;

#ifdef TCP_NODELAY
    union {
	struct sockaddr sa;
#ifdef UNIXCONN
	struct sockaddr_un un;
#endif /* UNIXCONN */
#ifdef TCPCONN
	struct sockaddr_in in;
#endif /* TCPCONN */
#ifdef DNETCONN
	struct sockaddr_dn dn;
#endif /* DNETCONN */
    } from;
    int	fromlen;
#endif /* TCP_NODELAY */

    readyconnections = (((long)closure) & WellKnownConnections);
    if (!readyconnections)
	return TRUE;
    connect_time = GetTimeInMillis();
    /* kill off stragglers */
    for (i=1; i<currentMaxClients; i++)
    {
	if (client = clients[i])
	{
	    oc = (OsCommPtr)(client->osPrivate);
	    if (oc && (oc->conn_time != 0) &&
		(connect_time - oc->conn_time) >= TimeOutValue)
		CloseDownClient(client);     
	}
    }
    while (readyconnections) 
    {
	curconn = ffs (readyconnections) - 1;
	readyconnections &= ~(1 << curconn);
	if ((newconn = accept (curconn,
			      (struct sockaddr *) NULL, 
			      (int *)NULL)) < 0) 
	    continue;
#ifdef TCP_NODELAY
	fromlen = sizeof (from);
	if (!getpeername (newconn, &from.sa, &fromlen))
	{
	    if (fromlen && (from.sa.sa_family == AF_INET)) 
	    {
		int mi = 1;
		setsockopt (newconn, IPPROTO_TCP, TCP_NODELAY,
			   (char *)&mi, sizeof (int));
	    }
	}
#endif /* TCP_NODELAY */
    /* ultrix reads hang on Unix sockets, hpux reads fail, AIX fails too */
#if defined(O_NONBLOCK) && (!defined(ultrix) && !defined(hpux) && !defined(AIXV3) && !defined(uniosu))
	(void) fcntl (newconn, F_SETFL, O_NONBLOCK);
#else
#ifdef FIOSNBIO
	{
	    int	arg;
	    arg = 1;
	    ioctl(newconn, FIOSNBIO, &arg);
	}
#else
#if (defined(AIXV3) || defined(uniosu)) && defined(FIONBIO)
	{
	    int arg;
	    arg = 1;
	    ioctl(newconn, FIONBIO, &arg);
	}
#else
	fcntl (newconn, F_SETFL, FNDELAY);
#endif
#endif
#endif
	oc = (OsCommPtr)xalloc(sizeof(OsCommRec));
	if (!oc)
	{
	    ErrorConnMax(newconn);
	    close(newconn);
	    continue;
	}
	if (GrabInProgress)
	{
#ifdef SVR4
	    FD_SET(newconn,&SavedAllClients);
	    FD_SET(newconn,&SavedAllSockets);
#else
	    BITSET(SavedAllSockets, newconn);
	    BITSET(SavedAllSockets, newconn);
#endif
	}
	else
	{
#ifdef SVR4
	    FD_SET(newconn,&AllClients);
	    FD_SET(newconn,&AllSockets);
#else
	    BITSET(AllClients, newconn);
	    BITSET(AllSockets, newconn);
#endif
	}
	oc->fd = newconn;
	oc->input = (ConnectionInputPtr)NULL;
	oc->output = (ConnectionOutputPtr)NULL;
	oc->conn_time = connect_time;
	if ((newconn < lastfdesc - 1) &&
	    (client = NextAvailableClient((pointer)oc)))
	{
	    ConnectionTranslation[newconn] = client->index;
	}
	else
	{
	    ErrorConnMax(newconn);
	    CloseDownFileDescriptor(oc);
	}
    }
    return TRUE;
}

#define NOROOM "Maximum number of clients reached"

/************
 *   ErrorConnMax
 *     Fail a connection due to lack of client or file descriptor space
 ************/

static void
ErrorConnMax(fd)
    register int fd;
{
    xConnSetupPrefix csp;
    char pad[3];
    struct iovec iov[3];
    char byteOrder = 0;
    int whichbyte = 1;
    struct timeval waittime;
#ifdef SVR4
  fd_set mask;
#else
    long mask[mskcnt];
#endif

    /* if these seems like a lot of trouble to go to, it probably is */
    waittime.tv_sec = BOTIMEOUT / MILLI_PER_SECOND;
    waittime.tv_usec = (BOTIMEOUT % MILLI_PER_SECOND) *
		       (1000000 / MILLI_PER_SECOND);
#ifdef SVR4
    FD_ZERO(&mask);
    FD_SET(fd, &mask);
    (void)select(fd + 1, &mask, (fd_set *) NULL, (fd_set *) NULL, &waittime);
#else
    CLEARBITS(mask);
    BITSET(mask, fd);
    (void)select(fd + 1, (int *) mask, (int *) NULL, (int *) NULL, &waittime);
#endif

    /* try to read the byte-order of the connection */
    (void)read(fd, &byteOrder, 1);
    if ((byteOrder == 'l') || (byteOrder == 'B'))
    {
	csp.success = xFalse;
	csp.lengthReason = sizeof(NOROOM) - 1;
	csp.length = (sizeof(NOROOM) + 2) >> 2;
	csp.majorVersion = X_PROTOCOL;
	csp.minorVersion = X_PROTOCOL_REVISION;
	if (((*(char *) &whichbyte) && (byteOrder == 'B')) ||
	    (!(*(char *) &whichbyte) && (byteOrder == 'l')))
	{
	    swaps(&csp.majorVersion, whichbyte);
	    swaps(&csp.minorVersion, whichbyte);
	    swaps(&csp.length, whichbyte);
	}
	iov[0].iov_len = sz_xConnSetupPrefix;
	iov[0].iov_base = (char *) &csp;
	iov[1].iov_len = csp.lengthReason;
	iov[1].iov_base = NOROOM;
	iov[2].iov_len = (4 - (csp.lengthReason & 3)) & 3;
	iov[2].iov_base = pad;
	(void)writev(fd, iov, 3);
    }
}

/************
 *   CloseDownFileDescriptor:
 *     Remove this file descriptor and it's I/O buffers, etc.
 ************/

static void
CloseDownFileDescriptor(oc)
    register OsCommPtr oc;
{
    int connection = oc->fd;

    if(FD_ISSET(connection, &AllTLIConnections)) {
      t_snddis(connection,(struct t_call *)NULL);
      t_unbind(connection);
      t_close(connection);
    }
    else
      close(connection);

    FreeOsBuffers(oc);

#ifdef SVR4
    FD_CLR(connection, &AllSockets);
    FD_CLR(connection, &AllClients);
#else
    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
#endif
#ifdef SERVER_LOCALCONN
#ifdef SVR4
    FD_CLR(connection, &AllStreams);
#else
    BITCLEAR(AllStreams, connection);
#endif
#endif
#ifdef NOVELL
FD_CLR(connection, &AllTLIConnections);
#endif /* NOVELL */
#ifdef SVR4
    FD_CLR(connection, &ClientsWithInput);
#else
    BITCLEAR(ClientsWithInput, connection);
#endif
    if (GrabInProgress)
    {
#ifdef SVR4
	FD_CLR(connection,&SavedAllSockets);
	FD_CLR(connection,&SavedAllClients);
	FD_CLR(connection,&SavedClientsWithInput);
#else
	BITCLEAR(SavedAllSockets, connection);
	BITCLEAR(SavedAllClients, connection);
	BITCLEAR(SavedClientsWithInput, connection);
#endif
    }
#ifdef SVR4
    FD_CLR(connection, &ClientsWriteBlocked);
#else
    BITCLEAR(ClientsWriteBlocked, connection);
#endif
#ifdef SVR4
    if (!ANYSET(ClientsWriteBlocked.fds_bits))
    	AnyClientsWriteBlocked = FALSE;
    FD_CLR(connection, &OutputPending);
#else
    if (!ANYSET(ClientsWriteBlocked))
    	AnyClientsWriteBlocked = FALSE;
    BITCLEAR(OutputPending, connection);
#endif
    xfree(oc);
}

/*****************
 * CheckConections
 *    Some connection has died, go find which one and shut it down 
 *    The file descriptor has been closed, but is still in AllClients.
 *    If would truly be wonderful if select() would put the bogus
 *    file descriptors in the exception mask, but nooooo.  So we have
 *    to check each and every socket individually.
 *****************/

void
CheckConnections()
{
    long		mask;
#ifdef SVR4
    fd_set		tmask;
#else
    long		tmask[mskcnt]; 
#endif

    register int	curclient, curoff;
    int			i;
    struct timeval	notime;
    int r;

    notime.tv_sec = 0;
    notime.tv_usec = 0;

    for (i=0; i<mskcnt; i++)
    {
#ifdef SVR4
	mask = AllClients.fds_bits[i];
#else
	mask = AllClients[i];
#endif
        while (mask)
    	{
	    curoff = ffs (mask) - 1;
 	    curclient = curoff + (i << 5);
#ifdef SVR4
            FD_ZERO(&tmask);
            FD_SET(curclient, &tmask);
            r = select (curclient + 1, &tmask, (fd_set *)NULL, (fd_set *)NULL, 
			&notime);
#else
            CLEARBITS(tmask);
            BITSET(tmask, curclient);
            r = select (curclient + 1, (int *)tmask, (int *)NULL, (int *)NULL, 
			&notime);
#endif
                  if (r < 0)
		CloseDownClient(clients[ConnectionTranslation[curclient]]);
	    mask &= ~(1 << curoff);
	}
    }	
}


/*****************
 * CloseDownConnection
 *    Delete client from AllClients and free resources 
 *****************/

void
CloseDownConnection(client)
    ClientPtr client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;

    if (oc->output && oc->output->count)
	FlushClient(client, oc, (char *)NULL, 0);
    ConnectionTranslation[oc->fd] = 0;
#ifdef XDMCP
    XdmcpCloseDisplay(oc->fd);
#endif
    CloseDownFileDescriptor(oc);
    client->osPrivate = (pointer)NULL;
}

void
AddEnabledDevice(fd)
    int fd;
{
#ifdef SVR4
    FD_SET(fd, &EnabledDevices);
    FD_SET(fd, &AllSockets);
#else
    BITSET(EnabledDevices, fd);
    BITSET(AllSockets, fd);
#endif
}

void
RemoveEnabledDevice(fd)
    int fd;
{
#ifdef SVR4
    FD_CLR(fd, &EnabledDevices);
    FD_CLR(fd, &AllSockets);
#else
    BITCLEAR(EnabledDevices, fd);
    BITCLEAR(AllSockets, fd);
#endif
}

#if defined(XTEST) && !defined(NO_NEW_XTRAP)
/* for new version of XTrap */
void
AlsoListenToClient(client)
    ClientPtr client;
{
    if (client && GrabInProgress && (GrabInProgress != client->index))
    {
	OsCommPtr oc = (OsCommPtr)client->osPrivate;
	int connection = oc->fd;
#ifdef SVR4
        FD_SET(connection, &AllClients);
        FD_SET(connection, &AllSockets);
        FD_SET(connection, &LastSelectMask);
        if (FD_ISSET(connection, &SavedClientsWithInput))
            FD_SET(connection, &ClientsWithInput);
#else
        BITSET(AllClients, connection);
        BITSET(AllSockets, connection);
        BITSET(LastSelectMask, connection);
        if (GETBIT(SavedClientsWithInput, connection))
            BITSET(ClientsWithInput, connection);
#endif
    }
}

/* for new version of XTrap */
void
AlsoUnListenToClient(client)
    ClientPtr client;
{
    if (client && GrabInProgress && (GrabInProgress != client->index))
    {
	OsCommPtr oc = (OsCommPtr)client->osPrivate;
	int connection = oc->fd;
#ifdef SVR4
        FD_SET(connection, &AllClients);
        FD_SET(connection, &AllSockets);
        FD_SET(connection, &LastSelectMask);
        if (FD_ISSET(connection, &SavedClientsWithInput))
            FD_SET(connection, &ClientsWithInput);
#else
        BITSET(AllClients, connection);
        BITSET(AllSockets, connection);
        BITSET(LastSelectMask, connection);
        if (GETBIT(SavedClientsWithInput, connection))
            BITSET(ClientsWithInput, connection);
#endif
    }
}
#endif /* defined(XTEST) && !defined(NO_NEW_XTRAP) */

/*****************
 * OnlyListenToOneClient:
 *    Only accept requests from  one client.  Continue to handle new
 *    connections, but don't take any protocol requests from the new
 *    ones.  Note that if GrabInProgress is set, EstablishNewConnections
 *    needs to put new clients into SavedAllSockets and SavedAllClients.
 *    Note also that there is no timeout for this in the protocol.
 *    This routine is "undone" by ListenToAllClients()
 *****************/
void
OnlyListenToOneClient(client)
    ClientPtr client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (! GrabInProgress)
    {
#ifdef SVR4
	COPYBITS (ClientsWithInput.fds_bits, SavedClientsWithInput.fds_bits);
        FD_CLR(connection, &SavedClientsWithInput);
	if (FD_ISSET(connection, &ClientsWithInput))
	{
	    FD_ZERO(&ClientsWithInput);	    
	    FD_SET(connection, &ClientsWithInput);
	}
	else
        {
	    FD_ZERO(&ClientsWithInput);	    
	}
	COPYBITS(AllSockets.fds_bits, SavedAllSockets.fds_bits);
	COPYBITS(AllClients.fds_bits, SavedAllClients.fds_bits);

	UNSETBITS(AllSockets.fds_bits, AllClients.fds_bits);
	FD_SET(connection, &AllSockets);
	FD_ZERO(&AllClients);
	FD_SET(connection, &AllClients);
#else
	COPYBITS (ClientsWithInput, SavedClientsWithInput);
        BITCLEAR (SavedClientsWithInput, connection);
	if (GETBIT(ClientsWithInput, connection))
	{
	    CLEARBITS(ClientsWithInput);	    
	    BITSET(ClientsWithInput, connection);
	}
	else
        {
	    CLEARBITS(ClientsWithInput);	    
	}
	COPYBITS(AllSockets, SavedAllSockets);
	COPYBITS(AllClients, SavedAllClients);

	UNSETBITS(AllSockets, AllClients);
	BITSET(AllSockets, connection);
	CLEARBITS(AllClients);
	BITSET(AllClients, connection);
#endif
	GrabInProgress = client->index;
    }
}

/****************
 * ListenToAllClients:
 *    Undoes OnlyListentToOneClient()
 ****************/
void
ListenToAllClients()
{
    if (GrabInProgress)
    {
#ifdef SVR4
	ORBITS(AllSockets.fds_bits, AllSockets.fds_bits, SavedAllSockets.fds_bits);
	ORBITS(AllClients.fds_bits, AllClients.fds_bits, SavedAllClients.fds_bits);
	ORBITS(ClientsWithInput.fds_bits, ClientsWithInput.fds_bits, SavedClientsWithInput.fds_bits);
#else
	ORBITS(AllSockets, AllSockets, SavedAllSockets);
	ORBITS(AllClients, AllClients, SavedAllClients);
	ORBITS(ClientsWithInput, ClientsWithInput, SavedClientsWithInput);
#endif
	GrabInProgress = 0;
    }	
}

/****************
 * IgnoreClient
 *    Removes one client from input masks.
 *    Must have cooresponding call to AttendClient.
 ****************/

void
IgnoreClient (client)
    ClientPtr	client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (!GrabInProgress || GrabInProgress == client->index)
    {
#ifdef SVR4
        if (FD_ISSET (connection, &ClientsWithInput))
            FD_SET(connection, &IgnoredClientsWithInput);
        else
            FD_CLR(connection, &IgnoredClientsWithInput);
        FD_CLR(connection, &ClientsWithInput);
        FD_CLR(connection, &AllSockets);
        FD_CLR(connection, &AllClients);
        FD_CLR(connection, &LastSelectMask);
#else
        if (GETBIT (ClientsWithInput, connection))
            BITSET(IgnoredClientsWithInput, connection);
        else
            BITCLEAR(IgnoredClientsWithInput, connection);
        BITCLEAR(ClientsWithInput, connection);
        BITCLEAR(AllSockets, connection);
        BITCLEAR(AllClients, connection);
        BITCLEAR(LastSelectMask, connection);
#endif
    }
    else
    {
#ifdef SVR4
        if (FD_ISSET (connection, &SavedClientsWithInput))
            FD_SET(connection, &IgnoredClientsWithInput);
        else
            FD_CLR(connection, &IgnoredClientsWithInput);
        FD_CLR(connection, &SavedClientsWithInput);
        FD_CLR(connection, &SavedAllSockets);
        FD_CLR(connection, &SavedAllClients);
#else
        if (GETBIT (SavedClientsWithInput, connection))
            BITSET(IgnoredClientsWithInput, connection);
        else
            BITCLEAR(IgnoredClientsWithInput, connection);
        BITCLEAR(SavedClientsWithInput, connection);
        BITCLEAR(SavedAllSockets, connection);
        BITCLEAR(SavedAllClients, connection);
#endif
    }
    isItTimeToYield = TRUE;
}

/****************
 * AttendClient
 *    Adds one client back into the input masks.
 ****************/
void
AttendClient (client)
    ClientPtr	client;
{
    OsCommPtr oc = (OsCommPtr)client->osPrivate;
    int connection = oc->fd;

    if (!GrabInProgress || GrabInProgress == client->index)
    {
#ifdef SVR4
    	FD_SET(connection, &AllClients);
    	FD_SET(connection, &AllSockets);
	FD_SET(connection, &LastSelectMask);
    	if (FD_ISSET (connection, &IgnoredClientsWithInput))
	    FD_SET(connection, &ClientsWithInput);
    }
    else
    {
	FD_SET(connection, &SavedAllClients);
	FD_SET(connection, &SavedAllSockets);
	if (FD_ISSET(connection, &IgnoredClientsWithInput))
	    FD_SET(connection, &SavedClientsWithInput);
#else
    	BITSET(AllClients, connection);
    	BITSET(AllSockets, connection);
	BITSET(LastSelectMask, connection);
    	if (GETBIT (IgnoredClientsWithInput, connection))
	    BITSET(ClientsWithInput, connection);
    }
    else
    {
	BITSET(SavedAllClients, connection);
	BITSET(SavedAllSockets, connection);
	if (GETBIT(IgnoredClientsWithInput, connection))
	    BITSET(SavedClientsWithInput, connection);
#endif
    }
#ifdef BUILTIN
    BIAttendClient(connection);
#endif
}

#ifdef AIXV3

static int grabbingClient;
static Bool reallyGrabbed;

/****************
* DontListenToAnybody:
*   Don't listen to requests from any clients. Continue to handle new
*   connections, but don't take any protocol requests from anybody.
*   We have to take care if there is already a grab in progress, though.
*   Undone by PayAttentionToClientsAgain. We also have to be careful
*   not to accept any more input from the currently dispatched client.
*   we do this be telling dispatch it is time to yield.

*   We call this when the server loses access to the glass
*   (user hot-keys away).  This looks like a grab by the 
*   server itself, but gets a little tricky if there is already
*   a grab in progress.
******************/

void
DontListenToAnybody()
{
    if (!GrabInProgress)
    {
#ifdef SVR4
        COPYBITS(ClientsWithInput.fds_bits, SavedClientsWithInput.fds_bits);            COPYBITS(AllSockets.fds_bits, SavedAllSockets.fds_bits);
        COPYBITS(AllClients.fds_bits, SavedAllClients.fds_bits);
#else
        COPYBITS(ClientsWithInput, SavedClientsWithInput);
        COPYBITS(AllSockets, SavedAllSockets);
        COPYBITS(AllClients, SavedAllClients);
#endif

	GrabInProgress = TRUE;
	reallyGrabbed = FALSE;
    }
    else
    {
	grabbingClient = ((OsCommPtr)clients[GrabInProgress]->osPrivate)->fd;
	reallyGrabbed = TRUE;
    }
#ifdef SVR4
    FD_ZERO(&ClientsWithInput);
    UNSETBITS(AllSockets.fds_bits, AllClients.fds_bits);
    FD_ZERO(&AllClients);
#else
    CLEARBITS(ClientsWithInput);
    UNSETBITS(AllSockets, AllClients);
    CLEARBITS(AllClients);
#endif

    isItTimeToYield = TRUE;
}

void
PayAttentionToClientsAgain()
{
    if (reallyGrabbed)
    {
#ifdef SVR4
        FD_SET(grabbingClient, &AllSockets);
        FD_SET(grabbingClient, &AllClients);
#else
        BITSET(AllSockets, grabbingClient);
        BITSET(AllClients, grabbingClient);
#endif
    }
    else
    {
	ListenToAllClients();
    }
    reallyGrabbed = FALSE;
}

#endif
