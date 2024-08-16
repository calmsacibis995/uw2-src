/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/control.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: control.c,v 1.2 1994/02/18 15:06:02 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		control.c
*	Date Created:	04/12/90
*	Version:			
*	Programmer(s):	Rick Johnson
*	Purpose:			Handle server and client control functions.  This
*						includes address detection and synchronization.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include <time.h>
#include "tlitest.h"
#include "control.h"


/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
#ifdef NLM
void CRescheduleLast (void);
#endif


/*--------------------------------------------------------------------
*	Global Variables
*-------------------------------------------------------------------*/
int			Ctrlfd;
int			CSPXfd; /* SPX(II) based sync. endpoint */
static tudata	Rud;
static tudata	Sud;


/*====================================================================
*
*	broadcast
*
*		This function broadcasts a message to a specified socket.
*
*	Return:	None
*
*	Parameters:
*		network	(I)	Network to broadcast on
*		socket	(I)	Socket to broadcast to
*		data		(I)	Data to send
*
*===================================================================*/
void broadcast (char *network, char *socket, void *data, int size)
{
	static char		addr[ADDRSIZE];
	static int		setup = FALSE;
	static tudata	ud;

	if (!setup)
	{
		ud.addr.buf = addr;
		ud.addr.len = ADDRSIZE;
		ud.opt.buf = NULL;
		ud.opt.len = 0;
		memcpy(&addr[4],NODE,6);
		setup = TRUE;
	}
	ud.udata.buf = (char *) data;
	ud.udata.len = size;
	memcpy(addr,network,4);
	memcpy(&addr[10],socket,2);
#ifdef MDEBUG
	printf("Broadcast to addr = ");
	for(i = 0 ; i<ADDRSIZE; i++)
		printf("0x%x ", ud.addr.buf[i]);
	printf("\n");
#endif
	sndudata(&ud);
}

/*====================================================================
*
*	EndCtrl
*
*		This function shuts down the endpoint that is being used for
*		server address detection and process synchronization.
*
*	Return:	None
*
*	Parameters:	None
*
*===================================================================*/
void EndCtrl ()
{

/*--------------------------------------------------------------------
*	Close the control fd
*-------------------------------------------------------------------*/
#ifndef IPX_SYNC
	t_snddis(CSPXfd,NULL);
	t_close(CSPXfd);
#endif
	t_close(Ctrlfd);
}

/*====================================================================
*
*	FindSrvr
*
*		This function finds the address of the TLI server.
*
*	Return:	None
*
*	Parameters:
*		srvraddr	(O)	Found TLI server address
*
*===================================================================*/
void FindSrvr (uint8 *srvraddr)
{
	int			flags;
	int			noinfo = TRUE;
	short int	rdata;
	short int	sdata;
	int			size;
	long			timer = ADDRTIMEOUT;
#ifndef IPX_SYNC
	tinfo		info;		/* info structure for CSPXfd endpoint */
	tcallptr	call;		/* call structure for CSPXfd endpoint */
#endif

/*--------------------------------------------------------------------
*	Broadcast on the networks we use
*-------------------------------------------------------------------*/
	sdata = ADDRREQ;
	size = sizeof(sdata);
	broadcast(Net1,SRVRCTRLSKT,&sdata,size);
	broadcast(Net2,SRVRCTRLSKT,&sdata,size);

/*--------------------------------------------------------------------
*	Look for a server response
*-------------------------------------------------------------------*/
	Rud.addr.buf = (char *) srvraddr;
	Rud.addr.len = ADDRSIZE;
	Rud.udata.buf = (char *) &rdata;
	Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
	Rud.addr.maxlen = ADDRSIZE;
	Rud.opt.buf = opt;
	Rud.opt.maxlen = MAXOPT;
#endif

	while (noinfo && timer > 0)
	{
		if (rcvudata(&Rud,&flags))
		{
			Delaytime(DELAYTIME);
			timer -= DELAYTIME;
			if (!(timer % 1000))
			{
				broadcast(Net1,SRVRCTRLSKT,&sdata,size);
				broadcast(Net2,SRVRCTRLSKT,&sdata,size);
			}
		}

/*--------------------------------------------------------------------
*	Handle received message
*-------------------------------------------------------------------*/
		else switch (rdata)
		{
			case ADDRACK:					/* Server address arrived */
				noinfo = FALSE;
				break;
			case RESET:						/* Server busy, reset timer */
				timer = ADDRTIMEOUT;
				break;
			default:							/* Unexepected message */
				printf("data = %x\n",rdata);
				ScreamAndDie("FindSrvr: Unexpected msg");
				break;
		}
	}

/*--------------------------------------------------------------------
*	Check for a timeout error
*-------------------------------------------------------------------*/
	if (timer < 100)
		ScreamAndDie("FindSrvr: Address ack timeout");

/*--------------------------------------------------------------------
*	Wait for the server to process other address requests
*-------------------------------------------------------------------*/
	noinfo = TRUE;
	timer = ADDRTIMEOUT;
	while (noinfo && timer > 0)
	{
		Rud.addr.buf = (char *) srvraddr;
		Rud.addr.maxlen = ADDRSIZE;
		Rud.udata.buf = (char *) &rdata;
		Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
		Rud.opt.buf = opt;
		Rud.opt.maxlen = MAXOPT;
#endif
		if (rcvudata(&Rud,&flags))
		{
			Delay();
			timer -= DELAYTIME;
		}

/*--------------------------------------------------------------------
*	Handle received message
*-------------------------------------------------------------------*/
		else switch (rdata)
		{
			case ADDRDONE:					/* Server ready to continue */
				noinfo = FALSE;
				break;
			case RESET:						/* Server busy, reset timer */
				timer = ADDRTIMEOUT;
				break;
			default:							/* Unexepected message */
				printf("data = %x\n",rdata);
				ScreamAndDie("FindSrvr: Invalid msg");
				break;
		}
	}

/*--------------------------------------------------------------------
*	Check for a timeout error
*-------------------------------------------------------------------*/
	if (timer <= 0)
		ScreamAndDie("FindSrvr: Address done timeout");

#ifndef IPX_SYNC
 
/*--------------------------------------------------------------------
* Begin Connection based endpoint for Syncronization to replace IPX
* based sync. calls.
*
* Modified by: Jon T. Matsukawa 3/8/93
*-------------------------------------------------------------------*/
 
 
	printf("Establishing SPX(II) based sync. setup\n");
	printf("--------------------------------------\n\n");
 
	CSPXfd = tliopen(SPX,O_RDWR,&info,TNOERR,UNBND);
	tlibind(CSPXfd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(CSPXfd,T_CALL,T_ALL,TNOERR,IDLE);
	call->addr.len = call->addr.maxlen;
	memcpy(call->addr.buf,srvraddr,10);
	memcpy(&(call->addr.buf[10]),SPXCTRLSKT,2);
	tliconnect(CSPXfd,call,NULL,TNOERR,DATAXFER);
	tlifree(CSPXfd,(char *)call,T_CALL,TNOERR,DATAXFER);
	 
	printf("Sync. endpoint setup complete.\n");

#endif
}

/*====================================================================
*
*	InitCtrl
*
*		This function sets up an endpoint to be used for determining
*		the server's location and for synchronization of the test
*		processes on the client side.
*
*	Return:	None
*
*	Parameters:
*		mode	(I)	SRVR or CLNT mode
*
*===================================================================*/
void InitCtrl (char mode)
{
	char	addr[ADDRSIZE];
	tbind	bind;

/*--------------------------------------------------------------------
*	Open the control fd
*-------------------------------------------------------------------*/
	Ctrlfd = t_open(IPX,ASYNC,NULL);
	if (Ctrlfd == -1)
		TScreamAndDie("t_open");

/*--------------------------------------------------------------------
*	Bind the control fd
*-------------------------------------------------------------------*/
	if (mode == SRVR)
		memcpy(&addr[10],SRVRCTRLSKT,2);
	else
		memcpy(&addr[10],CLNTCTRLSKT,2);
	bind.addr.buf = addr;
	bind.addr.len = ADDRSIZE;
	if (t_bind(Ctrlfd,&bind,NULL) == -1)
		TScreamAndDie("t_bind");

/*--------------------------------------------------------------------
*	Set up the TLI t_unitdata structure 
*-------------------------------------------------------------------*/
	Rud.addr.maxlen = ADDRSIZE;
	Rud.opt.buf = NULL;
	Rud.opt.len = 0;
	Rud.opt.maxlen = 0;
	Sud.addr.len = ADDRSIZE;
	Sud.opt.buf = NULL;
	Sud.opt.len = 0;
	Sud.opt.maxlen = 0;
}


/*====================================================================
*
*	rcvudata
*
*		This routine makes a TLI t_rcvudata call and checks for errors.
*
*	Return:	None
*
*	Parameters:
*		ud			(I)	Pointer to a t_unitdata structure
*		flags		(I)	Pointer to any flags returned by t_rcvudata
*
*===================================================================*/
int rcvudata (tudataptr ud, int *flags)
{
	int	rcode;

	ud->addr.maxlen = ADDRSIZE;
	ud->opt.buf = opt;
	ud->opt.maxlen = MAXOPT;
	ud->udata.len = 0;

	rcode = t_rcvudata(Ctrlfd,ud,flags);
	if (rcode && t_errno != TNODATA)
		TScreamAndDie("t_rcvudata");

	return ( rcode );
}

/*====================================================================
*
*	ScreamAndDie
*
*		This routine should be used after an unexpected error occurs
*		and the program should be exited after displaying an error
*		message.
*
*	Return:	None
*
*	Parameters:
*		msg	(I)	Part of the error message to be output
*
*===================================================================*/
void ScreamAndDie (char *msg)
{
	printf("%s\n",msg);
	EndCtrl();
	exit(50);
}

/*====================================================================
*
*	sndudata
*
*		This routine makes a TLI t_sndudata call and checks for errors.
*
*	Return:	None
*
*	Parameters:
*		ud			(I)	Pointer to a t_unitdata structure
*
*===================================================================*/
void sndudata (tudataptr ud)
{
	if (t_sndudata(Ctrlfd,ud))
		TScreamAndDie("t_sndudata");
}

/*====================================================================
*
*	SrvrAdd
*
*		This function is used by the server to send the server address
*		to requesting clients.
*
*	Return:	None
*
*	Parameters:
*		clients	(I)	The number of clients sending address requests
*
*===================================================================*/
void SrvrAdd (int clients)
{
	int			flags;
	char		raddr[ADDRSIZE];
	short int	rdata;
	char		saddr[ADDRSIZE];
	short int	sdata;
	int			size;
	long		timer = ADDRTIMEOUT;
#ifndef IPX_SYNC
	tbindptr	bind;
	tcallptr	call;
#endif

/*--------------------------------------------------------------------
*	Receive address requests and respond to them
*-------------------------------------------------------------------*/
	Rud.addr.buf = raddr;
	Rud.udata.buf = (char *) &rdata;
	Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
	Rud.addr.maxlen = ADDRSIZE;
	Rud.opt.buf = opt;
	Rud.opt.maxlen = MAXOPT;
#endif
	size = sizeof(sdata);
	Sud.addr.buf = saddr;
	Sud.udata.buf = (char *) &sdata;
	Sud.udata.len = size;
	while (clients && timer > 0)
	{
		if (rcvudata(&Rud,&flags))
		{
			Delay();
			timer -= DELAYTIME;
		}
		else if (rdata == ADDRREQ)
		{
			clients--;
			timer = ADDRTIMEOUT;
			sdata = ADDRACK;
			memcpy(saddr,raddr,12);
			sndudata(&Sud);
			if (clients)
			{
				sdata = RESET;
				broadcast(Net1,CLNTCTRLSKT,&sdata,size);
				broadcast(Net2,CLNTCTRLSKT,&sdata,size);
			}
		}
		else
		{
			printf("data = %x\n",rdata);
			ScreamAndDie("SrvrAdd: Unexpected msg");
		}
	}

/*--------------------------------------------------------------------
*	Check for a timeout error
*-------------------------------------------------------------------*/
	if (timer <= 0)
		ScreamAndDie("SrvrAdd: Address request timeout");

/*--------------------------------------------------------------------
*	Let all the clients proceed
*-------------------------------------------------------------------*/
	sdata = ADDRDONE;
	broadcast(Net1,CLNTCTRLSKT,&sdata,size);
	broadcast(Net2,CLNTCTRLSKT,&sdata,size);

#ifndef IPX_SYNC
/*--------------------------------------------------------------------
* Begin Connection based endpoint for Syncronization to replace IPX
* based sync. calls.
*
* Modified by: Jon T. Matsukawa 3/8/93
*-------------------------------------------------------------------*/
 
	printf("Establishing SPX(II) based sync. setup\n");
	printf("--------------------------------------\n\n");
 
	CSPXfd = tliopen(SPX,O_RDWR,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(CSPXfd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	memcpy(&(bind->addr.buf[10]),SPXCTRLSKT,2);
	bind->qlen = MAXCONN;
	tlibind(CSPXfd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(CSPXfd,T_CALL,T_ALL,TNOERR,IDLE);
	tlilisten(CSPXfd,call,TNOERR,INCON);
	tliaccept(CSPXfd,CSPXfd,call,TNOERR,DATAXFER,DATAXFER);

	tlifree(CSPXfd,(char *)call,T_CALL,TNOERR,DATAXFER);
	tlifree(CSPXfd,(char *)bind,T_BIND,TNOERR,DATAXFER);

	printf("Sync. endpoint setup complete.\n");
#endif

}

/*====================================================================
*
*	TScreamAndDie
*
*		This routine should be used after an unexpected error in a TLI
*		function is found and the program should be exited.
*
*	Return:	None
*
*	Parameters:
*		msg	(I)	Part of the error message to be output
*
*===================================================================*/
void TScreamAndDie (char *msg)
{
	t_error(msg);
	EndCtrl();
	exit(60);
}


#ifdef IPX_SYNC
/*====================================================================
*
*	SrvrSync
*
*		This function is used by the server process to synchronize
*		the TLI tests.	 One server can synchronize with more than one
*		client.
*
*	Return:	None
*
*	Parameters:
*		clients	(I)	The number of clients to synchronize with
*		major		(I)	Major test number
*		minor		(I)	Minor test number
*
*===================================================================*/
void SrvrSync (int clients, short int major, short int minor)
{
	int		flags;
	int		rcode=0;
	char		raddr[ADDRSIZE];
	sydata	rdata;
	char		saddr[ADDRSIZE];
	sydata	sdata;
	int		size;
	long		timer = SYNCTIMEOUT;

/*--------------------------------------------------------------------
*	Look for synchronization requests
*-------------------------------------------------------------------*/
	Rud.addr.buf = raddr;
	Rud.udata.buf = (char *) &rdata;
	Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
	Rud.opt.buf = opt;
	Rud.opt.maxlen = MAXOPT;
#endif
	size = sizeof(sdata);
	Sud.addr.buf = saddr;
	Sud.udata.buf = (char *) &sdata;
	Sud.udata.len = size;
#ifdef MDEBUG
	printf("IPX SrvrSync: WAIT..");
#endif
	while ((clients > 0) && (timer > 0))
	{
		if (rcvudata(&Rud,&flags))
		{
			while(Delay()==0)
				;  
			timer -= DELAYTIME;
		}
		else
		{
#ifdef MDEBUG
	printf("Received..");
#endif
			timer = SYNCTIMEOUT;
			if (rdata.msg == SYNCREQ)
			{

/*--------------------------------------------------------------------
*	Ack a sync request if the major and minor test numbers are correct
*-------------------------------------------------------------------*/
				if (GETINT16(rdata.major) == major && 
					GETINT16(rdata.minor) == minor) {
					clients--;
					sdata.msg = SYNCACK;
				}

/*--------------------------------------------------------------------
*	Nack a sync request if the major or minor test numbers are wrong
*-------------------------------------------------------------------*/
				else
				{
					printf("Invalid test number received\n");
					sdata.msg = SYNCNACK;
				}
				memcpy(saddr,raddr,12);
				sdata.major = major;
				sdata.minor = minor;
#ifdef MDEBUG
	printf("SEND..");
#endif
				sndudata(&Sud);
			}
			else
				printf("Invalid message received - 0x%x\n",rdata.msg);
		}
	}

/*--------------------------------------------------------------------
*	Check for a timeout error, maybe a sync request wasn't received
*-------------------------------------------------------------------*/
	if (timer <= 0)
		{
		ScreamAndDie("SrvrSync: sync request timeout");
		}
/*--------------------------------------------------------------------
*	Done syncing, let everything go
*-------------------------------------------------------------------*/
	sdata.msg = SYNCDONE;
#ifdef MDEBUG
	printf("SEND..");
#endif
	broadcast(Net1,CLNTCTRLSKT,&sdata,size);
	broadcast(Net2,CLNTCTRLSKT,&sdata,size);
#ifdef MDEBUG
	printf("DONE\n");
#endif
}

/*====================================================================
*
*	ClntSync
*
*		This function is used by the client process to synchronize
*		the TLI tests.	 One client can synchronize with one server.
*
*	Return:	None
*
*	Parameters:
*		srvraddr	(I)	The address of the server to sync with
*		major		(I)	Major test number
*		minor		(I)	Minor test number
*
*===================================================================*/
void ClntSync (uint8 *srvraddr, short int major, short int minor)
{
	int		flags;
	int		retries=0;
	int		delrcode=FALSE;
	char		msg[MSGSIZE];
	char		raddr[ADDRSIZE];
	sydata	rdata;
	char		saddr[ADDRSIZE];
	sydata	sdata;
	long		timer = SYNCTIMEOUT;

/*--------------------------------------------------------------------
*	Send a synchronization request
*-------------------------------------------------------------------*/
	Rud.addr.buf = raddr;
	Rud.udata.buf = (char *) &rdata;
	Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
	Rud.opt.buf = opt;
	Rud.opt.maxlen = MAXOPT;
#endif
	Sud.addr.buf = saddr;
	Sud.udata.buf = (char *) &sdata;
	Sud.udata.len = sizeof(sdata);
	sdata.msg = SYNCREQ;
	sdata.major = GETINT16(major);
	sdata.minor = GETINT16(minor);
	memcpy(saddr,srvraddr,ADDRSIZE);
#ifdef MDEBUG
	printf("IPX ClntSync: SEND..");
#endif
	sndudata(&Sud);

/*--------------------------------------------------------------------
*	Wait for a reply to the request
*-------------------------------------------------------------------*/
#ifdef MDEBUG
	printf("Wait..");
#endif
	while (	(rcvudata(&Rud,&flags)==-1) && (timer > 0) && 
			(delrcode==FALSE) && (retries<5) )
	{
		if(retries != 0)
		{
			sndudata(&Sud);
			printf("Sync Retry # %d\n",retries);
		}
		retries++; 
		delrcode = Delay();
	}
#ifdef MDEBUG
	printf("Received..");
#endif
	if (retries==5)
	{
		EndCtrl();
		exit(5);
	}
	
/*--------------------------------------------------------------------
*	Check for a timeout error
*-------------------------------------------------------------------*/
	if (timer <= 0)
		ScreamAndDie("ClntSync: Ack timeout");

/*--------------------------------------------------------------------
*	Check for sync request reply
*-------------------------------------------------------------------*/
	switch (rdata.msg)
	{
		case SYNCNACK:
			sprintf(msg,"%d.%d not synchronized",major,minor);
			ScreamAndDie(msg);
			break;
		case SYNCACK:
			Rud.addr.buf = raddr;
			Rud.udata.buf = (char *) &rdata;
			Rud.udata.maxlen = sizeof(rdata);
#ifdef NWU
			Rud.opt.buf = opt;
			Rud.opt.maxlen = MAXOPT;
#endif
#ifdef MDEBUG
	printf("Wait..");
#endif
			while (rcvudata(&Rud,&flags))
				;
#ifdef MDEBUG
	printf("Received..");
#endif
			if (rdata.msg != SYNCDONE)
				ScreamAndDie("ClntSync: SYNCDONE not received");
			break;
		default:
			sprintf(msg,"ClntSync: Unexpected msg received, 0x%x",rdata.msg);
			ScreamAndDie(msg);
			break;
	}
#ifdef MDEBUG
	printf("DONE\n");
#endif
}

#else		/*IPX_SYNC   use SPX for synchronization */

/*====================================================================
*
*   SrvrSync
*
*	   This function is used by the server process to synchronize
*	   the TLI tests.   One server can synchronize with more than one
*	   client.
*
*   Return: None
*
*   Parameters:
*	   clients (I) The number of clients to synchronize with
*	   major	   (I) Major test number
*	   minor	   (I) Minor test number
*
*===================================================================*/
void SrvrSync (int clients, short int major, short int minor)
{
 
	char	sndsyncbuf[40];
	char	rcvsyncbuf[40];
	char	expbuf[40];
	int		flags;
/*--------------------------------------------------------------------
*   Send a synchronization request
*-------------------------------------------------------------------*/
	memset(sndsyncbuf,'\0',40);
	memset(rcvsyncbuf,'\0',40);
	memset(expbuf,'\0',40);
 
	sprintf(sndsyncbuf,"Server on Test %d.%d",major,minor);
	sprintf(expbuf,   "Client on Test %d.%d",major,minor);
#ifdef MDEBUG
	printf("SrvrSync: WAIT..");
#endif
	t_rcv(CSPXfd,rcvsyncbuf,sizeof(rcvsyncbuf),&flags);
#ifdef MDEBUG
	printf("Received..");
#endif
 
/*--------------------------------------------------------------------
* Check if server sync numbers match client sync numbers
*-------------------------------------------------------------------*/
 
	if(strcmp(rcvsyncbuf,expbuf)!=0)
	{
		printf("%s\n%s\n",rcvsyncbuf,expbuf);
		printf("receive buffer size %d\n",strlen(rcvsyncbuf));
		sprintf(sndsyncbuf,"%d.%d not synchronized",major,minor);
		ScreamAndDie(sndsyncbuf);
	}
 
#ifdef MDEBUG
	printf("SEND..");
#endif

	t_snd(CSPXfd,sndsyncbuf,strlen(sndsyncbuf)+1,0);
#ifdef MDEBUG
	printf("DONE\n");
#endif
 
	return;
 
}

/*====================================================================
*
*   ClntSync
*
*	   This function is used by the client process to synchronize
*	   the TLI tests.   One client can synchronize with one server.
*
*   Return: None
*
*   Parameters:
*	   srvraddr	(I) The address of the server to sync with
*	   major	   (I) Major test number
*	   minor	   (I) Minor test number
*
*===================================================================*/
void ClntSync (uint8 *srvraddr, short int major, short int minor)
{
 
	char	sndsyncbuf[40];
	char	rcvsyncbuf[40];
	char	expbuf[40];
	int		flags;
/*--------------------------------------------------------------------
*   Send a synchronization request
*-------------------------------------------------------------------*/
	memset(sndsyncbuf,'\0',40);
	memset(rcvsyncbuf,'\0',40);
	memset(expbuf,'\0',40);
 
	sprintf(sndsyncbuf,"Client on Test %d.%d",major,minor);
	sprintf(expbuf,   "Server on Test %d.%d",major,minor);
#ifdef MDEBUG
	printf("SPX ClntSync: SEND..");
#endif
	t_snd(CSPXfd,sndsyncbuf,strlen(sndsyncbuf)+1,0);
#ifdef MDEBUG
	printf("Wait..");
#endif
	t_rcv(CSPXfd,rcvsyncbuf,sizeof(rcvsyncbuf),&flags);
#ifdef MDEBUG
	printf("Received..");
#endif
 
/*--------------------------------------------------------------------
* Check if server sync numbers match client sync numbers
*-------------------------------------------------------------------*/
 
	if(strcmp(rcvsyncbuf,expbuf)!=0) {
		printf("%s\n%s\n",rcvsyncbuf,expbuf);
		sprintf(sndsyncbuf,"%d.%d not synchronized",major,minor);
		ScreamAndDie(sndsyncbuf);
	}

#ifdef MDEBUG
	printf("DONE\n");
#endif
	return;
}

 




#endif		/* IPX_SYNC */
