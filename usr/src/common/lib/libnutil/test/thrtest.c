/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/thrtest.c	1.7"
#include	<stdio.h>
#include	<thread.h>
#include 	<sys/signal.h>
#include	<stdlib.h>
#include	<sys/sap_app.h>
#include	"thrtest.h"
#include	"nwmsg.h"
#include	"npsmsgtable.h"
#include	"printmsgtable.h"
#include	"utilmsgtable.h"
#include	"nwcmmsgs.h"

int		globvar = 0;

thread_t	t1,t2,t3;

void  		*thread1 ( );
void  		*thread2 ( );
void  		*thread3 ( );


#ifdef _REENTRANT

THREAD_KEY_T _doit;

void
_create_keys()
{
	/* create keys for per-thread storage */
	THR_KEYCREATE(&_doit, &_free_ntoa_novell_keytbl);
}


/* this free function is called automatically by thr_exit */

void
_free_ntoa_novell_keytbl(void *t)
{
	struct _ntoa_novell_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _ntoa_novell_tsd *)t;

	free (t);
}

#else
#define _create_keys()
#endif /* _REENTRANT */


main ()

{
	int			i, j;
	int			arg;
	int			ret;
	

/*
**	map memory for the threads
*/
	if((i = SAPMapMemory()) != 0)
	{
		printf("SAPMapMemory failure #%d\n", i);
		exit( 1 );
	}

	printf ("Thread test program\n");
	_create_keys();

	arg = 1;
	ret = THR_CREATE ( NULL, 0, thread1, (void *)&arg, THR_BOUND, &t1 );
	if ( ret ) {
		printf ( "Unable to create thread 1 -- error %d\n",ret );
		exit ( 1 );
	}
	printf ( "Thread 1 running!\n");

	arg = 2;
	ret = THR_CREATE ( NULL, 0, thread2, (void *)&arg, THR_BOUND, &t2 );
	if ( ret ) {
		printf ( "Unable to create thread 2 -- error %d\n",ret );
		exit ( 2 );
	}
	printf ( "Thread 2 running!\n");

	arg = 3;
	ret = THR_CREATE ( NULL, 0, thread3, (void *)&arg, THR_BOUND, &t3 );
	if ( ret ) {
		printf ( "Unable to create thread 3 -- error %d\n",ret );
		exit ( 3 );
	}
	printf ( "Thread 3 running!\n");

	sleep ( 20 );
/*
**	unmap memory for the threads
*/
	SAPUnmapMemory();
	printf("Exiting\n");
}

void *
thread1 ( )
{
	int			i=0;
	struct sigaction	jsigstruct;
	extern void		dosig1(int);

	jsigstruct.sa_handler = &dosig1;
	sigemptyset(&jsigstruct.sa_mask);
	jsigstruct.sa_flags = 0;

	sigaction(i,&jsigstruct, NULL);

	doitfs();
}

void *
thread2 ( )
{
	int			i=0;
	struct sigaction	jsigstruct;
	extern void		dosig2(int);

	jsigstruct.sa_handler = &dosig2;
	sigemptyset(&jsigstruct.sa_mask);
	jsigstruct.sa_flags = 0;

	sigaction(i,&jsigstruct, NULL);

	doitds();
}

void *
thread3 ( )
{
	int			i=0;
	struct sigaction	jsigstruct;
	extern void		dosig3(int);

	jsigstruct.sa_handler = &dosig3;
	sigemptyset(&jsigstruct.sa_mask);
	jsigstruct.sa_flags = 0;

	sigaction(i,&jsigstruct, NULL);

	doitps();
}


void
dosig1(int signo)
{
	printf("dosig() of thread 1 signo = %d\n", signo);
	exit(2);
}

void
dosig2(int signo)
{
	printf("dosig() of thread 2 signo = %d\n", signo);
	exit(3);
}

void
dosig3(int signo)
{
	printf("dosig() of thread 3 signo = %d\n", signo);
	exit(4);
}



doitfs()
{
	uint16	serverType;
	int	err, i, index = 0;
	int	count = 0;
	SAPI	sapResp[4];
	FILE	*zfd;

/*
**	Set a SAPI message domain
*/
	if( MsgBindDomain(MSG_DOMAIN_SAPI, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR) != SUCCESS)
	{
		fprintf(stderr, "THREAD1: Can't get SAPI message catalogue!\n");
		return;
	}
	printf("--------------------------------------------------------\n");
	printf("Thread 1, doitfs, 3 SAPI Message domain messages\n\n");
	printf(MsgGetStr(A_SAP_STATS_UNAVAIL));
	printf(MsgGetStr(A_NEWLINE));
	printf(MsgGetStr(A_KNOWN_SERVERS), 2);
	printf(MsgGetStr(A_UNKNOWN_ARG), "DOITFS Test");
	printf("--------------------------------------------------------\n");

/*
**	Issue a Get all File Servers request
*/
	serverType = FILE_SERVER_TYPE;

	zfd = fopen("/tmp/FSstuff", "w");
	do {
		err = SAPGetAllServers( serverType, &index, sapResp, 4);
		if(err)
		{
			for(i=0; i<4; i++)
				fprintf(zfd, "DOITFS Type %x, Reply name is %s\n", serverType,
							sapResp[i].serverName);
		}
		count += err;
	}while(err == 4);
	fprintf(zfd, "DOITFS got %d replies\n", count);
	fclose(zfd);	
	
/*
**	Print again to ensure domain didn't change	
*/
	printf("--------------------------------------------------------\n");
	printf("RETRY: Thread 1, doitfs, 3 SAPI Message domain messages\n\n");
	printf(MsgGetStr(A_SAP_STATS_UNAVAIL));
	printf(MsgGetStr(A_NEWLINE));
	printf(MsgGetStr(A_KNOWN_SERVERS), 2);
	printf(MsgGetStr(A_UNKNOWN_ARG), "DOITFS Test");
	printf("--------------------------------------------------------\n");
}

doitds()
{
	uint16	serverType;
	int	err, i, index = 0;
	int	count = 0;
	SAPI	sapResp[4];
	FILE	*ufd;

/*
**	Set a Print message domain
*/
	if( MsgBindDomain(MSG_DOMAIN_NPRINT, MSG_DOMAIN_PRINT_FILE, MSG_PRINT_REV_STR) != SUCCESS)
	{
		fprintf(stderr, "THREAD2: Can't get NPRINT message catalogue!\n");
		return;
	}
	printf("--------------------------------------------------------\n");
	printf("Thread 2, doitds, 3 NPRINT Message domain messages\n\n");
	printf(MsgGetStr(RPMSG_PRINTER_READY));
	printf("\n");
	printf(MsgGetStr(RPMSG_NO_ADVERTISE));
	printf("\n");
	printf(MsgGetStr(RPMSG_RPRINTER_IN_USE));
	printf("\n");
	printf("--------------------------------------------------------\n");

/*
**	Issue a Get all Directory Servers request
*/
	serverType = DIRECTORY_SERVER_TYPE;
	ufd = fopen("/tmp/DSstuff", "w");

	do {
		err = SAPGetAllServers( serverType, &index, sapResp, 4);
		if(err)
		{
			for(i=0; i<4; i++)
				fprintf(ufd, "DOITDS Type %x, Reply name is %s\n", serverType,
							sapResp[i].serverName);
		}
		count += err;
	}while(err == 4);
	fprintf(ufd, "DOITDS got %d replies\n", count);
	fclose(ufd);	
	
/*
**	Print again to ensure domain didn't change	
*/
	printf("--------------------------------------------------------\n");
	printf("RETRY: Thread 2, doitds, 3 NPRINT Message domain messages\n\n");
	printf(MsgGetStr(RPMSG_PRINTER_READY));
	printf("\n");
	printf(MsgGetStr(RPMSG_NO_ADVERTISE));
	printf("\n");
	printf(MsgGetStr(RPMSG_RPRINTER_IN_USE));
	printf("\n");
	printf("--------------------------------------------------------\n");
}

doitps()
{
	uint16	serverType;
	int	err, i, index = 0;
	int	count = 0;
	SAPI	sapResp[4];
	FILE	*tfd;

/*
**	Set a NWCM message domain
*/
	if( MsgBindDomain(MSG_DOMAIN_NWCM, MSG_DOMAIN_NWCM_FILE, MSG_NWCM_REV_STR) != SUCCESS)
	{
		fprintf(stderr, "THREAD3: Can't get NWCM message catalogue!\n");
		return;
	}
	printf("--------------------------------------------------------\n");
	printf("Thread 3, doitps, 3 NWCM Message domain messages\n\n");
	printf(MsgGetStr(NWCM_NOT_FOUND));
	printf("\n");
	printf(MsgGetStr(NWCM_CONFIG_READ_ONLY));
	printf("\n");
	printf(MsgGetStr(NWCM_NOT_IMPLEMENTED));
	printf("\n");
	printf("--------------------------------------------------------\n");

/*
**	Issue a Get all Directory Servers request
*/
	serverType = PRINT_SERVER_TYPE;
	tfd = fopen("/tmp/PSstuff", "w");

	do {
		err = SAPGetAllServers( serverType, &index, sapResp, 1);
		if(err)
		{
			for(i=0; i<1; i++)
				fprintf(tfd, "DOITPS Type %x, Reply name is %s\n", serverType,
							sapResp[i].serverName);
		}
		count += err;
	}while(err == 1);
	fprintf(tfd, "DOITPS got %d replies\n", count);
	fclose(tfd);	
	
/*
**	Print again to ensure domain didn't change	
*/
	printf("--------------------------------------------------------\n");
	printf("RETRY: Thread 3, doitps, 3 NWCM Message domain messages\n\n");
	printf(MsgGetStr(NWCM_NOT_FOUND));
	printf("\n");
	printf(MsgGetStr(NWCM_CONFIG_READ_ONLY));
	printf("\n");
	printf(MsgGetStr(NWCM_NOT_IMPLEMENTED));
	printf("\n");
	printf("--------------------------------------------------------\n");

}
