/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/unbind.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: unbind.c,v 1.4 1994/07/15 19:15:55 meb Exp $"
/*********************************************************************
*
*	Program Name:	tlitest
*	File Name:		unbind.c
*	Date Created:	02/28/90
*	Version:			1.0	
*	Programmer(s):	Bruce Thorne
*	Purpose:			Test t_unbind
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
*	unbindclnt
*
*		This function tests the t_unbind function on the client side.
*
*	Return:	None
*
*	Parameters:
*		ps1				Session protocol stack
*		pstemp tempory session protocol stack
*		ps2				non-Session protocol stack
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void unbindclnt (char *ps1, char *ps2, int mode, int major)
{
	int			fd;
	int			flags;
	int			minor = 1;
	char 	*pstemp; 
	tbindptr		req;
	tbindptr		ret;
	tudataptr	udata;
NBAddr *nbaddr;

#if XTI | NWU
	showProtoStk("TESTING t_unbind",mode);
/*--------------------------------------------------------------------
* 1. All combinations of slashes in t_open \dev\spxs, error for no /dev
*___________________________________________________________________*/
	showtnum(major,minor);
	pstemp = "/dev/nspx";						
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
* 2. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "/dev\\nspx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif


/*--------------------------------------------------------------------
* 3. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\dev/nspx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif

/*--------------------------------------------------------------------
* 4. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\dev\\nspx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif


/*--------------------------------------------------------------------
* 5. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\nspx";
#ifdef OS2
	fd = tliopen(pstemp,mode,NULL,TSYSERR,UNINIT);
#else
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif
#endif
	tliclose(fd,TBADF,UNINIT);


/*--------------------------------------------------------------------
* 6. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "/nspx";
#ifdef OS2
	fd = tliopen(pstemp,mode,NULL,TSYSERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif
 tliclose(fd,TBADF,UNINIT);


/*--------------------------------------------------------------------
* 7. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
pstemp = "/dev/nipx";						
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif
	pstemp = "/dev\\nipx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif

/*--------------------------------------------------------------------
* 8. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\dev/nipx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif

/*--------------------------------------------------------------------
* 9. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\dev\\nipx";
#ifndef NWU
	fd = tliopen(pstemp,mode,NULL,TNOERR,UNBND);
 tliclose(fd,TNOERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif

/*--------------------------------------------------------------------
* 10. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "\\nipx";
#ifdef OS2
	fd = tliopen(pstemp,mode,NULL,TSYSERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif
 tliclose(fd,TBADF,UNINIT);

/*--------------------------------------------------------------------
* 11. Another combination
*__________________________________________________________________*/
	minor++;
	showtnum(major,minor);
	pstemp = "/nipx";
#ifdef OS2
	fd = tliopen(pstemp,mode,NULL,TSYSERR,UNINIT);
#else
	fd = tliopen(pstemp,mode,NULL,TBADNAME,UNINIT);
#endif
 tliclose(fd,TBADF,UNINIT);
#endif

/*--------------------------------------------------------------------
* 12.  T_UNBIND expecting TNOERR
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	13. Cause a TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliunbind(fd,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	Cause another TBADF error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = -1;
	tliunbind(fd,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TOUTSTATE error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tliunbind(fd,TOUTSTATE,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TLOOK error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	req = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	req->addr.len = req->addr.maxlen;
	LoadAddr(req->addr.buf,CLNT | DYN);
	ret = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	ret->addr.len = req->addr.maxlen;
	tlibind(fd,req,ret,TNOERR,IDLE);

if (ProtoStk == 3 || ProtoStk == 4)
{
nbaddr = (NBAddr *)ret->addr.buf;
printf("clnt name: %s type: %d namenum: %d\n",nbaddr->name,nbaddr->name_type,nbaddr->name_num);
}

	udata = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	udata->addr.len = udata->addr.maxlen;
	LoadAddr(udata->addr.buf,SRVR);

if (ProtoStk == 3 || ProtoStk == 4)
{
nbaddr = (NBAddr *)udata->addr.buf;
printf("srvr name: %s type: %d namenum: %d\n",nbaddr->name,nbaddr->name_type,nbaddr->name_num);
}
	strcpy(udata->udata.buf,"Client to Server"); 
	udata->udata.len = strlen(udata->udata.buf) + 1;
	
	ClntSync(Srvraddr,major,minor);

	tlisndudata(fd,udata,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor);
	tliunbind(fd,TLOOK,IDLE);

	tlircvudata(fd,udata,&flags,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);
	tlifree(fd,(char *)req,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)ret,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)udata,T_UNITDATA,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
}


/*====================================================================
*
*	unbindsrvr
*
*		This function is used for testing the t_unbind function on the
*		server side.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack to use for forcing error
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void unbindsrvr (char *protocol,int mode, int major)
{
	int			fd;
	int			flags;
	int			minor = 1;
	tbindptr		req;
	tbindptr		ret;
	tudataptr	udata;
NBAddr *nbaddr;

	showProtoStk("TESTING t_unbind",mode);

/*--------------------------------------------------------------------
*	1.- 12. Client unbinds correctly
*-------------------------------------------------------------------*/
	for(; minor<12; minor++) {
		showtnum(major,minor);
	}
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	13. Client TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client TBADF error #2
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client TOUTSTATE error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Send a message so that the client can cause a TLOOK error
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	req = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	req->addr.len = req->addr.maxlen;
	ret = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	ret->addr.len = ret->addr.maxlen;
	LoadAddr(req->addr.buf,SRVR);
	tlibind(fd,req,ret,TNOERR,IDLE);

if (ProtoStk == 3 || ProtoStk == 4)
{
nbaddr = (NBAddr *)ret->addr.buf;
printf("clnt name: %s type: %d namenum: %d\n",nbaddr->name,nbaddr->name_type,nbaddr->name_num);
}
	udata = (tudataptr) tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	tlircvudata(fd,udata,&flags,TNOERR,IDLE);

	strcpy(udata->udata.buf,"Server to Client");
	udata->addr.len = udata->addr.maxlen;
	udata->udata.len = strlen(udata->udata.buf) + 1;
	tlisndudata(fd,udata,TNOERR,IDLE);
	SrvrSync(1,major,minor);

	tliunbind(fd,TNOERR,UNBND);
	tlifree(fd,(char *)req,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)ret,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)udata,T_UNITDATA,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
}
