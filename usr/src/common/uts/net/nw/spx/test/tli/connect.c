/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/connect.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: connect.c,v 1.2 1994/02/18 15:05:58 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		connect.c
*	Date Created:	04/23/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test TLI t_connect.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include <string.h>
#include "tlitest.h"
#include "control.h"
#include "tliprcol.h"


/*====================================================================
*
*	connectclnt
*
*		This function tests the TLI t_connect function.  It is intended
*		to be called by a TLI client test program.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session protocol stack
*		ps2			non-Session protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void connectclnt (char *ps1, char *ps2, int mode, int major)
{
	char		dbuf[DATASIZE];
	int			fd;
	int			fd2;
	tinfo		info;
	int			minor = 1;
	tcallptr	rcvcall;
	tcallptr	sndcall;

	showProtoStk("TESTING t_connect",mode);

/*--------------------------------------------------------------------
*	1. Establish a connection and use it
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	LoadAddr(sndcall->addr.buf,SRVR);
	sndcall->addr.len = sndcall->addr.maxlen;
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
	{
		tliconnect(fd,sndcall,rcvcall,TNOERR,DATAXFER);
		checktcall(rcvcall);
	}
	else
	{
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	2. Check how t_connect handles the rejection of a connect request
*
*	Note: DOS IPX/SPX acknowledges the receipt of a packet by a
*	kneejerk reflex.  When a packet is received it is automatically
*	acknowledged and is not handled by the application through one of
*	the higher level TLI function calls.
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC){

#ifdef SPXII
        tliconnect(fd,sndcall,rcvcall,TLOOK,OUTCON);
#else
		tliconnect(fd,sndcall,rcvcall,TNOERR,DATAXFER);
#endif

	}
	else
	{
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
#ifdef SPXII
		tlircvconnect(fd,rcvcall,TLOOK,OUTCON);
#else
		tlircvconnect(fd,rcvcall,TNOERR,DATAXFER);
#endif
	}


	tlircvdis(fd,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);


/*--------------------------------------------------------------------
*	3. Cause a TBADF error by using an invalid endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
    fd2 = 100;
	tliconnect(fd2,sndcall,rcvcall,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	4. Cause a TOUTSTATE error from the T_UNBND state
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliconnect(fd,sndcall,NULL,TOUTSTATE,UNBND);
	
/*--------------------------------------------------------------------
*	5. Cause a TOUTSTATE error from the T_DATAXFER state
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);

	ClntSync(Srvraddr,major,minor);

	if (mode == SYNC){
		tliconnect(fd,sndcall,NULL,TNOERR,DATAXFER);

	}
	else
	{
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}

	tliconnect(fd,sndcall,NULL,TOUTSTATE,DATAXFER);
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	
/*--------------------------------------------------------------------
*	6. Cause a TBADADDR error by passing a bad address
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	sndcall->addr.len = 1;
	tliconnect(fd,sndcall,NULL,TBADADDR,IDLE);
	sndcall->addr.len = sndcall->addr.maxlen;
	
/*--------------------------------------------------------------------
*	7. Cause a TBUFOVFLW error by not allowing enough room to receive
*	the incoming address
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	rcvcall->addr.maxlen--;
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,sndcall,rcvcall,TBUFOVFLW,DATAXFER);
	else
	{
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,rcvcall,TBUFOVFLW,DATAXFER);
	}

	rcvcall->addr.maxlen++;
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Cause a TBUFOVFLW error by using a freed t_call structure
*-------------------------------------------------------------------*/
/*
	relinquish();
	minor++;
	showtnum(major,minor);
**	tlifree(fd,(char *)rcvcall,T_CALL,TNOERR,IDLE);
	rcvcall->addr.buf = NULL;
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,sndcall,rcvcall,TBUFOVFLW,DATAXFER);
	else
	{
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,rcvcall,TBUFOVFLW,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
*/
	tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
	tlifree(fd,(char *)rcvcall,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	
/*--------------------------------------------------------------------
*	8. Cause a TNOTSUPPORT error by opening an endpoint that uses a
*	connectionless protocol.
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
#ifndef NWU
	/*
	** t_alloc with a IPX fd (connectionless) will fail using T_CALL as the
	** structure type.  Donot allocate structures if running on UNIX SVR4.2 
	*/
	sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
#else
	sndcall = NULL;
#endif
	tliconnect(fd,sndcall,NULL,TNOTSUPPORT,IDLE);
	tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);


/*--------------------------------------------------------------------
*	Server sends disconnect to an already closed endpoint
*	connectionless protocol.
*-------------------------------------------------------------------*/
 /*	relinquish();
	minor++;
	showtnum(major,minor);
	if (mode == ASYNC)
	{

		
		fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
		tlibind(fd,NULL,NULL,TNOERR,IDLE);
		sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
		rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
		LoadAddr(sndcall->addr.buf,SRVR);
		sndcall->addr.len = sndcall->addr.maxlen;
		tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
		tlisnddis(fd,NULL,TNOERR,IDLE);
		Delaytime(1000);
		ClntSync(Srvraddr,major,minor);
		tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
		tliclose(fd,TNOERR,UNINIT);
		ClntSync(Srvraddr,major,minor);

 
	} */
/*-----------------------------------------------------------------
* This is a special test for the NLM side to test memory usage.
* Client connects delays 5 seconds and reconnects.
*------------------------------------------------------------------*/
/*  	if (mode == SYNC)
	{
		relinquish();
		minor++;
		showtnum(major,minor);

		while(!kbhit())
		{

			fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
			tlibind(fd,NULL,NULL,TNOERR,IDLE);
			sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
			rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
			LoadAddr(sndcall->addr.buf,SRVR);
			sndcall->addr.len = sndcall->addr.maxlen;
			ClntSync(Srvraddr,major,minor);
			tliconnect(fd,sndcall,rcvcall,TNOERR,DATAXFER);
			strcpy(dbuf,DATA);
			tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
			Delaytime(5000);
			tlisnddis(fd,NULL,TNOERR,IDLE);
			ClntSync(Srvraddr,major,minor);
			tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
			tliclose(fd,TNOERR,UNINIT);
			ClntSync(Srvraddr,major,minor);

		}
 
	} 

  */									 

} 


/*====================================================================
*
*	connectsrvr
*
*		This function tests the TLI t_connect function.  It is intended
*		to be called by a TLI server test program.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void connectsrvr (char *protocol, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		flags;
	int		minor = 1;

	showProtoStk("TESTING t_connect",mode);

/*--------------------------------------------------------------------
*	Accept a connection request
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
/*	bind->qlen = MAXCONN; */
	bind->qlen = 1;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
	tlircvdis(fd,NULL,TNOERR,IDLE);




/* newly added for proper functioning */
/*--------------------------------------------------------------------
*	Reject a connection request
*-------------------------------------------------------------------*/
	relinquish();
	minor++;				/* 2.2 */
	showtnum(major,minor);
	SrvrSync(1,major,minor);


	tlilisten(fd,call,TNOERR,INCON);
	tlisnddis(fd,call,TNOERR,IDLE);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);


/*--------------------------------------------------------------------
*	Client TBADF error 
*-------------------------------------------------------------------*/
	relinquish();
	minor++;				/* 2.3 */
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client TOUTSTATE error 
*-------------------------------------------------------------------*/
	minor++;				/* 2.4 */
	showtnum(major,minor);
	
/*--------------------------------------------------------------------
*	Assist the client in causing a TOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;				/* 2.5 */
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
/*	bind->qlen = MAXCONN;*/
	bind->qlen = 1;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);


	
/*--------------------------------------------------------------------
*	Client TBADADDR error 
*-------------------------------------------------------------------*/
	minor++;				/* 2.6 */
	showtnum(major,minor);
	
/*--------------------------------------------------------------------
*	Client TBADOPT error 
*-------------------------------------------------------------------*/
/*
	minor++;				
	showtnum(major,minor);
*/
	
/*--------------------------------------------------------------------
*	Client TBADDATA error 
*-------------------------------------------------------------------*/
/*
	minor++;			
	showtnum(major,minor);
*/
	
/*--------------------------------------------------------------------
*	Assist the client in causing a TBUFOVFLW error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;				/* 2.7 */
	showtnum(major,minor);
	SrvrSync(1,major,minor);


	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);



/*--------------------------------------------------------------------
*	Assist the client in causing another TBUFOVFLW error
*-------------------------------------------------------------------*/
/*
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
*/
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	
/*--------------------------------------------------------------------
*	Client TNOTSUPPORT error 
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
/*
pause();
*/
/*--------------------------------------------------------------------
*	Send disconnect to an already closed endpoint
*-------------------------------------------------------------------*/
/*	relinquish();
 	minor++;
	showtnum(major,minor);
	if (mode == ASYNC)
	{

	
		fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		LoadAddr(bind->addr.buf,SRVR);
		bind->addr.len = bind->addr.maxlen;
//		bind->qlen = MAXCONN; 
		bind->qlen = 1;
		tlibind(fd,bind,NULL,TNOERR,IDLE);
		call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
		Delaytime(2000);
//		tlilisten(fd,call,TNOERR,INCON);
		SrvrSync(1,major,minor);
//		tlisnddis(fd,call,TNOERR,IDLE);
		tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
		tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
		tliclose(fd,TNOERR,UNINIT);
		SrvrSync(1,major,minor);

	}*/ 

/*--------------------------------------------------------------------
 * Test NLM memory usage by opening and closing connection until kbhit
 *-------------------------------------------------------------------*/
 /*	if (mode == SYNC)
	{
  		relinquish();
 		minor++;
		showtnum(major,minor);
	
	 	while(!kbhit())
		{

	
			fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
			bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
			LoadAddr(bind->addr.buf,SRVR);
			bind->addr.len = bind->addr.maxlen;
			bind->qlen = 1;
			tlibind(fd,bind,NULL,TNOERR,IDLE);
			call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
			SrvrSync(1,major,minor);
			tlilisten(fd,call,TNOERR,INCON);
			tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
			tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
			Delaytime(5000);
			tlircvdis(fd,NULL,TNOERR,IDLE);
			SrvrSync(1,major,minor);
			tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
			tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
			tliclose(fd,TNOERR,UNINIT);
			SrvrSync(1,major,minor);

		}

	}
 
	*/
}
