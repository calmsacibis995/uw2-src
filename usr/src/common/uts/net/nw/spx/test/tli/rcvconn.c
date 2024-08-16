/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/rcvconn.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: rcvconn.c,v 1.3 1994/08/05 14:38:36 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		rcvconn.c
*	Date Created:	04/05/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_rcvconnect function.
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
*	rcvconnectclnt
*
*		This function tests the TLI t_rcvconnect function.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session Protocol Stack to use
*		ps2			non-Session Protocol Stack to use
*		major	(I)	Major test number
*
*===================================================================*/
void rcvconnectclnt (char *ps1, char *ps2, int major)
{
	int		fd;
	int		minor = 1;
	tcallptr	sndcall;
	tcallptr	rcvcall;

	showProtoStk("TESTING t_rcvconnect ",ASYNC);

/*--------------------------------------------------------------------
*	1.  Make a valid t_rcvconnect call
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,ASYNC,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	LoadAddr(sndcall->addr.buf,SRVR);
	sndcall->addr.len = sndcall->addr.maxlen;
	ClntSync(Srvraddr,major,minor);
	tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
	tlircvconnect(fd,rcvcall,TNOERR,DATAXFER);
	checktcall(rcvcall);
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	2.  Cause a TBUFOVFLW error
*-------------------------------------------------------------------*/

	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
	rcvcall->addr.maxlen--; 
	tlircvconnect(fd,rcvcall,TBUFOVFLW,DATAXFER);
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	rcvcall->addr.maxlen++; 


/*--------------------------------------------------------------------
*	Unable to cause a TNODATA error
*-------------------------------------------------------------------*/
/*
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	tliconnect(fd,sndcall,NULL,TNODATA,OUTCON);
	tlircvconnect(fd,rcvcall,TNODATA,IDLE);
	tlircvconnect(fd,rcvcall,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
*/
	tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
	tlifree(fd,(char *)rcvcall,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	3.  Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlircvconnect(fd,NULL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	4.  Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,ASYNC,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
#ifndef NWU
	sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	/*
	** t_alloc with a IPX fd will fail using T_CALL, do not reference
	** allocate structures if running on UNIX SVR4.2 
	*/
	LoadAddr(sndcall->addr.buf,SRVR);
	sndcall->addr.len = sndcall->addr.maxlen;
#else
	sndcall = NULL;
#endif
	tliconnect(fd,sndcall,NULL,TNOTSUPPORT,IDLE);

	/* ????? UNIX SVR4.2 ESMP bug ??? 
	 * UNIX SVR4.2 ESMP returns TOUTSTATE instead of TNOTSUPPORT
	 *
	 *	tlircvconnect(fd,NULL,TNOTSUPPORT,IDLE);
	 */
	tlircvconnect(fd,NULL,TOUTSTATE,IDLE);


#ifndef NWU
	tlifree(fd,(char *)sndcall,T_CALL,TNOERR,IDLE);
#endif
	tliclose(fd,TNOERR,UNINIT);
}


/*====================================================================
*
*	rcvconnectsrvr
*
*		This function provides support on the server side for testing
*		the TLI t_rcvconnect function.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack to use
*		major	(I)	Major test number
*
*===================================================================*/
void rcvconnectsrvr (char *protocol, int major)
{
	tbindptr	bind;
	tcallptr	call;
	int		fd;
	int		minor = 1;

	showProtoStk("TESTING t_rcvconnect ",ASYNC);

/*--------------------------------------------------------------------
*	Provide support for the client in making a valid t_rcvconnect call
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,ASYNC,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
	bind->qlen = MAXCONN;
	tlibind(fd,bind,bind,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Client TBUFOVFLW error
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
	relinquish();
	SrvrSync(1,major,minor);
	Delaytime(250);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);


/*--------------------------------------------------------------------
*	Client unable to cause a TNODATA error
*-------------------------------------------------------------------*/
/*
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
*	Client TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}
