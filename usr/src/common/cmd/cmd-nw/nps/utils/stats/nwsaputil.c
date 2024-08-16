/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/nwsaputil.c	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nwsaputil.c,v 1.11 1994/09/01 21:39:56 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "nwmsg.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/sap_app.h>
#include "util_proto.h"
#include "nwconfig.h"
#include "npsmsgtable.h"

static uint8	*sname;
static uint16	socket, type;
static int 		topt = 0;
static int 		sopt = 0;
static int 		nopt = 0;
static int 		aopt = 0;
static int 		dopt = 0;
static int 		qopt = 0;

extern int	optind;
extern char	*optarg;
char titleStr[] = "NWSAPUTIL";

static int AddInfo( void);
static int DeleteInfo( void);
static int GetInfo( void);

/*ARGSUSED*/
int
main( int argc, char *argv[] )
{
	int 	opt;
	char	*ptr;
	int 	terr, err = 0;
	int ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_SUTIL, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Error exit.\n",
			titleStr, ccode);
		exit(1);
	}
 
/*
**	Process passed in arguments
*/
	while((opt = getopt(argc, argv, "n:t:s:adq?")) != (char)-1)
	{
		switch(opt)
		{
			case 'n':	/* Server Name */
				nopt++;
				sname = (uint8 *)optarg;
				break;

			case 't':	/* Server Type */
				topt++;
				type = (uint16)strtoul( optarg, &ptr, 0 );
				break;

			case 's':	/* Server Socket */
				sopt++;
				socket = (uint16)strtoul( optarg, &ptr, 0 );
				break;

			case 'a':	/* Add a server to table */
				if(qopt || dopt)
				{
					printf(MsgGetStr(P_EXCLUSIVE));
					exit( 1 );
				}
				aopt++;
				break;

			case 'd':	/* Delete a server from table */
				if(qopt || aopt)
				{
					printf(MsgGetStr(P_EXCLUSIVE));
					exit( 1 );
				}
				dopt++;
				break;

			case 'q':	/* Query request */
				if(aopt || dopt)
				{
					printf(MsgGetStr(P_EXCLUSIVE));
					exit( 1 );
				}
				qopt++;
				break;

			case '?':
				err++;
				break;

			default:
				err++;
				break;
		}
	}

/*
**	Check for valid arguments, give usage report if bad args presented
*/
	for( ; optind < argc; optind++ )
	{
		err++;
		fprintf(stderr, MsgGetStr(P_UNKNOWN), argv[optind]);
	}

	if(err)
	{
		fprintf(stderr, MsgGetStr(P_USAGE_HEAD));
		fprintf(stderr, MsgGetStr(P_USAGE_1));
		fprintf(stderr, MsgGetStr(P_USAGE_2));
		fprintf(stderr, MsgGetStr(P_USAGE_3));
		fprintf(stderr, MsgGetStr(P_USAGE_4));
		fprintf(stderr, MsgGetStr(P_USAGE_5));
		fprintf(stderr, MsgGetStr(P_USAGE_6));
		exit( 0 );
	}

/*
**	Check to make sure all needed args were passed in for each action
**	If everything is OK, do it.
*/
	if(aopt)
	{
		if(!sopt || !topt)
		{
			fprintf(stderr, MsgGetStr(P_MISSING_ARG1));
			exit ( 1 );
		}
		if((err = AddInfo()) < 0)
		{
			fprintf(stderr, MsgGetStr(P_ADD_FAIL));
			SAPPerror( err, "" );
		}
	}
	else if(dopt)
	{
		if(!topt)
		{
			fprintf(stderr, MsgGetStr(P_MISSING_ARG2));
			exit ( 1 );
		}
		if((err = DeleteInfo()) < 0)
		{
			fprintf(stderr, MsgGetStr(P_DELETE_FAIL));
			SAPPerror( err, "" );
		}
	}
	else if(qopt)
		err = GetInfo();
	else
	{
		fprintf(stderr, MsgGetStr(P_NO_ACTION));
		exit( 1 );
	}

	return( err );		
}

/*
 * int strcmpi(s1, s2)
 *
 *	Like strcmp(), but case insensitive.
 */

static int
strcmpi(char * s1, char * s2)
{
	int	r;

	while (*s1 && *s2)
		if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
			return r;
	return (int) (toupper(*s1) - toupper(*s2));
}


static int
AddInfo ( void )
{
	char serverName[NWCM_MAX_STRING_SIZE];

	if(!nopt)
	{
		if(NWCMGetParam( "server_name", NWCP_STRING, serverName ) != 0)
			return( -SAPL_NWCM );

		sname = (uint8 *)serverName;		
	}
	return( SAPAdvertiseMyServer(type, sname, socket, SAP_ADVERTISE_FOREVER) );
}


static int
DeleteInfo ( void )
{
	char serverName[NWCM_MAX_STRING_SIZE];

	if(!nopt)
	{
		if(NWCMGetParam( "server_name", NWCP_STRING, serverName ) != 0)
			return( -SAPL_NWCM );

		sname = (uint8 *)serverName;		
	}
	return( SAPAdvertiseMyServer(type, sname, socket, SAP_STOP_ADVERTISING) );
}

static int
GetInfo( void)
{
	int 	ret, len, i, index = 0;
	PersistList_t persist;


	while((ret = SAPListPermanentServers(&index, &persist, 1)) == 1)
	{

/*
**	Check the returned records against any masks that have been supplied
*/
		if(topt)
			if(type != persist.ServerType)
				continue;

		if(sopt)
			if(socket != persist.ServerSocket)
				continue;

		len = strlen((char *)persist.ServerName);
		if(nopt)
			if(strcmpi((char *)persist.ServerName, (char *)sname))
				continue;

/*
**	print the persist file record
*/
		printf("%s", persist.ServerName);
		for(i=0; i<NWMAX_SERVER_NAME_LENGTH + 2 - len; i++)
			printf(" ");
		printf("0x%04x   ", persist.ServerType);
		printf("0x%04x", persist.ServerSocket);
		printf("\n");
	}
	if(ret < 0)
		printf(MsgGetStr(P_INFO_UNAVAIL));

	return(0);
}
