/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpNet/parent/parent.c	1.10.2.9"
#ident  "$Header: parent.c 1.2 91/06/27 $"

/*=================================================================*/
/*
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <stropts.h>
#include <signal.h>
#include <string.h>
#include <netdir.h>
#include <netconfig.h>
#include "lpNet.h"
#include "lp.h"
#include "networkMgmt.h"
#include "logMgmt.h"
#include "errorMgmt.h"
#include "mpipes.h"
#include "debug.h"
#include "systems.h"

#ifndef	_POLLFD_T
typedef	struct	pollfd		pollfd_t;
#endif

#ifndef	_STRRECVFD_T
typedef	struct	strrecvfd	strrecvfd_t;
#endif

#ifndef	_SIGACTION_T
typedef	struct	sigaction	sigaction_t;
#endif


/*---------------------------------------------------------*/
/*
*/
#ifdef	__STDC__

	void	lpNetParent (void);
	void	lpNetChild (void);

static	int	NewChild (connectionInfo *, systemInfo *, processRank);
static	int	s5NewChild (connectionInfo *, systemInfo *, processRank);
static	int	bsdNewChild (connectionInfo *, systemInfo *, processRank);
static	int	nucNewChild (connectionInfo *, systemInfo *, processRank);
static	void	LpExecEvent (pollfd_t *);
static	void	listenS5Event (pollfd_t *);
static	void	listenBSDEvent (pollfd_t *);
static	void	lpSystemInfoFilter (systemInfo *);
static	void	Shutdown (boolean);
static	void	SoftwareTerminationTrap (int);

extern	void	svChild (void);
extern	void	bsdChild (void);
extern	void	nucChild (void);

#else

	void	lpNetParent ();
	void	lpNetChild ();

static	int	NewChild ();
static	int	s5NewChild ();
static	int	bsdNewChild ();
static	int	nucNewChild ();
static	void	LpExecEvent ();
static	void	listenS5Event ();
static	void	listenBSDEvent ();
static	void	lpSystemInfoFilter ();
static	void	Shutdown ();
static	void	SoftwareTerminationTrap ();

extern	void	svChild ();
extern	void	bsdChild ();
extern	void	nucChild ();

#endif
/*=================================================================*/

/*==================================================================*/
/*
*/
void
lpNetParent ()
{
	/*---------------------------------------------------------*/
	/*
	*/
		enum
		{
			lpExec		= 0,
			listenS5	= 1,
			listenBSD	= 2
		} pipeIds;

		int	i,
			fd,
			fds [2],
			nEvents;
		ulong	nFiles		= 3;
		pollfd_t	pipeEvents [3];
		sigaction_t	sigAction;

	static	char	FnName []	= "lpNetParent";
	/*---------------------------------------------------------*/

	/*---------------------------------------------------------*/
	/*
	*/
#ifdef	DEBUG
{
	char	fileName [64];

	(void)	sprintf (fileName, "%s/p%d", LP_LOGS_DIR, getpid ());
	OPEN_DEBUG_FILE (fileName)
}
#endif

	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	Parent initialization.
	*/
	(void)	memset (&sigAction, 0, sizeof (sigaction_t));
	sigAction.sa_handler	= SoftwareTerminationTrap;
	if (sigaction (SIGTERM, &sigAction, NULL) == -1)
		TrapError (NonFatal, Unix, FnName, "sigaction");

	(void)	memset (&sigAction, 0, sizeof (sigaction_t));
	sigAction.sa_handler	= SIG_IGN;
	sigAction.sa_flags	= SA_NOCLDWAIT;
	if (sigaction (SIGCLD, &sigAction, NULL) == -1)
		TrapError (NonFatal, Unix, FnName, "sigaction");

	(void)	memset (&sigAction, 0, sizeof (sigaction_t));
	sigAction.sa_handler	= SIG_IGN;
	if (sigaction (SIGXFSZ, &sigAction, NULL) == -1)
		TrapError (NonFatal, Unix, FnName, "sigaction");

	WriteLogMsg ("Starting lpNetParent.");

	if (ProcessInfo.lpExec == -1)
		ProcessInfo.lpExec = 0;

	if (ProcessInfo.lpExec == 0) {
		fd = dup (ProcessInfo.lpExec);
		TRACE (fd)
		if (fd == -1)
			TrapError (Fatal, Unix, FnName, "dup");
		(void)
		close (ProcessInfo.lpExec);
		ProcessInfo.lpExec = fd;
	}
	ProcessInfo.lpExecMsg_p =
		mconnect (NULL, ProcessInfo.lpExec, ProcessInfo.lpExec);
	if (ProcessInfo.lpExecMsg_p == NULL)
		TrapError (Fatal, Unix, FnName, "mconnect");

	ProcessInfo.listenS5 = MountPipe (fds, PIPE_LISTEN_S5);
	if (ProcessInfo.listenS5 == -1)
		TrapError (Fatal, Unix, FnName, "MountPipe");

	ProcessInfo.listenBSD = MountPipe (fds, PIPE_LISTEN_BSD);
	if (ProcessInfo.listenBSD == -1)
		TrapError (Fatal, Unix, FnName, "MountPipe");
	/*---------------------------------------------------------*/

	/*---------------------------------------------------------*/
	/*
	**	Prepare for polling on all pipes.
	*/
	pipeEvents [lpExec].fd		= ProcessInfo.lpExec;
	pipeEvents [lpExec].events	= POLLIN;
	pipeEvents [listenS5].fd	= ProcessInfo.listenS5;
	pipeEvents [listenS5].events	= POLLIN;
	pipeEvents [listenBSD].fd	= ProcessInfo.listenBSD;
	pipeEvents [listenBSD].events	= POLLIN;
	/*---------------------------------------------------------*/

	/*---------------------------------------------------------*/
	/*
	**	Establish a process group for lpNet
	*/
	(void) setsid ();
	/*---------------------------------------------------------*/

	/*---------------------------------------------------------*/
	/*
	**	When polling we will block for an event.
	*/
	WriteLogMsg ("Initialized & Polling.");

	nFiles = 3;
	TRACE (nFiles)
	while (ProcessInfo.processType == ParentProcess)
	{

		/*-------------------------------------------------*/
		/*
		*/
		nEvents = poll (pipeEvents, nFiles, -1);
		TRACE (nEvents)
		TRACE (errno)

		if (nEvents == 0)
			continue;

		if (nEvents == -1)
		{
			switch (errno) {
			case	EAGAIN:
			case	EINTR:
				continue;

			case	EFAULT:
			case	EINVAL:
			default:
				TrapError (Fatal, Unix, FnName, "poll");
			}
		}
		/*-------------------------------------------------*/
		/*
		*/
		for (i=0; i < nFiles; i++) {
			if (pipeEvents [i].revents)
			switch (i)
			{
			case	lpExec:
				LpExecEvent (&pipeEvents [i]);
				break;

			case	listenS5:
				listenS5Event (&pipeEvents [i]);
				break;

			case	listenBSD:
				listenBSDEvent (&pipeEvents [i]);
				break;
			}
			if (ProcessInfo.processType != ParentProcess)
				break;
		}
	}
	/*---------------------------------------------------------*/


	EXITP;
	return;
}
/*=================================================================*/

/*=================================================================*/
/*
**	lpExec sent a message to the parent and this is
**	the function that handles it.
*/
static
void
LpExecEvent (pipeEvent_p)

pollfd_t	*pipeEvent_p;
{
	/*----------------------------------------------------------*/
	/*
	*/
		int		msgType;
		char		msgBuffer [128],
				errBuffer [128],
				*systemName_p;
		boolean		badEvent	= False;
		systemInfo	*sip;

	register	uint	revents;
	static		char	FnName []	= "LpExecEvent";
	/*---------------------------------------------------------*/

	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	We are most concerned w/ POLLIN.  POLLIN can occur
	**	w/ POLLHUP and POLLERR.
	*/
	revents = (unsigned int)  pipeEvent_p->revents;

_POLLIN:
	if (! (revents & POLLIN))
		goto	_POLLPRI;


	/*---------------------------------------------------------*/
	/*
	**	This section reads the pipe for the lpExec message
	**	and switches on the message type.
	*/
	if (mread (ProcessInfo.lpExecMsg_p, msgBuffer,
		sizeof (msgBuffer)) == -1)
		TrapError (Fatal, Unix, FnName, "mread");

	msgType = mtype (msgBuffer);

	switch (msgType) {
	case	S_NEW_CHILD:
		(void)	getmessage (msgBuffer, S_NEW_CHILD, &systemName_p);
		sip = GetSystemInfoByName (systemName_p, 0);

		if (sip == NULL) {
			(void)	sprintf (errBuffer,
			"Cannot find system info for '%s'.", systemName_p);

			TrapError (NonFatal, Internal, FnName, errBuffer);
			(void)
			mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
				systemName_p, "", S5_PROTO, MUNKNOWN);
		}
		else 
		{
gotit:
			lpSystemInfoFilter (sip);
			if (NewChild (NULL, sip, MasterChild) == 0) {
				EXITP;
				return;
			}
			FreeSystemInfo (&sip);
		}
		break;

	case	S_SHUTDOWN:
		WriteLogMsg ("Received shutdown instruction from lpExec.");
		Shutdown (False);
		break;

	default:
		(void)	sprintf (msgBuffer,
		"Received unknown message from lpExec, type := %d\n",
		msgType);

		TrapError (NonFatal, Internal, FnName, msgBuffer);
	}


	/*---------------------------------------------------------*/
	/*
	*/
_POLLPRI:
	if (revents & POLLPRI) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLPRI) on lpExec pipe.");
	}
_POLLERR:
	if (revents & POLLERR) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLERR) on lpExec pipe.");
	}
_POLLNVAL:
	if (revents & POLLNVAL) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLNVAL) on lpExec pipe.");
	}
_POLLHUP:
	if (revents & POLLHUP) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Hangup (POLLHUP) has occurred.");
	}
		

	if (badEvent)
		TrapError (Fatal, Internal, FnName, "Cannot recover.");


	EXITP;
	return;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
void
listenS5Event (pipeEvent_p)

pollfd_t	*pipeEvent_p;
{
	/*----------------------------------------------------------*/
	/*
	*/
		char		msgBuffer [128];
		void		*networkMsg_p;
		boolean		badEvent	= False;
		strrecvfd_t	strrecvfd;
		systemInfo	*sip;
		networkMsgTag	*networkMsgTagp;
		jobControlMsg	*jobControlMsgp;
		connectionInfo	*cip;

	register	uint	revents;
	static		char	FnName []	= "listenS5Event";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	We are most concerned w/ POLLIN.  POLLIN can occur
	**	w/ POLLHUP and POLLERR.
	*/
	revents = (unsigned int)  pipeEvent_p->revents;

_POLLIN:
	if (! (revents & POLLIN))
		goto	_POLLPRI;


	/*---------------------------------------------------------*/
	/*
	**	This section reads the file-descriptor off the pipe
	**	and places it in processInfo.
	*/
	if (ioctl (ProcessInfo.listenS5, I_RECVFD, &strrecvfd) == -1) {
		TrapError (NonFatal, Unix, FnName, "ioctl");
		EXITP
		return;
	}

	cip = AcceptConnection (strrecvfd.fd);

	if (! cip)
	{
		(void)	t_close (strrecvfd.fd);
		EXITP
		return;
	}


	/*---------------------------------------------------------*/
	/*
	**	This section reads the first message from the
	**	remote-system which must be the identification message.
	**	Once we have the ident message we map the system-name
	**	to the system-info.  If the machine name is not in
	**	our data base then refuse access.
	*/
	networkMsgTagp =
	ReceiveNetworkMsg (cip, &jobControlMsgp, &networkMsg_p);

	if (networkMsgTagp == NULL)
	{
		FreeConnectionInfo (&cip);
		EXITP
		return;
	}
	if (networkMsgTagp->msgType != SystemIdMsg)
	{
		TrapError (NonFatal, Internal, FnName,
			"Bad message from remote");
		FreeConnectionInfo (&cip);
		FreeNetworkMsg (networkMsgTagp->msgType, &networkMsg_p);
		EXITP
		return;
	}
	sip = GetSystemInfoByName (((systemIdMsg *)networkMsg_p)->systemNamep, 0);

	if (sip == NULL)
	{
		/*
		**  Look for entry with name of '+' with a type of 's5'.
		*/
		sip = GetSystemInfoByName ("+", SystemVSystem);
		if (sip)
		{
			free(sip->systemName_p);
			if (sip->systemName_p = strdup(((systemIdMsg *)networkMsg_p)->systemNamep))
			{
				(void) sprintf (msgBuffer,
				"Using s5 protocol for incoming connection from '%s'.",
					sip->systemName_p);

				TrapError (NonFatal, Internal, FnName, msgBuffer);
				goto gotit;
			}
		}

		(void) sprintf (msgBuffer,
			"Cannot find system info for '%s'.",
			((systemIdMsg *)networkMsg_p)->systemNamep);
		TrapError (NonFatal, Internal, FnName, msgBuffer);
		FreeNetworkMsg (networkMsgTagp->msgType, &networkMsg_p);
		FreeConnectionInfo (&cip);
		EXITP
		return;
	}
gotit:
	FreeNetworkMsg (networkMsgTagp->msgType, &networkMsg_p);


	/*---------------------------------------------------------*/
	/*
	**	Now we are ready to create a new child, whereupon
	**	we simply return to the calling function.
	*/
	lpSystemInfoFilter (sip);
	if (NewChild (cip, sip, SlaveChild) == 0)
	{
		EXITP;
		return;
	}
	FreeConnectionInfo (&cip);
	FreeSystemInfo (&sip);


	/*---------------------------------------------------------*/
	/*
	**	We ignore a hangup on this pipe because the port-monitor
	**	will always hangup after it has handed us a file-descriptor.
	*/
_POLLPRI:
	if (revents & POLLPRI) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLPRI) on lpExec pipe.");
	}
_POLLERR:
	if (revents & POLLERR) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLERR) on lpExec pipe.");
	}
_POLLNVAL:
	if (revents & POLLNVAL) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLNVAL) on lpExec pipe.");
	}
_POLLHUP:
	if (revents & POLLHUP)
		;

	if (badEvent)
		TrapError (Fatal, Internal, FnName, "Cannot recover.");


	EXITP;
	return;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
void
listenBSDEvent (pipeEvent_p)

pollfd_t	*pipeEvent_p;
{
	/*----------------------------------------------------------*/
	/*
	*/
		char		msgBuffer [128];
		boolean		badEvent	= False;
		strrecvfd_t	strrecvfd;
		systemInfo	*sip;
		connectionInfo	*cip;

	register	uint	revents;
	static		char	FnName []	= "listenBSDEvent";

	int r;
	struct netconfig	*configp;
	struct nd_hostservlist	*hostservlistp;


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	We are most concerned w/ POLLIN.  POLLIN can occur
	**	w/ POLLHUP and POLLERR.
	*/
	revents = (unsigned int)  pipeEvent_p->revents;

_POLLIN:
	if (! (revents & POLLIN))
		goto	_POLLPRI;


	/*---------------------------------------------------------*/
	/*
	**	This section reads the file-descriptor off the pipe
	**	and places it in processInfo.
	*/
	if (ioctl (ProcessInfo.listenBSD, I_RECVFD, &strrecvfd) == -1)
	{
		TrapError (NonFatal, Unix, FnName, "ioctl");
		EXITP
		return;
	}

	cip = AcceptConnection (strrecvfd.fd);

	if (cip == NULL)
	{
		(void)	t_close (strrecvfd.fd);
		EXITP
		return;
	}


	/*---------------------------------------------------------*/
	/*
	**	This section tries to determine the name of the
	**	BSD system calling us.  Once we have the name
	**	we map it to the system-info.  If the machine
	**	name is not in our data base then refuse access.
	*/
	sip = GetSystemInfoByAddr (cip, "tcp");
	if (!sip)
	{
		/*
		**  Convert address into name.
		*/
		configp = getnetconfigent ("tcp");
		if (!configp)
		{
			TrapError (NonFatal, Internal, FnName, 
				"Cannot find netconfig entry for connection.");
			FreeConnectionInfo (&cip);
			EXITP
			return;
		}

		r = netdir_getbyaddr (configp, &hostservlistp, &(cip->receiveCall_p->addr));

		if (r || !hostservlistp || !hostservlistp->h_cnt)
		{
			freenetconfigent (configp);
			TrapError (NonFatal, Internal, FnName, 
				"Cannot map address to remote machine name.");
			FreeConnectionInfo (&cip);
			EXITP
			return;
		}
		freenetconfigent (configp);

		/*
		**  Look for entry with name of '*' with a type of 'BSD'.
		*/
		sip = GetSystemInfoByName ("*", BerkeleySystem);
		if (sip)
		{
			free(sip->systemName_p);
			if (sip->systemName_p = strdup(hostservlistp->h_hostservs[0].h_host))
			{
				(void) sprintf (msgBuffer,
				"Using BSD protocol for incoming connection from '%s'.",
					sip->systemName_p);

				TrapError (NonFatal, Internal, FnName, msgBuffer);
				netdir_free ((char *)hostservlistp, ND_HOSTSERVLIST);
				goto gotit;
			}
		}
		netdir_free ((char *)hostservlistp, ND_HOSTSERVLIST);
		TrapError (NonFatal, Internal, FnName, 
			"Cannot find system-info for connection.");
		FreeConnectionInfo (&cip);
		EXITP
		return;
	}

gotit:

	/*---------------------------------------------------------*/
	/*
	**	Now we are ready to create a new child, whereupon
	**	we simply return to the calling function.
	*/
	lpSystemInfoFilter (sip);
	if (NewChild (cip, sip, SlaveChild) == 0) {
		EXITP;
		return;
	}
	FreeConnectionInfo (&cip);
	FreeSystemInfo (&sip);


	/*---------------------------------------------------------*/
	/*
	**	We ignore a hangup on this pipe because the port-monitor
	**	will always hangup after it has handed us a file-descriptor.
	*/
_POLLPRI:
	if (revents & POLLPRI) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLPRI) on lpExec pipe.");
	}
_POLLERR:
	if (revents & POLLERR) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLERR) on lpExec pipe.");
	}
_POLLNVAL:
	if (revents & POLLNVAL) {
		badEvent = True;
		TrapError (NonFatal, Internal, FnName,
		"Bad event (POLLNVAL) on lpExec pipe.");
	}
_POLLHUP:
	if (revents & POLLHUP)
		;

	if (badEvent)
		TrapError (Fatal, Internal, FnName, "Cannot recover.");


	EXITP;
	return;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
void
lpNetChild ()
{
	/*----------------------------------------------------------*/
	/*
	*/
	char *	pathp;
	static	char	FnName []	= "lpNetChild";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	*/
#ifdef	DEBUG
{
	char	fileName [64];
	FILE	*foo;

	(void)	sprintf (fileName, "%s/c%d", LP_LOGS_DIR,  getpid ());
	OPEN_DEBUG_FILE (fileName);
}
#endif

	/*----------------------------------------------------------*/
	/*
	**  Assumptions:
	**	uid = lp
	**	gid = lp
	**	lid = SYS_PRIVATE
	*/
	(void)	umask (0);

	pathp = makepath (Lp_Tmp, SIP->systemName_p, (char *) 0);
	if (! pathp)
	{
		TrapError (Fatal, Unix, FnName, "malloc");
	}
	if (mkdir (pathp, (mode_t) 0755) < 0 && errno != EEXIST)
	{
		TrapError (Fatal, Unix, FnName, "mkdir");
	}
	if (! SetFileLevel (pathp, "SYS_PUBLIC") && errno != ENOPKG)
	{
		TrapError (Fatal, Unix, FnName, "SetFileLevel");
	}
	free (pathp);
	pathp = makepath (Lp_Requests, SIP->systemName_p, (char *) 0);
	if (! pathp)
	{
		TrapError (Fatal, Unix, FnName, "malloc");
	}
	if (mkdir (pathp, (mode_t) 0770) < 0 && errno != EEXIST)
	{
		TrapError (Fatal, Unix, FnName, "mkdir");
	}
	free (pathp);
	/*----------------------------------------------------------*/
	/*
	*/
	switch	(SIP->systemType) {
	case	SystemVSystem:
		svChild ();
		break;

	case	BerkeleySystem:
		bsdChild ();
		break;

	case	NetWareSystem:
		nucChild ();
		break;

	case	UnknownSystem:
	default:
		TrapError (Fatal, Internal, FnName, "Unknown system.");
	}


	EXITP
	return;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
int
NewChild (cip, sip, childRank)

connectionInfo	*cip;
systemInfo	*sip;
processRank	childRank;
{
	/*----------------------------------------------------------*/
	/*
	*/
		int	childPid;
	static	char	FnName []		= "NewChild";

	ENTRYP
	/*----------------------------------------------------------*/
	/*
	*/
	if (sip == NULL)
	{
		EXITP
		return	-1;
	}
	switch	(sip->systemType) {
	case	SystemVSystem:
		childPid = s5NewChild (cip, sip, childRank);
		break;

	case	BerkeleySystem:
		childPid = bsdNewChild (cip, sip, childRank);
		break;

	case	NetWareSystem:
		childPid = nucNewChild (cip, sip, childRank);
		break;

	case	UnknownSystem:
	default:
		TrapError (Fatal, Internal, FnName, "Unknown system.");
	}


	return	childPid;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
int
s5NewChild (cip, sip, childRank)

connectionInfo	*cip;
systemInfo	*sip;
processRank	childRank;
{
	/*----------------------------------------------------------*/
	/*
	*/
		int	childPid,
			pipeFds [2];
		char	msgBuffer [128];
	static	char	FnName []		= "s5NewChild";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	The first thing we need is a new pipe for the
	**	child an lpExec to communicate with.
	**	An error means we cannot fork.
	*/
	if (pipe (pipeFds) == -1)
	{
		TrapError (NonFatal, Unix, FnName, "pipe");

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, "", S5_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		EXITP
		return	-1;
	}


	/*---------------------------------------------------------*/
	/*
	**	Here is where we fork a child.  If there is an error
	**	then we log it and inform lpExec we cannot fork
	**	at this time.
	*/
	childPid = fork ();

	switch	(childPid) {
	/*----------------------------------------------------------*/
	/*
	*/
	case	-1:
		TrapError (NonFatal, Unix, FnName, "fork");

		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, "", S5_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		break;


	/*---------------------------------------------------------*/
	/*
	**	This begins the child code of the successful fork.
	**
	**	Here the ProcessInfo is modified to reflect
	**	that this is a child and the listening pipes
	**	are closed.
	*/
	case	0:
		(void)	sigset (SIGTERM, SIG_HOLD);
		(void)	sigset (SIGCLD, SIG_IGN);

		(void)	mdisconnect (ProcessInfo.lpExecMsg_p);
		(void)	close (ProcessInfo.listenS5);
		(void)	close (ProcessInfo.listenBSD);
		(void)	close (pipeFds [0]);

		ProcessInfo.processType	= ChildProcess;
		ProcessInfo.processRank	= childRank;
		ProcessInfo.processId	= getpid ();
		ProcessInfo.lpExec	= pipeFds [1];
		ProcessInfo.lpExecMsg_p =
		mconnect (NULL, ProcessInfo.lpExec, ProcessInfo.lpExec);
		if (ProcessInfo.lpExecMsg_p == NULL)
			TrapError (Fatal, Unix, FnName, "mconnect");
		ProcessInfo.listenS5	=
		ProcessInfo.listenBSD	= -1;
		CIP			= cip;
		SIP			= sip;
		if (CIP)
			CIP->sip = sip;
		break;


	/*---------------------------------------------------------*/
	/*
	**	The is the parent code of a successful fork.
	**
	**	First the R_NEW_CHILD message along with the
	**	file-descriptor of the new pipe is sent to lpExec.
	**	The parent logs that it has started a child
	**	then the both the file-descritpors are closed
	**	since the parent does not use them.
	**
	**	If an error occurs while sending the file-descriptor
	**	to lpExec then the parent sends an S_SHUTDOWN
	**	message to the child.
	*/
	default:
		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, (childRank == MasterChild ?
		    ProcessInfo.systemName_p : sip->systemName_p), S5_PROTO, MOK) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "mputm");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		if (ioctl (ProcessInfo.lpExec, I_SENDFD, pipeFds [0]) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "ioctl");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		else
		{
			(void)
			sprintf (msgBuffer, "Started child for %s, pid = %d",
				sip->systemName_p, childPid);
			WriteLogMsg (msgBuffer);
		}
		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);
	} /* switch */


	EXITP
	return	childPid;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
int
bsdNewChild (cip, sip, childRank)

connectionInfo	*cip;
systemInfo	*sip;
processRank	childRank;
{
	/*----------------------------------------------------------*/
	/*
	*/
		int	childPid,
			pipeFds [2];
		char	msgBuffer [128];
	static	char	FnName []		= "bsdNewChild";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	The first thing we need is a new pipe for the
	**	child an lpExec to communicate with.
	**	An error means we cannot fork.
	*/
	if (childRank == MasterChild && pipe (pipeFds) == -1)
	{
		TrapError (NonFatal, Unix, FnName, "pipe");

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, "", BSD_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		EXITP
		return	-1;
	}


	/*---------------------------------------------------------*/
	/*
	**	Here is where we fork a child.  If there is an error
	**	then we log it and inform lpExec we cannot fork
	**	at this time.
	*/
	childPid = fork ();

	switch	(childPid) {
	/*----------------------------------------------------------*/
	/*
	*/
	case	-1:
		TrapError (NonFatal, Unix, FnName, "fork");

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);

		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
			sip->systemName_p, "", BSD_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		break;


	/*---------------------------------------------------------*/
	/*
	**	This begins the child code of the successful fork.
	**
	**	Here the ProcessInfo is modified to reflect
	**	that this is a child and the listening pipes
	**	are closed.
	*/
	case	0:
		(void)	sigset (SIGTERM, SIG_HOLD);
		(void)	sigset (SIGCLD, SIG_IGN);

		(void)	mdisconnect (ProcessInfo.lpExecMsg_p);
		ProcessInfo.lpExecMsg_p = NULL;
		ProcessInfo.lpExec = -1;

		(void)	close (ProcessInfo.listenS5);
		(void)	close (ProcessInfo.listenBSD);
		ProcessInfo.listenS5	=
		ProcessInfo.listenBSD	= -1;
		ProcessInfo.processType	= ChildProcess;
		ProcessInfo.processRank	= childRank;
		ProcessInfo.processId	= getpid ();

		if (childRank == MasterChild)
		{
			(void)	close (pipeFds [0]);
			ProcessInfo.lpExec = pipeFds [1];
			ProcessInfo.lpExecMsg_p = mconnect (NULL,
				ProcessInfo.lpExec, ProcessInfo.lpExec);
			if (ProcessInfo.lpExecMsg_p == NULL)
				TrapError (Fatal, Unix, FnName, "mconnect");
		}
		else
			ProcessInfo.lpExec = -1;

		CIP = cip;
		SIP = sip;
		if (CIP)
			CIP->sip = sip;
		break;


	/*---------------------------------------------------------*/
	/*
	**	The is the parent code of a successful fork.
	**
	**	Only if we are a MasterChild do we register
	**	the child with lpExec.
	**
	**	First the R_NEW_CHILD message along with the
	**	file-descriptor of the new pipe is sent to lpExec.
	**	The parent logs that it has started a child
	**	then the both the file-descritpors are closed
	**	since the parent does not use them.
	**
	**	If an error occurs while sending the file-descriptor
	**	to lpExec then the parent sends an S_SHUTDOWN
	**	message to the child.  If that fails then the parent
	**	kill(2)'s the child.
	*/
	default:
		if (childRank == SlaveChild)
			break;

		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, (childRank == MasterChild ?
		    ProcessInfo.systemName_p : sip->systemName_p), BSD_PROTO, MOK) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "mputm");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		if (ioctl (ProcessInfo.lpExec, I_SENDFD, pipeFds [0]) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "ioctl");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		else
		{
			(void)
			sprintf (msgBuffer, "Started child for %s, pid = %d",
				sip->systemName_p, childPid);
			WriteLogMsg (msgBuffer);
		}
		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);
	} /* switch */


	EXITP
	return	childPid;
}
/*=================================================================*/

/*=================================================================*/
/*
*/
static
int
nucNewChild (cip, sip, childRank)

connectionInfo	*cip;
systemInfo	*sip;
processRank	childRank;
{
	/*----------------------------------------------------------*/
	/*
	*/
		int	childPid,
			pipeFds [2];
		char	msgBuffer [128];
	static	char	FnName []		= "nucNewChild";


	ENTRYP
	/*---------------------------------------------------------*/
	/*
	**	The first thing we need is a new pipe for the
	**	child an lpExec to communicate with.
	**	An error means we cannot fork.
	*/
	if (childRank == MasterChild && pipe (pipeFds) == -1)
	{
		TrapError (NonFatal, Unix, FnName, "pipe");

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, "", NUC_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		EXITP
		return	-1;
	}


	/*---------------------------------------------------------*/
	/*
	**	Here is where we fork a child.  If there is an error
	**	then we log it and inform lpExec we cannot fork
	**	at this time.
	*/
	childPid = fork ();

	switch	(childPid) {
	/*----------------------------------------------------------*/
	/*
	*/
	case	-1:
		TrapError (NonFatal, Unix, FnName, "fork");

		if (childRank == SlaveChild)
		{
			EXITP
			return	-1;
		}
		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);

		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
			sip->systemName_p, "", NUC_PROTO, MNOSTART) == -1)
			TrapError (Fatal, Unix, FnName, "mputm");
		break;


	/*---------------------------------------------------------*/
	/*
	**	This begins the child code of the successful fork.
	**
	**	Here the ProcessInfo is modified to reflect
	**	that this is a child and the listening pipes
	**	are closed.
	*/
	case	0:
		(void)	sigset (SIGTERM, SIG_HOLD);
		(void)	sigset (SIGCLD, SIG_IGN);

		(void)	mdisconnect (ProcessInfo.lpExecMsg_p);
		ProcessInfo.lpExecMsg_p = NULL;
		ProcessInfo.lpExec = -1;

		(void)	close (ProcessInfo.listenS5);
		(void)	close (ProcessInfo.listenBSD);
		ProcessInfo.listenS5	=
		ProcessInfo.listenBSD	= -1;
		ProcessInfo.processType	= ChildProcess;
		ProcessInfo.processRank	= childRank;
		ProcessInfo.processId	= getpid ();

		if (childRank == MasterChild)
		{
			(void)	close (pipeFds [0]);
			ProcessInfo.lpExec = pipeFds [1];
			ProcessInfo.lpExecMsg_p = mconnect (NULL,
				ProcessInfo.lpExec, ProcessInfo.lpExec);
			if (ProcessInfo.lpExecMsg_p == NULL)
				TrapError (Fatal, Unix, FnName, "mconnect");
		}
		else
			ProcessInfo.lpExec = -1;

		CIP = cip;
		SIP = sip;
		if (CIP)
			CIP->sip = sip;
		break;


	/*---------------------------------------------------------*/
	/*
	**	The is the parent code of a successful fork.
	**
	**	Only if we are a MasterChild do we register
	**	the child with lpExec.
	**
	**	First the R_NEW_CHILD message along with the
	**	file-descriptor of the new pipe is sent to lpExec.
	**	The parent logs that it has started a child
	**	then the both the file-descritpors are closed
	**	since the parent does not use them.
	**
	**	If an error occurs while sending the file-descriptor
	**	to lpExec then the parent sends an S_SHUTDOWN
	**	message to the child.  If that fails then the parent
	**	kill(2)'s the child.
	*/
	default:
		if (childRank == SlaveChild)
			break;

		if (mputm (ProcessInfo.lpExecMsg_p, R_NEW_CHILD,
		    sip->systemName_p, (childRank == MasterChild ?
		    ProcessInfo.systemName_p : sip->systemName_p), NUC_PROTO, MOK) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "mputm");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		if (ioctl (ProcessInfo.lpExec, I_SENDFD, pipeFds [0]) == -1)
		{
			TrapError (NonFatal, Unix, FnName, "ioctl");
			if (kill (childPid, SIGTERM) == -1)
				TrapError (NonFatal, Unix, FnName, "kill");
			TrapError (Fatal, Internal, FnName,
				"Cannot continue.");
		}
		else
		{
			(void)
			sprintf (msgBuffer, "Started child for %s, pid = %d",
				sip->systemName_p, childPid);
			WriteLogMsg (msgBuffer);
		}
		(void)	close (pipeFds [0]);
		(void)	close (pipeFds [1]);
	} /* switch */


	EXITP
	return	childPid;
}
/*==================================================================*/

/*==================================================================*/
/*
*/
static
void
lpSystemInfoFilter (sip)

systemInfo	*sip;
{
	if (sip->systemType == BerkeleySystem)
	{
		sip->ncProtoFamily_p = strdup (NC_INET);
		if (sip->ndHostServ.h_serv)
			free (sip->ndHostServ.h_serv);
		sip->ndHostServ.h_serv = strdup ("printer");
		sip->ndOption = ND_SET_RESERVEDPORT;
	}
	else if (sip->systemType == SystemVSystem)
	{
#ifdef	NO_CS
		; /*EMPTY*/
#else
		if (sip->ndHostServ.h_serv)
			free (sip->ndHostServ.h_serv);
		sip->ndHostServ.h_serv = strdup ("listen:lp");
#endif
	}
	else
	{
		; /*NetWareSystem service setup*/
	}
	return;
}
/*==================================================================*/

/*==================================================================*/
/*
**	This is the shutdown procedure for the parent.
*/
static
void
Shutdown (notifyLpExec)

boolean	notifyLpExec;
{
		int	fds [2];
		sigaction_t	sigAction;
	static	char	FnName []	= "Shutdown";

	ENTRYP

	if (notifyLpExec &&
	    mputm (ProcessInfo.lpExecMsg_p, S_SHUTDOWN, 0) == -1)
	{
		TrapError (NonFatal, Unix, FnName, "mputm");
	}
	fds [1] = -1;
	fds [0] = ProcessInfo.listenS5;
	(void)	UnmountPipe (fds, PIPE_LISTEN_S5);

	fds [0] = ProcessInfo.listenBSD;
	(void)	UnmountPipe (fds, PIPE_LISTEN_BSD);

	/* kill all descendants, but not the parent */
	(void)	memset (&sigAction, 0, sizeof (sigaction_t));
	sigAction.sa_handler	= SIG_IGN;
	if (sigaction (SIGTERM, &sigAction, NULL) == -1)
		TrapError (NonFatal, Unix, FnName, "sigaction");
	if (sigsend(P_PGID, getpid(), SIGTERM) == -1)
		if (errno != ESRCH)
			TrapError (NonFatal, Unix, FnName, "sigsend");
	/* May need some cleanup for NetWare - do it here */

	EXITP
	Exit (0);
}
/*=================================================================*/

/*==================================================================*/
/*
**	This function is used to trap the SIGTERM signal for
**	the parent.
*/
static
void
SoftwareTerminationTrap (signalNumber)

int	signalNumber;
{
	static	char	FnName [] = "cSoftwareTerminationTrap";

	ENTRYP;
	/*----------------------------------------------------------*/
	/*
	*/
	signalNumber = signalNumber;

	WriteLogMsg ("Received 'SIGTERM' signal, exiting.");


	EXITP
	Exit (0);	/*  Does not return.	*/
}
/*==================================================================*/
