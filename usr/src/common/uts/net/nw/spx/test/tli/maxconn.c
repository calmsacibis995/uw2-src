/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/maxconn.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: maxconn.c,v 1.3 1994/08/23 17:25:04 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		maxconn.c
*	Date Created:	04/03/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the maximum pending connection limit
*						established by the t_bind call for use by the
*						t_listen call.  The server checks out how this
*						limit affects the t_listen call.  The client
*						provides support for testing the pending connection
*						limit.
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
*	maxconnclnt
*
*		This routine is used to verify the maximum number of outstanding
*		connect indications on the client side of the test.  This test
*		is used to see how the t_bind and t_listen functions on the
*		server side and the t_connect and t_rcvconnect functions on the
*		client side handle the maximum pending connections.
*
*	Return:	None
*
*	Parameters:
*		major	(I)	Major test number
*
*===================================================================*/
void maxconnclnt (char *protocol, int mode, int major)
{
	tcallptr	call;
	int		fd[MAXCONN + 1];
	int		idx = 0;
	int		minor = 1;
	int		qlen =3;
    tbindptr    bind;
    int     maxconn = MAXCONN;

	showProtoStk("TESTING multiple connections",ASYNC);

/*--------------------------------------------------------------------
*	Set up the endpoints    
*-------------------------------------------------------------------*/
	showtnum(major,minor);
#ifdef NWU	/*UNIX*/
/*--------------------------------------------------------------------
*   Check to see what negotiated number of connections is.
*-------------------------------------------------------------------*/
    fd[idx] = tliopen(protocol,2,NULL,TNOERR,UNBND);
    bind = (tbindptr) tlialloc(fd[idx],T_BIND,T_ALL,TNOERR,UNBND);
    bind->addr.len = bind->addr.maxlen;
    LoadAddr(bind->addr.buf,CLNT);
    bind->qlen = MAXCONN;
    tlibind(fd[idx],bind,bind,TNOERR,IDLE);
    maxconn = bind->qlen;
	if(Verbose)
		printf("qlen = %d\n",bind->qlen);
    t_close(fd[idx]);
#endif
	ClntSync(Srvraddr,major,minor);
	for (idx = 0; idx < maxconn; idx++)
	{
		relinquish();
		fd[idx] = tliopen(protocol,ASYNC,NULL,TNOERR,UNBND);
		tlibind(fd[idx],NULL,NULL,TNOERR,IDLE);
		
	}
		call = (tcallptr) tlialloc(fd[0],T_CALL,T_ALL,TNOERR,IDLE);
		call->addr.len = call->addr.maxlen;
		LoadAddr(call->addr.buf,SRVR);

/*--------------------------------------------------------------------
*	Queue up the limit connection requests for the server to
*	process
*-------------------------------------------------------------------*/
	for (idx = 0; idx < maxconn; idx++)
	{
		tliconnect(fd[idx],call,NULL,TNODATA,OUTCON);
#ifndef _Windows
		if(Verbose)
		printf("connection number = %d\n",idx);
#endif
	}

	ClntSync(Srvraddr,major,minor); 
/*--------------------------------------------------------------------
*	Receive connection acknowledgements up to the MAXCONN limit 
*-------------------------------------------------------------------*/
	if (mode==SYNC)
	{
		for (idx = 0; idx < maxconn; idx++)
		{
		tlircvconnect(fd[idx],NULL,TNOERR,DATAXFER);
#ifndef _Windows
		if(Verbose)
		printf("rcvconnect number = %d\n",idx);
#endif
		}
	}
	else
	{
		for (idx = 0; idx < qlen; idx++)
		{
		tlircvconnect(fd[idx],NULL,TNOERR,DATAXFER);
#ifndef _Windows
		if(Verbose)
		printf("rcvconnect number = %d\n",idx);
#endif
		}
		for (idx = qlen; idx < maxconn; idx++)
		{
		tlircvconnect(fd[idx],NULL,TNODATA,OUTCON);
#ifndef _Windows
		if(Verbose)
		printf("rcvconnect number = %d\n",idx);
#endif
		}


	}
/*--------------------------------------------------------------------
*	Check for one connection over the limit
*-------------------------------------------------------------------*/
    /*	tlircvconnect(fd[MAXCONN],NULL,TNOERR,DATAXFER); */

/*--------------------------------------------------------------------
*	Tear down the test
*-------------------------------------------------------------------*/
	ClntSync(Srvraddr,major,minor);
	tlifree(fd[0],(char *)call,T_CALL,TNOERR,DATAXFER);
	tlifree(fd[0],(char *)bind,T_CALL,TNOERR,DATAXFER);
	for (idx = 0; idx < maxconn; idx++)
	{
		tlisnddis(fd[idx],NULL,TNOERR,IDLE);
		tliclose(fd[idx],TNOERR,UNINIT);
	}
#ifndef NWU
    tlisnddis(fd[maxconn],NULL,TNOERR,IDLE);
    tliclose(fd[maxconn],TNOERR,UNINIT);
#endif
	ClntSync(Srvraddr,major,minor);

	}


/*====================================================================
*
*	maxconnsrvr
*
*		This routine is used to verify the maximum number of outstanding
*		connect indications on the server side of the test.  This test
*		is used to see how the t_bind and t_listen functions on the
*		server side and the t_connect and t_rcvconnect functions on the
*		client side handle the maximum pending connections.
*
*	Return:	None
*
*	Parameters:
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/

void maxconnsrvr (char *protocol, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call[MAXCONN];
	int		fd[MAXCONN + 1];
	int		idx;
	int		lfd;
	int		minor = 1;
    int     maxconn = MAXCONN;
    int     sequence[MAXCONN + 1];

	showProtoStk("TESTING multiple connections",mode);

/*--------------------------------------------------------------------
*	Set up the listen endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	SrvrSync(1,major,minor);
	lfd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(lfd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	bind->qlen = MAXCONN;
	tlibind(lfd,bind,bind,TNOERR,IDLE);
	maxconn = bind->qlen;
#ifndef _Windows
	if(Verbose)
	printf("qlen = %d\n",bind->qlen);
#endif


/*--------------------------------------------------------------------
*	Set up the other endpoints that will receive passed connections
*-------------------------------------------------------------------*/
	for (idx = 0; idx < maxconn; idx++)
	{
		relinquish();
		fd[idx] = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		tlibind(fd[idx],NULL,NULL,TNOERR,IDLE);
	}

/*--------------------------------------------------------------------
*	Wait for the client to post all of his connect requests
*-------------------------------------------------------------------*/
	SrvrSync(1,major,minor); 		      

/*--------------------------------------------------------------------
*	Process 0-MAXCONN connect requests
*   NOTE: In the AT&T TLI spec., a connect request cannot be accepted
*   until all indications (connect and disconnect) have been received.
*   Therefore for UNIX we must receive ALL indications, then accept.
*-------------------------------------------------------------------*/
	if (mode==SYNC)
	{
		call[0] = (tcallptr) tlialloc(lfd,T_CALL,T_ALL,TNOERR,IDLE);
#ifdef NWU
	for (idx = 0; idx < maxconn; idx++) {
		tlilisten(lfd,call[0],TNOERR,INCON);
		sequence[idx] = call[0]->sequence;
	}
 
	for (idx = 0; idx < maxconn; idx++) {
		call[0]->sequence = sequence[idx];
		tliaccept(lfd,fd[idx],call[0],TNOERR,IDLE|INCON,DATAXFER);
	}
#else

		for (idx = 0; idx < maxconn; idx++)
		{
			relinquish();
			tlilisten(lfd,call[0],TNOERR,INCON);
		   	tliaccept(lfd,fd[idx],call[0],TNOERR,IDLE|INCON,DATAXFER);
		}
#endif		
	}
	else
	{
#ifdef NWU
        for (idx = 0; idx < bind->qlen; idx++) {
            call[idx] = (tcallptr) tlialloc(lfd,T_CALL,T_ALL,TNOERR,IDLE);
            tlilisten(lfd,call[idx],TNOERR,INCON);
			sequence[idx] = call[idx]->sequence;
        }
        for (idx = bind->qlen; idx < maxconn; idx++) {
            call[idx] = (tcallptr) tlialloc(lfd,T_CALL,T_ALL,TNOERR,IDLE);
            tlilisten(lfd,call[idx],TNODATA,INCON);
			sequence[idx] = call[idx]->sequence;
        }
        for (idx = 0; idx < bind->qlen; idx++) {
			call[idx]->sequence = sequence[idx];
            tliaccept(lfd,fd[idx],call[idx],TNOERR,IDLE|INCON,DATAXFER);
        }
        for (idx = bind->qlen; idx < maxconn; idx++) {
			call[idx]->sequence = sequence[idx];
            tliaccept(lfd,fd[idx],call[idx],TOUTSTATE,IDLE|INCON,IDLE);
        }
#else
		for (idx = 0; idx < bind->qlen; idx++)
		{
			relinquish();
			call[idx] = (tcallptr) tlialloc(lfd,T_CALL,T_ALL,TNOERR,IDLE);
			tlilisten(lfd,call[idx],TNOERR,INCON);
		}
		for (idx = bind->qlen; idx <=MAXCONN; idx++)
		{
			relinquish();
			call[idx] = (tcallptr) tlialloc(lfd,T_CALL,T_ALL,TNOERR,IDLE);
			tlilisten(lfd,call[idx],TNODATA,INCON);
	     	}
		for (idx = 0; idx < bind->qlen; idx++)
		{
			tliaccept(lfd,fd[idx],call[idx],TNOERR,IDLE|INCON,DATAXFER);
		}
		for (idx = bind->qlen; idx <=MAXCONN; idx++)
		{
		
	     	tliaccept(lfd,fd[idx],call[idx],TOUTSTATE,IDLE|INCON,IDLE);
		}
#endif
	}


/*--------------------------------------------------------------------
*	Tear down the test
*-------------------------------------------------------------------*/
	SrvrSync(1,major,minor);
	tlifree(lfd,(char *)bind,T_BIND,TNOERR,IDLE);
	if (mode==SYNC)
 	{
		tlifree(lfd,(char *)call[0],T_CALL,TNOERR,IDLE);
	}
	for (idx = 0; idx < maxconn; idx++)
	{
		relinquish();
		if (mode==ASYNC)
		{
			tlifree(lfd,(char *)call[idx],T_CALL,TNOERR,IDLE);
		}	
		tlircvdis(fd[idx],NULL,TNOERR,IDLE);
		tliclose(fd[idx],TNOERR,UNINIT);
	}
	tliclose(lfd,TNOERR,UNINIT);
	SrvrSync(1,major,minor); 		      
}
