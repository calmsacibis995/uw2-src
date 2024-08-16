/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/sndrcvdi.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sndrcvdi.c,v 1.2 1994/02/18 15:06:46 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		sndrcvdi.c
*	Date Created:	04/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_snddis and t_rcvdis functions.
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
*	sndrcvdisclnt
*
*		This function tests the TLI t_snddis function and provides
*		support for the test of the t_rcvdis function.  This routine
*		is intended to be run by the TLI client test program.
*
*	Return:	None
*
*	Parameters:
*		ps1	(I)	Connection based protocol 
*		ps2	(I)	Connection-less based protocol 
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void sndrcvdisclnt (char *ps1, char *ps2, int mode, int major)
{
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	tinfo		info;
	int		minor = 1;
	int		flags;
	unsigned	templen;
	unsigned	tempmax;
	char		*tptr;
	static int pass = 0;

	pass++;



	showProtoStk("TESTING t_snddis",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_snddis call
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
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
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);


/*--------------------------------------------------------------------
*   This syncronization call is required because if the t_connect
*   indication from the client arrives before the t_rcvdis is called
*   then the returned state will be INCON. Which is valid but not
*   what is currently called for in this test case.
*-------------------------------------------------------------------*/

    ClntSync(Srvraddr,major,minor);


/*--------------------------------------------------------------------
*	2. Server rejects a connection request
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (mode == SYNC){
#ifdef SPXII
		tliconnect(fd,call,call,TLOOK,OUTCON);
#else
		tliconnect(fd,call,call,TNOERR,DATAXFER);
#endif

	}
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
#ifdef SPXII
		tlircvconnect(fd,call,TLOOK,OUTCON);
#else
		tlircvconnect(fd,call,TNOERR,DATAXFER);
#endif
	}
	ClntSync(Srvraddr,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);



/*--------------------------------------------------------------------
*	3. Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlisnddis(fd,NULL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	4. Cause a TOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlisnddis(fd,NULL,TOUTSTATE,IDLE);


/*--------------------------------------------------------------------
*	5. Cause a TBADDATA error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	ClntSync(Srvraddr,major,minor);
	showtnum(major,minor);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	LoadAddr(call->addr.buf,SRVR);
	call->addr.len = call->addr.maxlen;
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	templen = call->udata.len;
	tempmax = call->udata.maxlen;
	tptr = call->udata.buf;
	switch (info.connect)
	{
		case -2:
			call->udata.len = 3;
			call->udata.maxlen = 2;
			break;
		case -1:
			break;
		default:
			call->udata.len = call->udata.maxlen + 1;
			break;
	}
	call->udata.buf = dbuf;
	if (info.connect != -1)
		tlisnddis(fd,call,TBADDATA,DATAXFER);
	call->udata.len = templen;
	call->udata.maxlen = tempmax;
	call->udata.buf = tptr;
	ClntSync(Srvraddr,major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	6. Server TBADSEQ error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	Delaytime(500);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}

/*--------------------------------------------------------------------
*	7. Another server TBADSEQ error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
    tlisnddis(fd,NULL,TNOERR,IDLE);




/*--------------------------------------------------------------------
*	Cause a TLOOK error
*-------------------------------------------------------------------*/
/*
    relinquish();
	minor++;
	showtnum(major,minor);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	ClntSync(Srvraddr,major,minor);
	tlilook(fd,TNOERR,DATAXFER,T_DATA);
	ClntSync(Srvraddr,major,minor);
#ifdef NLM
	tlisnddis(fd,NULL,TNOERR,IDLE);
#else
	tlisnddis(fd,NULL,TLOOK,DATAXFER);
#endif
#ifdef NLM

#else
	tlilook(fd,TNOERR,DATAXFER,T_DATA);
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
	tlisnddis(fd,NULL,TNOERR,IDLE);
#endif
*/
    tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);




/*--------------------------------------------------------------------
*	8. Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlisnddis(fd,NULL,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	9. Server TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	10. Server TNODIS error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
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
	ClntSync(Srvraddr,major,minor);
	tlifree(fd,(char *)call,T_CALL,TNOERR,DATAXFER);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Server TBUFOVFLW error - Not possible to cause this error when
*	using SPX
*-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
*	11. Server TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}


/*====================================================================
*
*	sndrcvdissrvr
*
*		This function tests the TLI t_rcvdis function and provides
*		support for the test of the t_snddis function.  This routine
*		is intended to be run by the TLI server test program.
*
*	Return:	None
*
*	Parameters:
*		ps1	(I)	Connection based protocol 
*		ps2	(I)	Connection-less based protocol 
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void sndrcvdissrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	tdisc		disc;
	int		fd;
	int		minor = 1;

	showProtoStk("TESTING t_rcvdis",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_rcvdis call
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	Delaytime(1000);
	tlircvdis(fd,&disc,TNOERR,IDLE);
	checktdisc(&disc);


/*--------------------------------------------------------------------
*   This syncronization call is required because if the t_connect
*   indication from the client arrives before the t_rcvdis is called
*   then the returned state will be INCON. Which is valid but not
*   what is currently called for in this test case.
*-------------------------------------------------------------------*/
    SrvrSync(1,major,minor);



/*--------------------------------------------------------------------
*	2. Reject a connection request
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tlisnddis(fd,call,TNOERR,IDLE);
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	3. Client TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	4. Client TOUTSTATE error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	5. Client TBADDATA error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);

	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	6. Cause a TBADSEQ error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);


	tlilisten(fd,call,TNOERR,INCON);
#ifdef NWU
	/* cannot cause a BADSEQ with a NULL call struct, will cause T_OUTSTATE */
	call->sequence += 100;
	tlisnddis(fd,call,TBADSEQ,INCON);
	call->sequence -= 100;
#else
	tlisnddis(fd,NULL,TBADSEQ,INCON);
#endif

/*--------------------------------------------------------------------
*	7. Cause another TBADSEQ error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	call->sequence++;
	tlisnddis(fd,call,TBADSEQ,INCON);
	call->sequence--;
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Client TLOOK error
*-------------------------------------------------------------------*/
/*
    relinquish();
	minor++;
	showtnum(major,minor);

    tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);
	SrvrSync(1,major,minor);
	SrvrSync(1,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);
*/
    tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	8. Client TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	9. Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlircvdis(fd,NULL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	10. Cause a TNODIS error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
 	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	if (mode == ASYNC)
		tlircvdis(fd,NULL,TNODIS,DATAXFER);
	SrvrSync(1,major,minor);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,DATAXFER);
	tlifree(fd,(char *)call,T_CALL,TNOERR,DATAXFER);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TBUFOVFLW error - Not possible to cause this error when
*	using SPX
*-------------------------------------------------------------------*/

/*--------------------------------------------------------------------
*	11. Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlircvdis(fd,&disc,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);
}
