/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/sndrcvud.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sndrcvud.c,v 1.4 1994/08/05 14:39:03 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		sndrcvud.c
*	Date Created:	04/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_sndudata and t_rcvudata functions.
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
*	sndrcvudataclnt
*
*		This function tests the TLI t_sndudata function and provides
*		support for the testing of the t_rcvudata function.  This
*		routine is intended to be used on the client side of the TLI
*		test.
*
*	Return:	None
*
*	Parameters:
*		ps1				Session protocol stack
*		ps2				non-Session protocol stack
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void sndrcvudataclnt (char *ps1, char *ps2, int mode, int major)
{
	int			fd;
	tinfo			info;
	int			minor = 1;
	char			*tptr;
	tudataptr	ud;

	showProtoStk("TESTING t_sndudata",mode);

/*--------------------------------------------------------------------
*	Make a valid t_sndudata call w/ a small amount of data
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps2,mode,&info,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	ud->addr.len = ud->addr.maxlen;
	LoadAddr(ud->addr.buf,SRVR);
	strcpy(ud->udata.buf,DATA);
	ud->udata.len = strlen(ud->udata.buf) + 1;
	ClntSync(Srvraddr,major,minor);
	tlisndudata(fd,ud,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	Make a valid t_sndudata call w/ a large amount of data
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tptr = ud->udata.buf;
	ud->udata.buf = MuchoData;
	ud->udata.len = strlen(MuchoData) + 1;
	tlisndudata(fd,ud,TSYSERR,IDLE);
	ud->udata.buf = tptr;

/*--------------------------------------------------------------------
*	Make a valid t_sndudata call w/ zero length data
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ud->udata.len = 0;
#ifdef XTI
	tlisndudata(fd,ud,TBADDATA,IDLE);
#else
	tlisndudata(fd,ud,TNOERR,IDLE);
#endif
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlisndudata(fd,ud,TBADF,UNINIT);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	tlisndudata(fd,ud,TNOTSUPPORT,IDLE);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Server TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Server TNODATA error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Server TBUFOVFLW error
*-------------------------------------------------------------------*/
	relinquish();
	ClntSync(Srvraddr,major,minor);
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
  tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
	

/*--------------------------------------------------------------------
*	Server TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}


/*====================================================================
*
*	sndrcvudatasrvr
*
*		This function tests the TLI t_rcvudata function and provides
*		support for the testing of the t_sndudata function.  This
*		routine is intended to be used on the client side of the TLI
*		test.
*
*	Return:	None
*
*	Parameters:
*		ps1				Session protocol stack
*		ps2				non-Session protocol stack
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void sndrcvudatasrvr (char *ps1, char *ps2, int mode, int major)
{
	tbindptr		bind;
	int			fd;
	int			flags;
	int			minor = 1;
	tudataptr	ud;

	showProtoStk("TESTING t_rcvudata",mode);

/*--------------------------------------------------------------------
*	Make a valid t_rcvudata call and get a small amount of data
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
  tlircvudata(fd,ud,&flags,TNOERR,IDLE);
	checktudata(ud);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Client causes a TSYSERR by sending too large of a data packet
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client tries to pass a zero length data packet
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

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

/*--------------------------------------------------------------------
*	Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tlircvudata(fd,ud,&flags,TBADF,UNINIT);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TNODATA error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	if (mode == ASYNC)
		tlircvudata(fd,ud,&flags,TNODATA,IDLE);

/*--------------------------------------------------------------------
*	Cause a TBUFOVFLW error
*-------------------------------------------------------------------*/
	relinquish();
	SrvrSync(1,major,minor);
	minor++;
	showtnum(major,minor);
  ud->addr.maxlen--;
  SrvrSync(1,major,minor);
  tlircvudata(fd,ud,&flags,TBUFOVFLW,IDLE);
  ud->addr.maxlen++;
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
 
/*--------------------------------------------------------------------
*	Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
 	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	ud = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,UNBND);
	bind = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	bind->addr.len = bind->addr.maxlen;
	LoadAddr(bind->addr.buf,SRVR);
	tlibind(fd,bind,NULL,TNOERR,IDLE);
	tlircvudata(fd,ud,&flags,TNOTSUPPORT,IDLE);
	if (bind)
		tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	if (ud)
		tlifree(fd,(char *)ud,T_UNITDATA,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);
}
