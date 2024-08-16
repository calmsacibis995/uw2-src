/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/event.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: event.c,v 1.2 1994/02/18 15:06:10 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:	event.c
*	Date Created:	06/03/92
*	Version:	1.0
*	Programmer(s):	Jon T. Matsukawa
*	Purpose:	Test the new function t_event.
*	Modifications:  Too many to count, to many to finish....
*
*	COPYRIGHT (c) 1992 by Novell, Inc.  All Rights Reserved.
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
*	eventclnt
*
*		This function tests the TLI t_look function on the client side
*		of the test.
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
void eventclnt (char *ps1, char *ps2, int mode, int major)
{
	tcallptr		call;
	char			dbuf[DATASIZE];
	int			fd;
	int			minor = 1;
	tudataptr		ud;
	showProtoStk("TESTING t_event",mode);

/*--------------------------------------------------------------------
*	Server T_LISTEN event, client T_CONNECT event
*-------------------------------------------------------------------*/

	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	call->addr.len = call->addr.maxlen;
	LoadAddr(call->addr.buf,SRVR);
 	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
	{
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	}	
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlilook(fd,TNOERR,OUTCON|DATAXFER,T_CONNECT);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
 	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	Server T_DATA event
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	Look for a T_DISCONNECT event using an SPX endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Server T_DATA event
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	ud->addr.len = ud->addr.maxlen;
	LoadAddr(ud->addr.buf,SRVR);
	strcpy(ud->udata.buf,DATA);
	ud->udata.len = strlen(ud->udata.buf) + 1;
	ClntSync(Srvraddr,major,minor);
	tlisndudata(fd,ud,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Look for a T_NOEVENT event
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlilook(fd,TNOERR,IDLE,T_NOEVENT);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	
}


/*====================================================================
*
*	eventsrvr
*
*		This function is used to test the new(6-8-92) SPXII
*		t_event function, on the server side of the test suite.
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
void eventsrvr (char *ps1, char * ps2, int mode, int major)
{
	tbindptr		bind;
	tcallptr		call;
	char			dbuf[DATASIZE];
	char			params[DATASIZE];
	int			fd;
	int			flags;
	int			minor = 1;
	tudataptr	ud;

	showProtoStk("TESTING t_event",mode);

/*--------------------------------------------------------------------
*	Look for a T_LISTEN event using an SPX endpoint, client T_CONNECT
*	event
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);

#ifdef NLM
    if(mode != ASYNC)
    {
      strcpy(params, "(parameter set #1)");
      printf("Registering a Callback Function with t_event\n");
      printf("The Endpoint Registered is .... %d\n",fd);
      if(t_event(fd,MyCallBack,T_ENABLE, params)==-1)
		{
          printf("t_event failed ccode = -1\n");
          t_error("t_event failed, reason");
		}
  }
#endif 
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
 	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
 	SrvrSync(1,major,minor);


/*--------------------------------------------------------------------
*	Look for a T_DATA event using an SPX endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
	tlircv(fd,dbuf,DATASIZE,&flags,TNOERR,DATAXFER);
		
/*--------------------------------------------------------------------
*	Client T_DISCONNECT event
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
/*
#ifdef NLM
	printf("Disabling a Callback Function with t_event\n");
	printf("The Endpoint Disabled is .... %d\n",fd);
	if(t_event(fd,MyCallBack,T_DISABLE, "event param 1")==-1)
		{
		printf("t_event failed ccode = -1\n");
		t_error("t_event failed, reason");
		}
#endif
*/
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Look for a T_DATA event using an IPX endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);

#ifdef NLM 

	strcpy(params, "(parameter set #2)");
	printf("Registering a Callback Function with t_event\n");
	printf("The Connectionless Endpoint Registered is .... %d\n",fd);
	if(t_event(fd,MyCallBack,T_ENABLE,params)==-1)
		{
		printf("t_event failed ccode = -1\n");
		t_error("t_event failed, reason");
		}
#endif 

	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlircvudata(fd,ud,&flags,TNOERR,IDLE);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Client T_NOEVENT test
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);

}
