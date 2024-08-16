/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/sndrcv.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sndrcv.c,v 1.2 1994/02/18 15:06:43 vtag Exp $"
/*********************************************************************
*
*	Program Name:
*	File Name:		sndrcv.c
*	Date Created:	04/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test the TLI t_snd and t_rcv functions.
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


/*--------------------------------------------------------------------
*	Constants
*-------------------------------------------------------------------*/
#define REPS	20


/*====================================================================
*
*	sndrcvclnt
*
*		This function tests the TLI t_snd function and provides support
*		in testing the TLI t_rcv function.  This routine is intended
*		to be run on the the client side of the test.
*
*	Return:	None
*
*	Parameters:
*		ps1			Session protocol stack
*		ps2			non-Session protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void sndrcvclnt (char *ps1, char *ps2, int mode, int major)
{
	char		bigbuf[MAXDATA];
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		i;
	tinfo		info;
	int		minor = 1;

	showProtoStk("TESTING t_snd",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_snd call, small amount of data, no flags
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(ps1,mode,&info,TNOERR,UNBND);
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
	strcpy(dbuf,DATA);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);

/*--------------------------------------------------------------------
*	2. Make a valid t_snd call, small amount of data, T_MORE flag set
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i = 0; i < REPS; i++) {
		tlisnd(fd,dbuf,strlen(dbuf)+1,T_MORE,TNOERR,DATAXFER);
	}

	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,DATAXFER);


/*--------------------------------------------------------------------
*	3. Make a valid t_snd call, large amount of data, no flags
*-------------------------------------------------------------------*/

	minor++;
	showtnum(major,minor);
	memcpy(dbuf,(char *)&info.tsdu,sizeof(info.tsdu));
	tlisnd(fd,dbuf,sizeof(info.tsdu),0,TNOERR,DATAXFER);
	strcpy(bigbuf,MuchoData);
	if (info.tsdu == -1)
		tlisnd(fd,bigbuf,strlen(bigbuf)+1,0,TNOERR,DATAXFER);
	else
	{
		bigbuf[info.tsdu-1] = '\0';
		tlisnd(fd,bigbuf,(unsigned)info.tsdu,0,TNOERR,DATAXFER);
	}

/*--------------------------------------------------------------------
*	4. Try sending data when a disconnect packet has been received
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	ClntSync(Srvraddr,major,minor);
#if XTI | NWU
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TLOOK,DATAXFER);
#else
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,IDLE);
#endif
	ClntSync(Srvraddr,major,minor);
	tlircvdis(fd,NULL,TNOERR,IDLE);

/*--------------------------------------------------------------------
*	5. TOUTSTATE error new to t_snd in new XTI libs
*	Try sending data using SPX in the T_IDLE state
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
#if XTI | NWU
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TOUTSTATE,IDLE);
#else
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOERR,IDLE);
#endif
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	6. Cause a TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TSYSERR error
*-------------------------------------------------------------------*/
/*
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps1,mode,NULL,TNOERR,UNBND);
	tlisnd(fd,dbuf,strlen(dbuf)+1,0,TSYSERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
*/

/*--------------------------------------------------------------------
*	Server TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Server TNODATA error
*-------------------------------------------------------------------*/
	minor++;
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
	ClntSync(Srvraddr,major,minor);

/*--------------------------------------------------------------------
*	Server TLOOK error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	ClntSync(Srvraddr,major,minor);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Server TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
}


/*====================================================================
*
*	sndrcvsrvr
*
*		This function provides support in testing the TLI t_snd
*		function and tests the TLI t_rcv function.  This routine is
*		intended to be run on the server side of the test.
*
*	Return:	
*
*	Parameters:
*		ps1			Session protocol stack
*		ps2			non-Session protocol stack
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void sndrcvsrvr (char *ps1, char *ps2, int mode, int major)
{
	char		bigbuf[MAXDATA];
	tbindptr	bind;
	tcallptr	call;
	char		dbuf[DATASIZE];
	int		fd;
	int		flags;
	int		minor = 1;
	char		*ptr;
	int		space;
	long		temp;
	int             buff;
	int             cnt = 1;
	int             dsize;
	int             nextpak = 0;

	showProtoStk("TESTING t_rcv",mode);

/*--------------------------------------------------------------------
*	1. Make a valid t_rcv call, small amount of data, no flags set
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
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
	if (strcmp(dbuf,DATA))
		printf("Data sent != data received\n");

/*--------------------------------------------------------------------
*	2. Make a valid t_rcv call, small amount of data, T_MORE flag set
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	buff=sizeof(DATA);
	do
	{
		cnt++;
		memset(dbuf, '\0', sizeof(dbuf));
		tlircv(fd,dbuf,buff,&flags,TNOERR,DATAXFER);
		dsize=strlen(dbuf);
		if(Verbose)
			printf("Data Recieved: %s\n",dbuf);

		if(cnt > 10)
		  buff=10;

/*____________________________________________________________________
*       Verify that a packet following a first buffered half is recieved
*____________________________________________________________________*/

		if((nextpak > 0) && (dsize !=6))
		      printf("T_MORE error with small buffer, trailing half of packet lost or incorrect\n");

		if(dsize==10)
		  {
		    nextpak=6;
		    buff=7;  
 		  }
		else
		  nextpak=0;

/*____________________________________________________________________
*       Compare Expected data with actual data received (3 possible cases)
*____________________________________________________________________*/



		switch (dsize) {
		  case 16: if(strcmp (dbuf, DATA)) {
				printf("DATA sent != data received\n");
				printf("dbuf = %s \n", dbuf);
				printf("DATA = %s \n", DATA);
			    }
			    break;

		   case 10: if(strcmp (dbuf, DATA_A)){
				printf("DATA sent != data received\n");
				printf("dbuf   = %s \n", dbuf);
				printf("DATA_A = %s \n", DATA_A);
			    }
			    break;

		   case 6:  if(strcmp (dbuf, DATA_B)){
				printf("DATA sent != data received \n");
				printf("dbuf   = %s \n", dbuf);
				printf("DATA_B = %s \n", DATA_B);
			    }
			    break;

		   default: printf("Error Unexpected Data Size Received \n");
			    printf("Data received = %s \n", dbuf);
			    printf("data size     = %d \n", dsize);
			    break;
		}
	}
	while (flags & T_MORE);
/*--------------------------------------------------------------------
*	3. Make a valid t_rcv call, large amount of data, no flags set
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TNOERR,DATAXFER);
	memcpy((char *)&temp,dbuf,sizeof(temp));

	if (temp > 534)
	  {	
	    printf("temp = %ld \n",temp);
	    MuchoData[temp-1] = '\0';
	  }
	
	memset(bigbuf,0,MAXDATA);
	ptr = bigbuf;
	space = MAXDATA;
	do
	{
		tlircv(fd,ptr,space,&flags,TNOERR,DATAXFER);
		ptr = strchr(ptr,'\0');
		space = ptr - bigbuf;
	}
	while (flags & T_MORE);
	if (strcmp(bigbuf, MuchoData))
	{
		printf("Data sent != data received\n");
		printf("Strlen of bigbuf %d\n",strlen(bigbuf));
		exit(1);
	}
/*--------------------------------------------------------------------
*	4. Send a disconnect packet
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlisnddis(fd,NULL,TNOERR,IDLE);
	SrvrSync(1,major,minor);
	SrvrSync(1,major,minor);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	5. Client attempts to send data in the T_IDLE state
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	6. Client TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	7. Client TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);

/*--------------------------------------------------------------------
*	Client TSYSERR error
*-------------------------------------------------------------------*/
/*
	minor++;
	showtnum(major,minor);
*/

/*--------------------------------------------------------------------
*	8. Cause a TBADF error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	9. Cause a TNODATA error
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
	if (mode == ASYNC)
		tlircv(fd,dbuf,sizeof(dbuf),&flags,TNODATA,DATAXFER);
	SrvrSync(1,major,minor);

/*--------------------------------------------------------------------
*	10. Cause a TLOOK error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	SrvrSync(1,major,minor);
#if XTI | NWU
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TLOOK,DATAXFER);
#else
	tlircv(fd,dbuf,sizeof(dbuf),&flags,TLOOK,IDLE);
#endif
	tlircvdis(fd,NULL,TNOERR,IDLE);
	tlifree(fd,(char *)bind,T_BIND,TNOERR,IDLE);
	tlifree(fd,(char *)call,T_CALL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	11. Cause a TNOTSUPPORT error
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	fd = tliopen(ps2,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tlircv(fd,dbuf,strlen(dbuf),&flags,TNOTSUPPORT,IDLE);
	tliclose(fd,TNOERR,UNINIT);
}
