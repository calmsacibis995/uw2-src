/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/poll.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: poll.c,v 1.2 1994/02/18 15:06:33 vtag Exp $"
/*********************************************************************
*
*	Program Name:	 
*	File Name:		poll.c
*	Date Created:	09/05/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			To test the poll call when being used with TLI.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Defines
*-------------------------------------------------------------------*/
#define OPEN	1
#define LISTEN	2
#define CLOSE	3


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include "tlitest.h"
#include "control.h"

#ifdef DOS
#include "poll.h"
#endif

#ifdef OS2
#include "poll.h"
#endif

#ifdef NLM
#include "poll.h"
#endif

#include "polltest.h"


/*--------------------------------------------------------------------
*	Variables global to this module
*-------------------------------------------------------------------*/
int	Count[MAXFDS];
int	Total;


/*====================================================================
*
*	EstablishIpxComms
*
*		This function looks for incoming IPX packets on a predefined
*		socket.  A reply is returned on the fd that is to be used 
*		during testing.  This is kind of like passing a connection
*		with the TLI t_accept function.  It allows the establishment
*		of multiple communications using IPX.
*
*	Return:	void
*
*	Parameters:
*		fds 	(I)	The pollfd structures for each endpoint
*		nfds	(I)	The number of fds on which to establish connections
*		major	(I)	Major test number for synchronization
*		minor	(I)	Minor test number for synchronization
*
*===================================================================*/
void EstablishIpxComms (struct pollfd fds[], int nfds, int major, int minor)
{
	char		addr[ADDRSIZE];
	tbind		bind;
	int		data;
	int		fd;
	int		flags;
	int		i;
	tudata		ud;

	fd = tliopen(IPX,SYNC,NULL,TNOERR,UNBND);
	bind.addr.buf = addr;
	bind.addr.len = ADDRSIZE;
	memcpy(&addr[10],SOCKET,2);
	tlibind(fd,&bind,NULL,TNOERR,IDLE);
	ud.addr.buf = addr;
	ud.addr.maxlen = ADDRSIZE;
	ud.opt.buf = NULL;
	ud.opt.len = 0;
	ud.opt.maxlen = 0;
	ud.udata.buf = (char *) &data;
	ud.udata.maxlen = sizeof(data);
	SrvrSync(nfds,major,minor);
	for (i=0; i<nfds; i++)
	{
		tlircvudata(fd,&ud,&flags,TNOERR,IDLE);
		tlisndudata(fds[i].fd,&ud,TNOERR,IDLE);
	}
	tliclose(fd,TNOERR,UNINIT);
}


/*====================================================================
*
*	IpxPollClnt
*
*		This function provides support in testing the poll function.
*		The tests used in this routine use IPX endpoints and should be
*		run on the client side of the test.
*
*	Return:	void
*
*	Parameters:
*		mode	(I)	Synchronous or Asynchronous
*		major	(I)	Major test number
*
*===================================================================*/
void IpxPollClnt (int mode, int major)
{
	char		addr[ADDRSIZE];
	int		dbuf = 0;
	int		flags;
	int		fd;
	int		i;
	int		minor = 0;
	tudata		ud;

	showProtoStk("TESTING poll",mode);

/*--------------------------------------------------------------------
*	Set up an IPX endpoint
*-------------------------------------------------------------------*/
	fd = tliopen(IPX,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Find out which server socket to communicate with
*-------------------------------------------------------------------*/
	memcpy(addr,Srvraddr,10);
	memcpy(&addr[10],SOCKET,2);
	ud.addr.buf = addr;
	ud.addr.len = ADDRSIZE;
	ud.opt.buf = NULL;
	ud.opt.len = 0;
	ud.opt.maxlen = 0;
	ud.udata.buf = (char *) &dbuf;
	ud.udata.len = sizeof(dbuf);
	ClntSync(Srvraddr,major,minor);
	tlisndudata(fd,&ud,TNOERR,IDLE);
	ud.addr.maxlen = ADDRSIZE;
	ud.opt.maxlen = 0;
	ud.udata.maxlen = sizeof(dbuf);
	tlircvudata(fd,&ud,&flags,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	Send a single IPX packet to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlisndudata(fd,&ud,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	Send multiple IPX packets to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor++);
	for (i=0; i<NUMPACKETS; i++)
		tlisndudata(fd,&ud,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Send zero IPX packets to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor++);

/*--------------------------------------------------------------------
*	Send a single IPX packet to the poll server, server polls in
*	blocking mode
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor++);
	tlisndudata(fd,&ud,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Poll server checking inactive fds
*-------------------------------------------------------------------*/
	showtnum(major,minor);
}


/*====================================================================
*
*	IpxPollSrvr
*
*		Test the poll function using the IPX protocol.  This function
*		runs on the server side of the test.
*
*	Return:	void
*
*	Parameters:
*		mode		(I)	Synchronous or asynchronous
*		nfds		(I)	The number of fds to use
*		major		(I)	Major test number
*
*===================================================================*/
void IpxPollSrvr (int mode, int nfds, int major)
{
	struct pollfd	fds[MAXFDS];
	int				minor = 0;
	int				polled;
	int				totalexpected;

	showProtoStk("TESTING poll",mode);

/*--------------------------------------------------------------------
*	Set up the endpoints to use IPX
*-------------------------------------------------------------------*/
	SetupFds(fds,nfds,IPX,mode);
	EstablishIpxComms(fds,nfds,major,minor);
	SrvrSync(nfds,major,minor++);


/*--------------------------------------------------------------------
*	Single IPX packet received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvIpxData(fds,nfds);

/*--------------------------------------------------------------------
*	Multiple IPX packets received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ZeroCount();
	polled = 0;
	totalexpected = nfds*NUMPACKETS; 
	SrvrSync(nfds,major,minor++);
	while (Total < totalexpected && polled < totalexpected + 5)
	{
		tlipoll(fds,nfds,TIMEOUT,POLLIN);
		RcvIpxData(fds,nfds);
		polled++;
	}
	DisplayCount(nfds);

/*--------------------------------------------------------------------
*	Zero IPX packets received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor++);
	tlipoll(fds,nfds,0,0);

/*--------------------------------------------------------------------
*	Poll in blocking mode, single packet to be received on each active
*	endpoint.  Only expect one of the endpoints to show a POLLIN event
*	on the first poll.
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,-1,POLLIN);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvIpxData(fds,nfds);

/*--------------------------------------------------------------------
*	Tear down the IPX endpoints
*-------------------------------------------------------------------*/
	TeardownFds(fds,nfds);

/*--------------------------------------------------------------------
*	Poll using inactive fds
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlipoll(fds,nfds,0,POLLNVAL);
}


/*====================================================================
*
*	ListenFd
*
*		This function is used to open, listen, and close and close an
*		endpoint.  The endpoint is used for establishing and passing
*		connections.
*
*	Return:	void
*
*	Parameters:
*		control	(I)	Control code indicating the action to perform
*		fds 		(I)	The pollfd structures for each endpoint
*		nfds		(I)	The number of fds on which to establish connections
*		major		(I)	Major test number for synchronization
*		minor		(I)	Minor test number for synchronization
*
*===================================================================*/
void ListenFd (int control, struct pollfd fds[], int nfds, int major, int minor)
{
	tbind	bind;
	int 	i;
	static char		addr[ADDRSIZE];
	static tcall	call;
	static int		fd;

	switch (control)
	{
		case OPEN:
			fd = tliopen(SPX,SYNC,NULL,TNOERR,UNBND);
			bind.addr.buf = addr;
			bind.addr.len = ADDRSIZE;
			bind.addr.maxlen = ADDRSIZE;
			bind.qlen = MAXCONN;
			memcpy(&addr[10],SOCKET,2);
			tlibind(fd,&bind,&bind,TNOERR,IDLE);
			call.addr.maxlen = ADDRSIZE;
			call.addr.buf = addr;
			call.opt.maxlen = 0;
			call.opt.len = 0;
			call.opt.buf = NULL;
			call.udata.maxlen = 0;
			call.udata.len = 0;
			call.udata.buf = NULL;

		case LISTEN:
			SrvrSync(nfds,major,minor);
			for (i=0; i<nfds; i++)
			{

                               
				tlilisten(fd,&call,TNOERR,INCON);
				tliaccept(fd,fds[i].fd,&call,TNOERR,IDLE,DATAXFER);  
			}
			break;

		case CLOSE:
			tliclose(fd,TNOERR,UNINIT);
			break;

		default:
			break;
	}
}


/*====================================================================
*
*	RcvIpxData
*
*		Pick up the IPX data packets received on the active endpoints.
*
*	Return:	
*
*	Parameters:
*		fds 	(I)	The pollfd structures for each endpoint
*		nfds	(I)	The number of fds to process 
*
*===================================================================*/
void RcvIpxData (struct pollfd fds[], int nfds)
{
	char		addr[ADDRSIZE];
	char		data[DATASIZE];
	int		flags;
	int		i;
	tudata	ud;

	ud.addr.buf = addr;
	ud.addr.maxlen = ADDRSIZE;
	ud.opt.buf = NULL;
	ud.opt.len = 0;
	ud.opt.maxlen = 0;
	ud.udata.buf = (char *) data;
	ud.udata.maxlen = sizeof(data);
	for (i=0; i<nfds; i++)
		if (fds[i].revents & POLLIN)
		{
			tlircvudata(fds[i].fd,&ud,&flags,TNOERR,IDLE);
			Count[i]++;
			Total++;
		}
}


/*====================================================================
*
*	RcvSpxData
*
*		Pick up the SPX data packets received on the active endpoints.
*
*	Return:	void
*
*	Parameters:
*		fds 	(I)	The pollfd structures for each endpoint
*		nfds	(I)	The number of fds to process 
*
*===================================================================*/
void RcvSpxData (struct pollfd fds[], int nfds)
{
	char		data[DATASIZE];
	int		flags;
	int		i;

	for (i=0; i<nfds; i++)
		if (fds[i].revents & POLLIN)
		{
			tlircv(fds[i].fd,data,DATASIZE,&flags,TNOERR,DATAXFER);
			Count[i]++;
			Total++;
		}
}


/*====================================================================
*
*	RcvSpxDisc
*
*		Pick up the SPX disconnect requests on the active endpoints.
*		This breaks the established SPX connections.
*
*	Return:	void
*
*	Parameters:
*		fds 	(I)	The pollfd structures for each endpoint
*		nfds	(I)	The number of fds to process 
*
*===================================================================*/
void RcvSpxDisc (struct pollfd fds[], int nfds)
{
	int		i;

	for (i=0; i<nfds; i++)
		if (fds[i].revents & POLLIN)
			tlircvdis(fds[i].fd,NULL,TNOERR,IDLE);
}


/*====================================================================
*
*	SetupFds
*
*		This routine opens and binds the TLI endpoints.  It also
*		initializes the pollfd structures.
*
*	Return:	void
*
*	Parameters:
*		fds 		(I)	The pollfd structures for each endpoint
*		nfds		(I)	The number of fds to setup 
*		procol	(I)	The communications protocol to use
*		mode		(I)	Synchronous or asynchronous
*
*===================================================================*/
void SetupFds (struct pollfd fds[], int nfds, char *procol, int mode)
{
	char	addr[ADDRSIZE];
	tbind	bind;
	int	idx;

	bind.addr.buf = addr;
	bind.addr.len = ADDRSIZE;
	bind.qlen = 0;   
	memset(&addr[10],0,2);
	for (idx=0; idx<nfds; idx++)
	{
		fds[idx].fd = tliopen(procol,mode,NULL,TNOERR,UNBND);
		fds[idx].events = POLLIN;
		fds[idx].revents = 0;
		tlibind(fds[idx].fd,&bind,NULL,TNOERR,IDLE);
	}
	for ( ; idx<MAXFDS; idx++)
		fds[idx].fd = -1;
}


/*====================================================================
*
*	SpxPollClnt
*
*		This function provides support in testing the poll function.
*		The tests used in this routine use SPX endpoints and should be
*		run on the client side of the test.
*
*	Return:	void
*
*	Parameters:
*		mode	(I)	Synchronous or Asynchronous
*		major	(I)	Major test number
*
*===================================================================*/
void SpxPollClnt (int mode, int major)
{
	char	addr[ADDRSIZE];
	tcall	call;
	char	dbuf[2];
	int	fd;
	int	flags = 0;
	int	i;
	int	minor = 0;

	showProtoStk("TESTING poll",mode);

/*--------------------------------------------------------------------
*	Set up an endpoint to use SPX
*-------------------------------------------------------------------*/
	fd = tliopen(SPX,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	
/*--------------------------------------------------------------------
*	Establish a connection with the poll server
*-------------------------------------------------------------------*/
	ClntSync(Srvraddr,major,minor);
	memcpy(addr,Srvraddr,10);
	memcpy(&addr[10],SOCKET,2);
	call.addr.buf = addr;
	call.addr.len = ADDRSIZE;
	call.opt.buf = NULL;
	call.opt.len = 0;
	call.udata.buf = NULL;
	call.udata.len = 0;
	if (mode == SYNC)
		tliconnect(fd,&call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,&call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	Send a single SPX packet to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlisnd(fd,dbuf,sizeof(dbuf),flags,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor++);
	ClntSync(Srvraddr,major,minor);
/*--------------------------------------------------------------------
*	Send a single SPX disconnect packet to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	Establish a connection with the poll server and send multiple SPX
*	packets using the endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,&call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,&call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor++);
	for (i=0; i<NUMPACKETS; i++)
		tlisnd(fd,dbuf,sizeof(dbuf),flags,TNOERR,DATAXFER);

/*--------------------------------------------------------------------
*	Send zero SPX packets
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	Send a single SPX packet, poll server polling in blocking mode
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlisnd(fd,dbuf,sizeof(dbuf),flags,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor++);
	
/*--------------------------------------------------------------------
*	Send a single SPX packet and a disconnect to the poll server
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	tlisnd(fd,dbuf,sizeof(dbuf),flags,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor);
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor++);

/*--------------------------------------------------------------------
*	We're out of here
*-------------------------------------------------------------------*/
	showtnum(major,minor++);
	tliclose(fd,TNOERR,UNINIT);
}


/*====================================================================
*
*	SpxPollSrvr
*
*		Test the poll function using the SPX protocol.  This function
*		runs on the server side of the test.
*
*	Return:	void
*
*	Parameters:
*		mode	(I)	Synchronous or asynchronous
*		nfds	(I)	The number of fds to use
*		major	(I)	Major test number
*
*===================================================================*/
void SpxPollSrvr (int mode, int nfds, int major)
{
	struct pollfd	fds[MAXFDS];
	int				minor = 0;
	int				polled;
	int				totalexpected;

	showProtoStk("TESTING poll",mode);

/*--------------------------------------------------------------------
*	Set up the endpoints to use SPX
*-------------------------------------------------------------------*/
	SetupFds(fds,nfds,SPX,mode);
	ListenFd(OPEN,fds,nfds,major,minor);
	SrvrSync(nfds,major,minor++);

/*--------------------------------------------------------------------
*	Single SPX packet received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvSpxData(fds,nfds);
	SrvrSync(nfds,major,minor);
/*--------------------------------------------------------------------
*	Single SPX disconnect received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvSpxDisc(fds,nfds);

/*--------------------------------------------------------------------
*	Multiple SPX packets received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	ZeroCount();
	polled = 0;
	totalexpected = nfds*NUMPACKETS; 
	ListenFd(LISTEN,fds,nfds,major,minor);
	SrvrSync(nfds,major,minor++);
	while (Total < totalexpected && polled < totalexpected + 5)
	{
		tlipoll(fds,nfds,TIMEOUT,POLLIN);
		RcvSpxData(fds,nfds);
		polled++;
	}
	DisplayCount(nfds);

/*--------------------------------------------------------------------
*	Zero SPX packets received on each active endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,0,0);

/*--------------------------------------------------------------------
*	Poll in blocking mode, single packet to be received on each active
*	endpoint.  Only expect one of the endpoints to show a POLLIN event
*	on the first poll.
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,-1,POLLIN);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvSpxData(fds,nfds);

/*--------------------------------------------------------------------
*	Single SPX packet and a disconnect received on each active
*  endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(nfds,major,minor);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvSpxData(fds,nfds);
	SrvrSync(nfds,major,minor);
	SrvrSync(nfds,major,minor++);
	tlipoll(fds,nfds,TIMEOUT,POLLIN);
	RcvSpxDisc(fds,nfds);
	TeardownFds(fds,nfds);
	ListenFd(CLOSE,fds,nfds,0,0);

/*--------------------------------------------------------------------
*	Poll using inactive fds
*-------------------------------------------------------------------*/
	showtnum(major,minor++);
	tlipoll(fds,nfds,0,POLLNVAL);
}


/*====================================================================
*
*	TeardownFds
*
*		Close the endpoints being used by TLI.
*
*	Return:	void
*
*	Parameters:
*		fds 		(I)	The pollfd structures for each endpoint
*		nfds		(I)	The number of fds to close
*
*===================================================================*/
void TeardownFds (struct pollfd fds[], int nfds)
{
	int	idx;

/*--------------------------------------------------------------------
*	Close the active endpoints
*-------------------------------------------------------------------*/
	for (idx=0; idx<nfds; idx++)
	{
		tliclose(fds[idx].fd,TNOERR,UNINIT);
		fds[idx].fd = -1;
	}
}


/*====================================================================
*
*	tlipoll
*
*		This function displays a message, calls the poll function, and
*		checks the status of the call.
*
*	Return:	void
*
*	Parameters:
*		fds 		(I)	The pollfd structures for each endpoint
*		nfds		(I)	The number of fds to check for events
*		timeout	(I)	The amount of time to wait for events to occur
*		expected	(I)	Expected event to be found on the endpoints
*
*===================================================================*/
void tlipoll (struct pollfd fds[], int nfds, int timeout, short expected)
{
	int	ccode;
	int	i;

/*--------------------------------------------------------------------
*	Display a message and make the call
*-------------------------------------------------------------------*/
	showmsg(CALLFMT,"poll");
	if (Verbose)
		printf("\n");
	ccode = poll(fds,(unsigned long)nfds,timeout);

/*--------------------------------------------------------------------
*	Check the results
*-------------------------------------------------------------------*/
	if (ccode != nfds)
		printf("  At least one fd did not receive an event\n");
	for (i=0; i<nfds; i++)
		if (fds[i].revents == 0)
			printf("  No events found on fds[%d]\n",i);
		else if (fds[i].revents != expected)
			printf("  Unexpected event found on fds[%d]\n",i);
}


/*====================================================================
*
*	DisplayCount
*
*		This function displays the values contained in the Count array.
*
*	Return:	void
*
*		nfds		(I)	The number of fds to display packet counts for
*
*===================================================================*/
void DisplayCount (int nfds)
{
	int i;

	printf("Packet counts by fd\n");
	for (i=0; i<nfds; i++)
		printf("  fds[%d] = %d\n",i,Count[i]);
}


/*====================================================================
*
*	ZeroCount
*
*		This function initializes the Count array to 0.
*
*	Return:	void
*
*===================================================================*/
void ZeroCount ()
{
	int i;

	for (i=0; i<MAXFDS; i++)
		Count[i] = 0;
	Total = 0;
}



