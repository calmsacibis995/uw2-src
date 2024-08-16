/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/server.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: server.c,v 1.2 1994/02/18 15:06:41 vtag Exp $"
/*********************************************************************
*
*	Program Name:   ssearch.exe
*	File Name:	Server.c
*	Date Created:	01/24/92
*	Version:	1.0
*	Programmer(s):	Jon T. Matsukawa
*	Purpose:	Used by a server to locate clients.
*	Modifications:  All the time, it never stops!!!
*
*	COPYRIGHT (c) 1992 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/




#ifdef DOS
#include <conio.h>
#include <stdio.h>
#endif

#include <fcntl.h>
#include <string.h>
#include <tiuser.h>
#ifndef NWU
#include <process.h>
#endif
#include "proj.h"



void server()
{

/*************** Connectionless variables *****************/

	int 			flags=0;
	int			ufd;
	int			replys;
	struct t_unitdata	*ud;
	struct t_bind		*ubind;
	struct t_bind		*ret;
	char    		network[12];
	char			clientnet[12];
	char			servernet[12];
	char			net[4];
/************ End of Connectionless variables *************/
	char opt[MAXOPT];

    ud->addr.maxlen = ADDRSIZE;
    ud->opt.buf = opt;
    ud->opt.maxlen = MAXOPT;
	ud->udata.len = 0;





/**************************************************************************
*
*                   Test t_sndudata and t_rcvudata
*
**************************************************************************/



	printf("Executing t_open (**connectionless**)\n");
	printf("----------------------------------------\n");


	if ((ufd = t_open("/dev/nipx", O_RDWR, NULL)) == -1)
	{
		t_error("open of /dev/nipx failed");
		exit(1);
	}
	printf("t_open............................OK\n\n");

	printf("Executing t_alloc(**connectionless**)\n");
	printf("----------------------------------------\n");

	if((ubind=(struct t_bind *)t_alloc(ufd, T_BIND_STR, T_ALL))==NULL)

	  {
	   t_error("t_alloc of t_bind failed");
	   exit(1);
	  }
	printf("t_alloc...........................OK\n\n");


	printf("Executing t_alloc(**connectionless**)\n");
	printf("----------------------------------------\n");

	if((ret=(struct t_bind *)t_alloc(ufd, T_BIND_STR, T_ALL))==NULL)

	  {
	   t_error("t_alloc of t_bind failed");
	   exit(1);
	  }
	printf("t_alloc...........................OK\n\n");



	printf("Executing t_alloc(**connectionless**)\n");
	printf("----------------------------------------\n");

	if((ud=(struct t_unitdata *)t_alloc(ufd, T_UNITDATA_STR, T_ALL))==NULL)
	 {
	  t_error("t_alloc of t_unitdata failed");
	  exit(1);
	 }
	printf("t_alloc...........................OK\n\n");

	ubind->addr.len=ubind->addr.maxlen;

	memcpy(&network[10],UDSRVRSOCK,2);
	ubind->addr.buf=network;

	printf("Executing t_bind(**connectionless**)\n");
	printf("----------------------------------------\n");

	if (t_bind(ufd, ubind, ret) == -1)
	 {
	  t_error("bind failed");
	  exit(2);
	 }


	printf("t_bind............................OK\n\n");


	       while(t_look(ufd)!=T_DATA)
		 {
		 if(kbhit())
		  exit(1);
		 }

	printf("Executing t_rcvudata(**connectionless**)\n");
	printf("----------------------------------------\n");

	if(t_rcvudata(ufd,ud,&flags)==-1)
	 {
	  t_error("t_rcvudata failed");
	  exit(1);
	 }
	printf("t_rcvudata........................OK\n\n");


	if(strcmp(CLNTREQ,ud->udata.buf))
		{
		printf("data send != data received\n");
		exit(1);
		}

	memcpy(clientnet,ud->addr.buf,12);
	printf("***CLIENT REQUEST RECEIVED***\n");
	strcpy(ud->udata.buf,SRVRREP);
	ud->udata.len=strlen(ud->udata.buf)+1;


	printf("Executing t_sndudata(**connectionless**)\n");
	printf("----------------------------------------\n");

	for(replys=0;replys<=10;replys++)
	 {
	  if(t_sndudata(ufd,ud)==-1)
	   {
	    t_error("t_sndudata failed");
	    exit(1);
	   }
	}
	printf("t_sndudata(SERVER REPLY)..........OK\n");
	printf("%d SERVER REPLYS SENT\n\n",replys-1);

	printf("Executing t_close on ufd\n");
	printf("----------------------------------------\n");

	if(t_close(ufd)==-1)
	 {
	  t_error("t_close failed");
	  exit(1);
	 }
	printf("t_close...........................OK\n\n");


 }


