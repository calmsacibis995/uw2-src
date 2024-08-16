/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/listen.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: listen.c,v 1.2 1994/02/18 15:06:17 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		listen.c
*	Date Created:	04/23/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_listen function.
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
*	listenclnt
*
*		This routine is used for testing the TLI t_listen function on
*		the client side.
*
*	Return:	None
*
*	Parameters:
*		protocol		protcol stack to use
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void listenclnt (char *ps1, char *ps2, int mode, int major)
{
	tcallptr		call;
	char			dbuf[DATASIZE];
	int			fd;
	int			minor = 1;

	showProtoStk("TESTING t_listen",mode);

/*--------------------------------------------------------------------
*	Provide support for a valid t_listen call on the server side
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	LoadAddr(call->addr.buf,SRVR);
	call->addr.len = call->addr.maxlen;
	ClntSync(Srvraddr,major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}

/*--------------------------------------------------------------------
*	TBADF error on server
*-------------------------------------------------------------------*/
	minor++;	
	showtnum(major,minor);
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	ClntSync(Srvraddr,major,minor);
	Delaytime(250);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	TNODATA error on server
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);


/*--------------------------------------------------------------------
*	Help the server cause a TBUFOVFLW error
*-------------------------------------------------------------------*/

	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	Delaytime(1000);
	if (mode == SYNC){
#ifdef SPXII
	printf("    Long Timeout HERE\n");
		tliconnect(fd,call,NULL,TLOOK,OUTCON);
#else
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
#endif

	}
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
#ifdef SPXII
	printf("    Long Timeout HERE\n");
		tlircvconnect(fd,NULL,TLOOK,OUTCON);
#else
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
#endif
	}
	tlircvdis(fd,NULL,TNOERR,IDLE);

	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	ClntSync(Srvraddr,major,minor);



/*--------------------------------------------------------------------
*	TBADF error on server
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	TBADF error on server
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	TNOTSUPPORT error on server
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

}


/*====================================================================
*
*	listensrvr
*
*		This routine is used for testing the TLI t_listen function on
*		the server side.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session protocol stack
*		ps2			non-Session Protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void listensrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		flags;
	int		minor = 1;

	showProtoStk("TESTING t_listen",mode);

/*--------------------------------------------------------------------
*	Make a valid t_listen call
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
/*	bind->qlen = MAXCONN;  */
	 bind->qlen = 1;
	SrvrSync(1,major,minor);
#if NLM | NWU
	tlibind(fd,bind,NULL,TNOERR,IDLE|INCON);
#else
	tlibind(fd,bind,NULL,TNOERR,IDLE);
#endif
#if NLM | NWU
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,INCON|IDLE);
#else
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
#endif
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);

/*--------------------------------------------------------------------
*	Cause a TBADF error,
*-------------------------------------------------------------------*/
	minor++;				/* 3.2 */
	showtnum(major,minor);

#if XTI | NWU
	tlilisten(fd,call,TOUTSTATE,DATAXFER);
#else
#ifdef NLM
	tlilisten(fd,call,TLOOK,DATAXFER);
#else
	tlilisten(fd,call,TBADF,DATAXFER);
#endif
#endif


	SrvrSync(1,major,minor);
#ifndef NWU
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
#endif
	tlircvdis(fd,NULL,TNOERR,IDLE);




/*--------------------------------------------------------------------
*	Cause a TNODATA error
*-------------------------------------------------------------------*/
	minor++;				/* 3.3 */
	showtnum(major,minor);


	if (mode == ASYNC){
		tlilisten(fd,call,TNODATA,IDLE);

	}

/*--------------------------------------------------------------------
*	Cause a TBUFOVFLW error
*-------------------------------------------------------------------*/


	relinquish();
	minor++;				/* 3.4 */
	showtnum(major,minor);
	SrvrSync(1,major,minor);


	call->addr.maxlen--;
	tlilisten(fd,call,TBUFOVFLW,INCON); 
	call->addr.maxlen++;
	tlifree(fd,(char *)bind,T_BIND,TNOERR,INCON);
	tlifree(fd,(char *)call,T_CALL,TNOERR,INCON);

	tliclose(fd,TNOERR,UNINIT);
	printf("    Long Timeout HERE\n");
	SrvrSync(1,major,minor);
	
/*--------------------------------------------------------------------
*	Cause a TBADF error
*-------------------------------------------------------------------*/
	t_close(fd);

	relinquish();
	minor++;				/* 3.5 */
	showtnum(major,minor);

	tlilisten(fd,call,TBADF,UNINIT);

/*--------------------------------------------------------------------
*   Cause a TBADF error
*   In AT&T TLI spec., as long as the fd is a network endpoint, t_listen
*   will still function.  So we will skip tlilisten if UNIX.
*-------------------------------------------------------------------*/

	relinquish();
	minor++;				/* 3.6 */
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,UNBND);
#ifndef NLM
#ifndef NWU		/*future fix when NW SPX is XTI */
#ifdef XTI
	tlilisten(fd,call,TBADQLEN,UNBND);
#else
	tlilisten(fd,call,TBADF,UNBND);
#endif
#endif
#endif
	tlifree(fd,(char *)call,T_CALL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;				/* 3.7 */
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
/*	bind->qlen = MAXCONN; */
	bind->qlen = 1;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
#ifndef NWU
	/*
	** t_alloc with a IPX fd (connectionless) will fail using T_CALL as the
	** structure type.  Donot allocate structures if running on UNIX SVR4.2 
	*/
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
#else
	call = NULL;
#endif
	tlilisten(fd,call,TNOTSUPPORT,IDLE);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
#ifndef NWU
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
#endif
	tliclose(fd,TNOERR,UNINIT);

}
