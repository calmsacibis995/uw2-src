/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/inform.c	1.5"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/inform.c,v 1.6 1994/08/18 06:01:52 novell Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <nwmsg.h>
#include <errno.h>
#include "rprinter.h"
#include "inform.h"
#include "config_proto.h"

static char *getDateAndTimeStr(void);

static FILE 	*informFP;
static FILE 	*debugFP;
static uint16	 msgTypesToInform;
static char 	*debugFilePath;

void
InformInit(
	uint16	msgTypes,
	char 	debugFile[])
{
	CONFIG_CONSOLE_DEVICE_DEFN;

	msgTypesToInform = msgTypes;
	informFP = fopen(CONFIG_CONSOLE_DEVICE, "w" );
	if (informFP == NULL) {
		fprintf(stderr, "FATAL ERROR: %s on Console Open\n", strerror(errno));
		exit(1);
	}
	setbuf( informFP, (char *) 0 );

	debugFilePath = debugFile;
	if (debugFilePath) {
		debugFP = fopen( debugFilePath, "a" );
		setbuf( debugFP, (char *) 0 );
	}

	if (MsgBindDomain(MSG_DOMAIN_NPRINT, MSG_DOMAIN_PRINT_FILE, MSG_PRINT_REV_STR)
			!= SUCCESS) {
		fprintf(stderr,"FATAL ERROR: Error in message setup.\n"); 
		exit(1);
	}

}


void
InformTerminate(void)
{
	if (debugFilePath)
		fclose( debugFP );
	fclose( informFP );
}

void
InformLL(
	int		printerNumber,
	char	pserverName[],
	char	message[],
	uint16	msgType)
{

	if (msgType & msgTypesToInform)
	{
		if (msgType == MSG_NORMAL || msgType == MSG_VERBOSE) {

			if (printerNumber >= 0)
				fprintf( informFP, "NPRINTER:(%s:%d): %s\n",
					pserverName, printerNumber, message);
			else
				fprintf( informFP, "NPRINTER:%s\n", message);
		} else {
			if (printerNumber >= 0)
                                fprintf( informFP, "(%s:%d): %s\n",
                                        pserverName, printerNumber, message);
                        else
                                fprintf( informFP, "%s\n", message);
		}
	}
#ifdef DEBUG
	if (msgType == MSG_NORMAL) {
		fprintf( stdout, getDateAndTimeStr() );
		fprintf( stdout, "NPRINTER:(%s:%d): %s\n",
					pserverName, printerNumber, message);
	} else {
		fprintf( stdout, getDateAndTimeStr() );
                fprintf( stdout, "(%s:%d): %s\n",
                                        pserverName, printerNumber, message);
	}
#endif
	if (debugFilePath)
	{
		if (printerNumber >= 0)
		{
			fprintf( debugFP, getDateAndTimeStr() );
			fprintf( debugFP, "NPRINTER:(%s:%d): %s\n",
				pserverName, printerNumber, message );
		}
		else
		{
			fprintf( debugFP, getDateAndTimeStr() );
			fprintf( debugFP, "NPRINTER:%s\n", message );
		}
	}
}


void
Inform(
	void *printerArg,
	int		message,
	uint16	msgType)
{

	RPrinterInfo_t *printer = printerArg;
	
	if (printer)
		InformLL( printer->printerStatus.printerNumber,
			printer->pserverName, MsgGetStr(message), msgType );
	else
		InformLL( -1, "", MsgGetStr(message), msgType );
}

void 
InformMsg(
	void *printerArg,
	char	message[],
	uint16	msgType)
{
	RPrinterInfo_t *printer = printerArg;

	if (printer)
		InformLL( printer->printerStatus.printerNumber,
			printer->pserverName, message, msgType );
	else
		InformLL( -1, "", message, msgType );
}

void
InformWithInt(
	void *printerArg,
	int		message,
	long	msgInt,
	uint16	msgType)
{
	char msgStr[80];
	RPrinterInfo_t *printer = printerArg;

	sprintf( msgStr, MsgGetStr(message), msgInt );

	if (printer)
		InformLL( printer->printerStatus.printerNumber,
			printer->pserverName, msgStr, msgType );
	else
		InformLL( -1, "", msgStr, msgType );
}


void
InformWithStr(
	void *printerArg,
	int		message,
	char	auxStr[],
	uint16	msgType)
{
	char msgStr[250];
	RPrinterInfo_t *printer = printerArg;

	sprintf( msgStr, MsgGetStr(message), auxStr );

	if (printer)
		InformLL( printer->printerStatus.printerNumber,
			printer->pserverName, msgStr, msgType );
	else
		InformLL( -1, "", msgStr, msgType );
}

void
InformPrinterWithStr(
	int		printerNumber,
	char	*pserverName,
	int		message,
	char	auxStr[],
	uint16	msgType)
{
	char msgStr[250];

	sprintf( msgStr, MsgGetStr(message), auxStr );

	if (printerNumber != -1)
		InformLL(printerNumber, pserverName, msgStr, msgType );
	else
		InformLL( -1, "", msgStr, msgType );
}

void
InformConfigFilePath(
	int		message,
	char 	*pathStr)
{
	/* Make sure domain is active since this is called before InitInform*/
	if (MsgBindDomain(MSG_DOMAIN_NPRINT, MSG_DOMAIN_PRINT_FILE, MSG_PRINT_REV_STR)
			!= SUCCESS) {
		fprintf(stderr,"FATAL ERROR: Error in message setup.\n"); 
	}
	fprintf(stderr, MsgGetStr(message), pathStr);
}


static char *
getDateAndTimeStr(void)
{
	time_t currTime;

	currTime = time( (long *) 0 );

	return asctime( localtime( &currTime ) );
}










