/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/accept.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: accept.c,v 1.2 1994/02/18 15:05:47 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		accept.c
*	Date Created:	04/04/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_accept function.
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
*	acceptclnt
*
*		This function provides support on the client side of the test
*		for testing the TLI t_accept function.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack to use
*		major	(I)	Major test number
*
*===================================================================*/
void acceptclnt (char *protocol, int mode, int major)
{
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		minor = 1;

	showProtoStk("TESTING t_accept",mode);

/*--------------------------------------------------------------------
*	1. Make a connect request so that the server can accept on the same
*	endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	LoadAddr(call->addr.buf,SRVR);
	call->addr.len = call->addr.maxlen;
	ClntSync(Srvraddr,major,minor);
	Delaytime(500);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	2. Make another connect request so that the server can accept on a
*	different endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	3. Server TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	4. Help the server cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);


/*--------------------------------------------------------------------
*	5. Server TOUTSTATE error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	6. Server TOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
    tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Help the server cause a TBADOPT error by sending some options
*-------------------------------------------------------------------*/
/*
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	tliconnect(fd,call,NULL,TNODATA,OUTCON);
*/
	
/*--------------------------------------------------------------------
*	7. Server TBADSEQ error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	8. Server t_listen call fails with a TBUFOVFLW error.  Verifying
*	that a connection can still be accepted and used.
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER); 
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);



	tlifree(fd,(char *) call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	9. Server TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}


/*====================================================================
*
*	acceptsrvr
*
*		This function tests the TLI t_accept function.
*
*	Return:	None
*
*	Parameters:
*		ps1	(I)	connection based protocol stack to use
*		ps2	(I)	connectionless protocol stack to use
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Test major number
*
*===================================================================*/
void acceptsrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		flags;
	int		minor = 1;
	int		resfd;
	int		seq;

	showProtoStk("TESTING t_accept",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_accept call, accept a connection on the listen
*	endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
/*	bind->qlen = MAXCONN;  */
	bind->qlen = 1;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	tlircv(fd,dbuf,DATASIZE,&flags,TNOERR,DATAXFER);
	tlircvdis(fd,NULL,TNOERR,IDLE);





/*--------------------------------------------------------------------
*	2. Make a valid t_accept call, accept a connection on an endpoint
*	other than the listen endpoint (pass connection)
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	resfd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(resfd,NULL,NULL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,resfd,call,TNOERR,IDLE,DATAXFER);
	SrvrSync(1,major,minor);
	tlircv(resfd,dbuf,DATASIZE,&flags,TNOERR,DATAXFER);
	tlircvdis(resfd,NULL,TNOERR,IDLE);
	tliclose(resfd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	3. Cause a TBADF error by using an unopened fd
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliaccept(resfd,resfd,call,TBADF,UNINIT,UNINIT);

/*--------------------------------------------------------------------
*	4. Cause a TSYSERR error by trying to pass a connection to an unopened
*	endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
#if XTI | NWU
	tliaccept(fd,resfd,call,TBADF,INCON,UNINIT);
#else
	tliaccept(fd,resfd,call,TSYSERR,INCON,UNINIT);
#endif
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	5. Cause a TOUTSTATE error by making a t_accept call with fd not in 
*	the T_INCON state
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliaccept(fd,fd,call,TOUTSTATE,IDLE,IDLE);

/*--------------------------------------------------------------------
*	6. Cause a TOUTSTATE error by making a t_accept call with resfd not
*	in the T_IDLE state
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	resfd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	SrvrSync(1,major,minor);

	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,resfd,call,TOUTSTATE,INCON,UNBND);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
	tliclose(resfd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TBADOPT error by passing some invalid options
*-------------------------------------------------------------------*/
/*
	relinquish();
	minor++;
	showtnum(major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	call->opt.maxlen = 1;
	tliaccept(fd,fd,call,TBADOPT,INCON,INCON);
*/

/*--------------------------------------------------------------------
*	7. Cause a TBADSEQ error by passing an invalid sequence number
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);


	tlilisten(fd,call,TNOERR,INCON);
	seq = call->sequence;
	call->sequence = 0xBD;
	tliaccept(fd,fd,call,TBADSEQ,INCON,INCON);
	call->sequence = seq;
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
	
/*--------------------------------------------------------------------
*	8. Cause the t_listen call to fail with a TBUFOVFLW error by
*	corrupting the call structure, the t_accept call that follows
*	should succeed
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
	call->addr.maxlen--;

	tlilisten(fd,call,TBUFOVFLW,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	tlircv(fd,dbuf,DATASIZE,&flags,TNOERR,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
	tlifree(fd,(char *) bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *) call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	9. Cause a TNOTSUPPORT error
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
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
#else
	call = NULL;
#endif

	/* ????? UNIX SVR4.2 ESMP bug??? */
	tliaccept(fd,fd,call,TNOTSUPPORT,IDLE,IDLE);

#ifndef NWU
	tlifree(fd,(char *) call,T_CALL,TNOERR,IDLE);
#endif
	tliclose(fd,TNOERR,UNINIT);
}

