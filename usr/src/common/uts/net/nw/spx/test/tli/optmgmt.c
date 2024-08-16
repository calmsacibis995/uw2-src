/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/optmgmt.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: optmgmt.c,v 1.3 1994/07/19 14:15:48 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:	optmgmt.c
*	Date Created:	01/07/91
*	Version:	1.0
*	Programmer(s):	Jon T. Matsukawa
*	Purpose:	Test TLI t_optmgmt.
*	Modifications: (Too Many and Way to Often)
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
*	optmgmtclnt
*
*		This function tests the TLI t_optmgmt function.  It is intended
*		to be called by a TLI client test program.
*
*	Return:	None
*
*	Parameters:
*		ps1		Session protocol stack
*		ps2		non-Session protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void optmgmtclnt (char *ps1, int mode, int major)
{
	int			fd;
	int			fdbad=100;
	int			minor = 1;
	tinfo			info;
	tcallptr		rcvcall;
	tcallptr		sndcall;
	struct t_optmgmt	*optreq;
	struct t_optmgmt	*optret;

	showProtoStk("TESTING t_optmgmt",mode);

/*--------------------------------------------------------------------
*	1. Establish a connection and make a t_optmgmt call using
*	the T_DEFAULT mode to retrieve the providers default options
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);

	sndcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	rcvcall = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	optreq = (struct t_optmgmt *) tlialloc(fd,T_OPTMGMT,T_ALL,TNOERR,IDLE);
	optret = (struct t_optmgmt *) tlialloc(fd,T_OPTMGMT,T_ALL,TNOERR,IDLE);

	LoadAddr(sndcall->addr.buf,SRVR); 
	sndcall->addr.len = sndcall->addr.maxlen; 

	optreq->flags = T_DEFAULT;
	optreq->opt.len = 0;
	tlioptmgmt(fd,optreq,optret,TNOERR,IDLE);
	
	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	2. Make a t_optmgmt call using T_CHECK to verify whether the
*	options specified in req are supported by the provider.
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

 /* since optret should have set the above options to default options
    supported by the transport provider the following T_CHECK with
    t_optmgmt should return T_SUCCESS                              */

    optreq=optret;
	optreq->flags = T_CHECK;
	tlioptmgmt(fd,optreq,optret,TNOERR,IDLE);
	switch(optret->flags)
	  {
	   case T_SUCCESS:
		if(Verbose==TRUE)
	   	printf("ret->flag    T_SUCCESS              OK\n");
		break;

	   case	T_FAILURE:
		printf("ret->flag    T_FAILURE\n");
	  	exit(1);
		break;
		
	   default:	
	  	printf("optret->flags = %x           UNEXPECTED VALUE\n",optret->flags);
	  	exit(2);
		break;
	  }

	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	3. Cause a TSYSERR error using T_DEFAULT option and setting
*	optreq->opt.len != 0;
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
/*
	optreq=optret;
	optreq->flags = T_DEFAULT;
	tlioptmgmt(fd,dummy,optret,TSYSERR,IDLE);
*/
	ClntSync(Srvraddr,major,minor);


/*--------------------------------------------------------------------
*	4. Cause a TBADOPT error using T_CHECK option and setting
*	optreq->opt.len = 0;
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	optreq=optret;
	optreq->flags = T_CHECK;
	optreq->opt.len=0;
	tlioptmgmt(fd,optreq,optret,TBADOPT,IDLE);
	ClntSync(Srvraddr,major,minor);


/*--------------------------------------------------------------------
*	5. cause a TBADF error by making a t_optmgmt call
*	with an unopened fd.
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlioptmgmt(fdbad,optreq,optret,TBADF,UNINIT);
	ClntSync(Srvraddr,major,minor);
/*--------------------------------------------------------------------
*	6. Cause a TBADFLAG error by making a t_optmgmt call
*	with using an invalid flag
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	optreq->flags = '\x33';
	optreq->opt.len = 0;
	tlioptmgmt(fd,optreq,optret,TBADFLAG,IDLE);
	
	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	7. Cause a TBUFOVFLW error by making a t_optmgmt call
*	using an insufficient recieve buffer size.
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
#ifdef NWU
	optret->opt.maxlen = 3;
#else
	optret->opt.maxlen =5;
#endif
   	optreq->flags = T_DEFAULT;
    	sndcall->opt.len = 10;
	tlioptmgmt(fd,optreq,optret,TBUFOVFLW,IDLE);

/*--------------------------------------------------------------------
*	8. Cause a TOUTSTATE error by making a t_optmgmt call
*	using an unbound endpoint with t_optmgmt not in the T_IDLE state.
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
	optreq->flags = T_DEFAULT;
	sndcall->opt.len = 0;
	tliunbind(fd,TNOERR,UNBND);
	tlioptmgmt(fd,optreq,optret,TOUTSTATE,UNBND);
	
/*--------------------------------------------------------------------
*	Shut down endpoint. t_close, end of test sequence
*-------------------------------------------------------------------*/

	tlifree(fd,(char *)sndcall,T_BIND,TNOERR,UNBND);
 	tlifree(fd,(char *)rcvcall,T_CALL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

}		
	  
	

/*====================================================================
*
*	optmgtsrvr
*
*		This function tests the TLI optmgmt function.  It is intended
*		to be called by the TLI server test program.
*
*	Return:	None
*
*	Parameters:
*		protocol	protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void optmgmtsrvr (char *protocol, int mode, int major)
{
	tbindptr	bind;
	tcallptr	call;
	int		fd;
	int		minor = 1;

	showProtoStk("TESTING t_optmgmt",mode);

/*--------------------------------------------------------------------
*	1. Accept a connection request for client T_DEFAULT test
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
 	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	2. Accept a connection request for client T_CHECK test
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(bind->addr.buf,SRVR);
	bind->addr.len = bind->addr.maxlen;
	bind->qlen = MAXCONN;
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	call = (tcallptr) tlialloc(fd,T_CALL,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
 	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	3. Cause a TSYSERR error using T_DEFAULT option and setting
*	optreq->opt.len != 0;
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	4. Client Causes a TBADOPT call using T_CHECK option and setting
*	optreq->opt.len = 0;
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	5. Client tests, TBADF error by making a t_optmgmt call
*	with an unopened fd.
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);



/*--------------------------------------------------------------------
*	6. Cause a TBADFLAG error by making a t_optmgmt call
*	with using an invalid flag
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	7. Client tests cause, TBUFOVFLW error by making a t_optmgmt call
*	using an insufficient recieve buffer size.
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	8. Client causes a TOUTSTATE error by making a t_optmgmt call
*	using an unbound endpoint with TOUTSTATE not in T_IDLE.
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
   }
