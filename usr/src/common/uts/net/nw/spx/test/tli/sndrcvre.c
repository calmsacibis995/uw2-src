/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/sndrcvre.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sndrcvre.c,v 1.2 1994/02/18 15:06:49 vtag Exp $"
/*********************************************************************
*  
*	Program Name:	
*	File Name:	sndrcvre.c
*	Date Created:	01/09/91
*	Version:	1.1
*	Programmer(s):	Jon T. Matsukawa
*	Purpose: Test the TLI t_sndrel and t_rcvrel functions.
*	Modifications: 2/10/91 for SPX2
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

#define REPS	15


/*====================================================================
*
*	sndrcvrelclnt
*
*		This function tests the TLI t_sndrel function and provides
*		support for testing the TLI t_rcvrel function.  This function
*		is now supported by SPX2.  This routine is intended to be
*		be run on the client side of the test.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session protocol stack to use
*		ps2			non-Session protocol stack to use
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void sndrcvrelclnt (char *ps1, char *ps2, int mode, int major)
{
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		flags;
	int		minor = 1;
	int		fdbad=2;	

	showProtoStk("TESTING t_sndrel",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_sndrel and then recieve data before issuing
*	a t_rcvrel.
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
 	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	call->addr.len = call->addr.maxlen;
	LoadAddr(call->addr.buf,SRVR);
	ClntSync(Srvraddr,major,minor);
	Delaytime(250);
	if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	tlisndrel(fd,TNOERR,OUTREL);
	ClntSync(Srvraddr,major,minor);
	do
	{
		tlircv(fd,dbuf,sizeof(DATA),&flags,TNOERR,OUTREL);
		if(Verbose)
        printf("Data recieved = %s\n",dbuf);
        if (strcmp(dbuf,DATA))
			{
			printf("Data sent != data received\n");
            printf("data received = %s\n",dbuf);
            printf("data expected = %s\n",DATA);
            }
	}
	while (flags & T_MORE);
	ClntSync(Srvraddr,major,minor);
	tlircvrel(fd,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);


/*--------------------------------------------------------------------
*
*   2. Make a valid t_sndrel followed by a t_snddis.
*
*-------------------------------------------------------------------*/
  minor ++;
  showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
 	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	call->addr.len = call->addr.maxlen;
	LoadAddr(call->addr.buf,SRVR);
	ClntSync(Srvraddr,major,minor);
  if (mode == SYNC)
		tliconnect(fd,call,NULL,TNOERR,DATAXFER);
	else
	{
		tliconnect(fd,call,NULL,TNODATA,OUTCON);
		tlircvconnect(fd,NULL,TNOERR,DATAXFER);
	}
	tlisndrel(fd,TNOERR,OUTREL);
	ClntSync(Srvraddr,major,minor);
	ClntSync(Srvraddr,major,minor);	/* added NWU */
  tlisnddis(fd,NULL,TNOERR,IDLE);
  ClntSync(Srvraddr,major,minor);
  tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);


/*--------------------------------------------------------------------
*	3. Cause a t_sndrel TNOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
#ifndef NWU
	tlisndrel(fd,TOUTSTATE,IDLE);
#endif
	tliclose(fd,TNOERR,UNINIT);
/*--------------------------------------------------------------------
*	4. Cause a t_sndrel TNOTSUPPORT error by using the non Session
*	protocol stack.
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlisndrel(fd,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	5. Cause a t_sndrel TBADF error by using a non opened endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlisndrel(fdbad,TBADF,UNINIT);

}


/*====================================================================
*
*	sndrcvrelsrvr
*
*		This function tests the TLI t_sndrel function and provides
*		support for testing the TLI t_rcvrel function.  This function
*		is now supported by SPX2.  This routine is intended to be
*		be run on the server side of the test.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session protocol stack to use
*		ps2			non-Session protocol stack to use
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void sndrcvrelsrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		fdbad=2;
	int		minor = 1;
	int		i;

	showProtoStk("TESTING t_rcvrel",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_rcvrel and then send data before issuing
*	a t_sndrel.
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
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	Delaytime(1000);
	tlircvrel(fd,TNOERR,INREL);
	strcpy(dbuf,DATA);
	for (i = 0; i < REPS; i++)
	{
		relinquish();
		tlisnd(fd,dbuf,strlen(dbuf)+1,T_MORE,TNOERR,INREL);

	}
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,INREL);
	Delaytime(3000);
	tlisndrel(fd,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	SrvrSync(1,major,minor);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);


/*--------------------------------------------------------------------
*
*   2. Make a valid t_rcvrel followed by a t_rcvdis.
*
*-------------------------------------------------------------------*/
  minor++;
  showtnum(major,minor);
  fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlilisten(fd,call,TNOERR,INCON);
	tliaccept(fd,fd,call,TNOERR,DATAXFER,DATAXFER);
	SrvrSync(1,major,minor);
	Delaytime(1000);
	tlircvrel(fd,TNOERR,INREL);
	SrvrSync(1,major,minor);
	SrvrSync(1,major,minor);	/* add NWU */
  tlircvdis(fd,NULL,TNOERR,IDLE);
  tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	3. Cause a t_rcvrel TOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlircvrel(fd,TOUTSTATE,IDLE);
	tliclose(fd,TNOERR,UNINIT);
/*--------------------------------------------------------------------
*	4. Cause a t_rcvrel TNOTSUPPORT error by using the non Session
*	Protocol Stack.
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlircvrel(fd,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	5. Cause a t_rcvrel TBADF error by using a non opened endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlircvrel(fdbad,TBADF,UNINIT);

}
