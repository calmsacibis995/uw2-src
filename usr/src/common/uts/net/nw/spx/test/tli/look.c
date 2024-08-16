/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/look.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: look.c,v 1.2 1994/02/18 15:06:20 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		look.c
*	Date Created:	04/05/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_look function.
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
*	lookclnt
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
void lookclnt (char *ps1, char *ps2, int mode, int major)
{
	tcallptr		call;
	char			dbuf[DATASIZE];
	int			fd;
	int			minor = 1;
	tudataptr	ud;

	showProtoStk("TESTING t_look",mode);

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
#if XTI | NWU
    tlilook(fd,TNOERR,DATAXFER,T_DISCONNECT);
#else
	tlilook(fd,TNOERR,IDLE,T_DISCONNECT);
#endif
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
	ClntSync(Srvraddr,major,minor);
 tlilook(fd,TNOERR,IDLE,T_NOEVENT);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	
/*--------------------------------------------------------------------
*	Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlilook(fd,TBADF,UNINIT,T_NOEVENT);
}


/*====================================================================
*
*	looksrvr
*
*		This function is used to test the TLI t_look function on the
*		server side of the test.
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
void looksrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr		bind;
	tcallptr		call;
	char			dbuf[DATASIZE];
	int			fd;
	int			flags;
	int			minor = 1;
	tudataptr	ud;

	showProtoStk("TESTING t_look",mode);

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
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
 	SrvrSync(1,major,minor);
	Delaytime(20);
#if XTI | NWU
	tlilook(fd,TNOERR,IDLE,T_LISTEN);
#else
	tlilook(fd,TNOERR,INCON,T_LISTEN);
#endif
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
	tlilook(fd,TNOERR,DATAXFER,T_DATA);
	tlircv(fd,dbuf,DATASIZE,&flags,TNOERR,DATAXFER);
	
/*--------------------------------------------------------------------
*	Client T_DISCONNECT event
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
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
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilook(fd,TNOERR,IDLE,T_DATA);
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
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	Client TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}
