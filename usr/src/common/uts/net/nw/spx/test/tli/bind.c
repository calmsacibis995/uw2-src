/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/bind.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: bind.c,v 1.4 1994/05/16 22:24:14 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		bind.c
*	Date Created:	02/28/90
*	Version:			1.0	
*	Programmer(s):	Bruce Thorne
*	Purpose:			Test TLI t_bind
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#ifdef NWU
#include <sys/types.h>
#include <sys/stat.h>
#else /* NWU */
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#endif /* NWU */
#include <fcntl.h>
#include <string.h>
#include "tlitest.h"
#include "control.h"
#include "tliprcol.h"


/*--------------------------------------------------------------------
*	Constants
*-------------------------------------------------------------------*/
#ifdef NWU
#define SPXMAXSOCK	99
#define IPXMAXSOCK	1021
#define MAXSOCK	1024
#endif

#ifdef DOS
#define MAXSOCK	16  /* 20 - 4, ODI IPX w/out diagnostic responder */
#endif

#ifdef OS2
#define MAXSOCK	16  /* 33 - 8 */
#endif

/*
#ifdef OS2
#define MAXSOCK	25   33 - 8 
#endif
*/

#ifdef NLM
#define MAXSOCK	100  /* 100 - 8, 5 sockets used by the OS, 3 by CLIB */
#endif

#define MAXNBDG	40


/*====================================================================
*
*	testbind
*
*		This function tests the TLI t_bind function.
*
*	Return:	None
*
*	Parameters:
*		protocol	(I)	SPX or IPX
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void testbind (char *protocol, int mode, int major)
{
	unsigned	addrsize;
	int 		ccode;
	int		badfd;
	int		fd;
	int		fds[MAXSOCK + 1];
	int		i;
	tinfo 	info;
	int		minor = 1;
	tbindptr	req;
	tbindptr	req2;
	tbindptr	ret;
	tbindptr	ret2;
	tbindptr	ret3;
	char		*tempfile = "/tmp/qqxxyyzz.tmp\0";
	int j = 0;
	int		max_socket;
#ifdef NLM
	int		open_sockets;
	int		table_size;
	int		max_size;
#endif
	showProtoStk("TESTING t_bind",mode);

/*--------------------------------------------------------------------
*	1. Valid t_bind call with no bind structures
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,&info,TNOERR,UNBND);
	addrsize = (unsigned) info.addr;
	req  = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(req->addr.buf,CLNT | DYN);
	req2 = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	LoadAddr(req2->addr.buf,CLNT | DYN);
	ret  = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	ret2 = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	req->addr.len = addrsize;
	req2->addr.len = addrsize;

	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	2. Valid t_bind call with the req bind structure
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(fd,req,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	3. Valid t_bind call with the req and ret bind structures
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(fd,req,ret,TNOERR,IDLE);
	checktbind(ret);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	4. Valid t_bind call with the req and req bind structures
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(fd,req,req,TNOERR,IDLE);
	checktbind(req);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	5. Valid t_bind call with no bind structures, opens a dynamic SPX/IPX
*	socket
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	6. Valid t_bind call with req bind structure with an address length
*	of 0, opens a dynamic SPX/IPX socket
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	req->addr.len = 0;
	tlibind(fd,req,NULL,TNOERR,IDLE);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	7. Try binding to the same SPX socket twice
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	if (ProtoStk < 5)		/* TCP/IP fails on the bind */
	{
		tlibind(fd,req,ret,TNOERR,IDLE);
		fds[0] = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		tlibind(fds[0],req,ret2,TNOERR,IDLE);
		if (ProtoStk < 3)
		{
			if (!memcmp(ret->addr.buf,ret2->addr.buf,addrsize))
				printf("Addresses are not supposed to be the same\n");
		}else
			if (memcmp(ret->addr.buf,ret2->addr.buf,addrsize))
				printf("Addresses should be the same\n");

		tliclose(fds[0],TNOERR,UNINIT);
		tliunbind(fd,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	8. Cause a TBADF error by using an unopened fd with no bind structures
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	badfd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	tliclose(badfd,TNOERR,UNINIT);
	tlibind(badfd,NULL,NULL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	9. Cause a TBADF error by using an unopened fd with req and ret bind
*	structures
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(badfd,req,ret,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	Open a dummy file and use for invalid fd
*-------------------------------------------------------------------*/
	badfd = open(tempfile,O_CREAT|O_RDWR,S_IREAD|S_IWRITE);
	if (badfd < 0)
	{
		perror(tempfile);
		showmsg(TNUMFMT,"ERROR opening temporary file.");
	}

/*--------------------------------------------------------------------
*	10. Cause a TBADF error by using an invalid fd with no bind structures
*-------------------------------------------------------------------*/
	else
	{
		minor++;
		showtnum(major,minor);
		tlibind(badfd,NULL,NULL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	11. Cause a TBADF error by using an invalid fd with req and ret bind
*	structures
*-------------------------------------------------------------------*/
		minor++;
		showtnum(major,minor);
		tlibind(badfd,req,ret,TBADF,UNINIT);
		close(badfd);
		remove(tempfile);
	}

/*--------------------------------------------------------------------
*	12. Cause a TOUTSTATE error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlibind(fd,NULL,NULL,TOUTSTATE,IDLE);
	tliunbind(fd,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	13. Cause a TBADADDR error by using an invalid address length
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

	if (ProtoStk != 3 && ProtoStk != 4)
	{
		req->addr.len = 1;  
		tlibind(fd,req,ret,TBADADDR,UNBND);
		req->addr.len = addrsize;
	}

/*--------------------------------------------------------------------
*	14. Cause a TBUFOVFLW error by not allowing enough room in the ret
*	bind structure
*-------------------------------------------------------------------*/
	minor++;
	fflush(stderr);
	fflush(stdout);
	showtnum(major,minor);
	if (ProtoStk < 5)				/* TBADADDR error is returned instead */
	{
		req->addr.len = req->addr.maxlen;
		ret->addr.maxlen--;
		tlibind(fd,req,ret,TBUFOVFLW,IDLE);
		tliunbind(fd,TNOERR,UNBND);
		ret->addr.maxlen++;
	}

	tlifree(fd,(char *)ret ,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)ret2,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)req ,T_BIND,TNOERR,UNBND);
	tlifree(fd,(char *)req2,T_BIND,TNOERR,UNBND);

	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	15. Cause a TNOADDR error by opening too many sockets
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
#ifdef NWU
	if (ProtoStk == 1)
		max_socket = SPXMAXSOCK;
	if (ProtoStk == 2)
		max_socket = IPXMAXSOCK;
	if (ProtoStk > 2)
		max_socket = MAXSOCK;
#else
	max_socket = MAXSOCK;
#endif
	for (i = 0, ccode = 0; !ccode; i++) {
		if (i > max_socket)
		{
			printf("error, opened too many sockets, exp: %d, rec: %d\n",max_socket,i);
			ccode = -2;
			break;
		}

		if (ProtoStk == 4)
		{
			showmsg("%s\n","t_open");
			fds[i] = t_open(protocol,mode,NULL);
			if (fds[i] == -1)
			{
				ccode = -2;
				break;
			}
		}
		else
#ifdef NWU
		{
			showmsg("%s\n","t_open");
			fds[i] = t_open(protocol,mode,NULL);
			if (fds[i] == -1)
			{
				ccode = -2;
				break;
			}
		}
#else
		{
			fds[i] = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		}
#endif

		showmsg("%s\n","t_bind");
		ccode = t_bind(fds[i],NULL,NULL);
		if (!ccode)
			++j;
	}


#ifdef NWU
	if (ccode && t_errno != TSYSERR)
		printf("Unexpected ccode on exit from max socket loop, exp: %d, rec: %d\n",TSYSERR,t_errno);
#else
	if (ccode && t_errno != TNOADDR)
		printf("Unexpected ccode on exit from max socket loop, exp: %d, rec: %d\n",TNOADDR,t_errno);
#endif

	if (ProtoStk == 4)
	{
		if (i+1 != MAXNBDG)
			printf("Only opened %d DataGrams, exp: %d\n",i+1,MAXNBDG);
		else
			printf("Successfully opened %d DataGrams\n",i+1);
	}else
	{
		if (i != max_socket)
			printf("Only opened %d sockets, exp: %d\n",i,max_socket);
		else
			printf("Successfully opened %d sockets\n",i);
	}

	if (ccode != -2)
		tliclose(fds[i],TNOERR,UNINIT);

	for (--i; i >= 0; i--)
	{
		tliunbind(fds[i],TNOERR,UNBND);
		tliclose(fds[i],TNOERR,UNINIT);
	}
/*--------------------------------------------------------------------
*	16. test TADDRBUSY by binding to same socket with qlen
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
	
	if (ProtoStk == 1) {	/* SPX */
		fd = tliopen(protocol,mode,&info,TNOERR,UNBND);
		fds[0] = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		fds[1] = tliopen(protocol,mode,NULL,TNOERR,UNBND);
		req  = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		ret  = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		ret2 = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		ret3 = (tbindptr) tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		LoadAddr(req->addr.buf,CLNT | DYN);
		tlibind(fd,req,ret,TNOERR,IDLE);
	 	ret->qlen = 3;
		tlibind(fds[0],ret,ret2,TNOERR,IDLE);
	 	ret->qlen = 3;
		tlibind(fds[1],ret,ret3,TADDRBUSY,UNBND);

		tliunbind(fd,TNOERR,UNBND);
		tliunbind(fds[0],TNOERR,UNBND);
		tlifree(fd,(char *)req ,T_BIND,TNOERR,UNBND);
		tlifree(fd,(char *)ret ,T_BIND,TNOERR,UNBND);
		tlifree(fd,(char *)ret2 ,T_BIND,TNOERR,UNBND);
		tlifree(fd,(char *)ret3 ,T_BIND,TNOERR,UNBND);
		tliclose(fd,TNOERR,UNINIT);
		tliclose(fds[0],TNOERR,UNINIT);
		tliclose(fds[1],TNOERR,UNINIT);
	}
}
