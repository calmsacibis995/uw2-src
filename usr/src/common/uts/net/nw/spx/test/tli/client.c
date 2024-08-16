/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/client.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: client.c,v 1.2 1994/02/18 15:05:54 vtag Exp $"

/*********************************************************************
*
*	Program Name:   csearch.exe
*	File Name:	client.c
*	Date Created:	01/24/92
*	Version:	1.0
*	Programmer(s):	Jon T. Matsukawa
*	Purpose:	Used by a client to locate the test server.
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


void client()
{


/*************** Connectionless variables *****************/

	int			flags;
	int			ufd;
	int			requests;
	struct t_unitdata	*ud;
	char 			string[30];
	char    		network[12];
	char			clientnet[12];
	char			servernet[12];

/**********************************************************/



/**************************************************************************
*
*             Read net.ini for network to find Server and
*              convert the string into HEX;
*
**************************************************************************/


	int			ii;
	char			filenet[8];
	char			reversenet[4];
	char			net[4];
	char			*netptr;
	FILE			*file_ptr;

	char			name[3];


	if((file_ptr = fopen("net.ini", "rb")) == NULL)
	{
	  printf("Unable to Open file 'NET.INI'");
	  exit(0);
	}


	fread(filenet,8,1,file_ptr);
	fseek(file_ptr,10,SEEK_SET);
	fread(name,3,1,file_ptr);

	sscanf(filenet,"%lx",reversenet);
	netptr=(char *)&reversenet;
	for(ii=0;ii<4;ii++)
	 net[3-ii] = netptr[ii];



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

	printf("Executing t_bind(**connectionless**)\n");
	printf("----------------------------------------\n");

	if (t_bind(ufd, NULL, NULL) == -1)
	 {
	  t_error("bind failed");
	  exit(2);
	 }


	printf("t_bind............................OK\n\n");

	printf("Executing t_alloc(**connectionless**)\n");
	printf("----------------------------------------\n");

	if((ud=(struct t_unitdata *)t_alloc(ufd, T_UNITDATA_STR, T_ALL))==NULL)
	 {
	  t_error("t_alloc failed");
	  exit(1);
	 }
	printf("t_alloc...........................OK\n\n");
	ud->addr.len=ud->addr.maxlen;
	memcpy(&network[0],&net[0],4);
	memcpy(&network[4],BROADCAST,6);
	memcpy(&network[10],UDSRVRSOCK,2);
	ud->addr.buf=network;
	strcpy(ud->udata.buf,CLNTREQ);
	ud->udata.len=strlen(ud->udata.buf)+1;


	printf("Executing t_sndudata(**connectionless**)\n");
	printf("----------------------------------------\n");

	
	for(requests=0;requests<=10;requests++)
	 {
	  if(t_sndudata(ufd,ud)==-1)
	   {
	    t_error("t_sndudata failed");
	    exit(1);
	   }
	 }
	printf("t_sndudata(CLIENT REQUEST)........OK\n");
	printf("%d SERVER REQUEST PACKETS SENT\n\n",requests-1);


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
	memcpy(servernet,ud->addr.buf,12);


	if(strcmp(SRVRREP,ud->udata.buf))
		{
		printf("Invalid Server Reply\n");
		exit(1);
		}

	printf("***SERVER RESPONSE RECEIVED***\n\n");

	printf("Executing t_close on ufd\n");
	printf("----------------------------------------\n");

	if(t_close(ufd)==-1)
	 {
	  t_error("t_close failed");
	  exit(1);
	 }
	printf("t_close...........................OK\n\n");



}
/****************************************************************************/



