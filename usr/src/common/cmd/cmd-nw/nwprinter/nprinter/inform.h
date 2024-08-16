/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/inform.h	1.2"
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
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/inform.h,v 1.2 1994/05/12 17:08:02 novell Exp $
 */

#ifndef __RPRINTER_INFORM_H__
#define __RPRINTER_INFORM_H__

#include <printmsgtable.h>

#define MSG_NORMAL		0x01
#define MSG_VERBOSE		0x02
#define MSG_WARN		0x04
#define MSG_ERROR		0x08
#define MSG_DEBUG		0x10




void InformInit( uint16	msgTypes, char 	debugFile[]);
void InformTerminate(void);
void InformLL(
			int	printerNumber,
			char	pserverName[],
			char	message[],
			uint16	msgType);

void Inform( void *printer,
	int		message,
	uint16	msgType);

void InformMsg( void *printer,
	char	message[],
	uint16	msgType);

void
InformWithInt(
	void *printer,
	int		message,
	long	msgInt,
	uint16	msgType);

void
InformWithStr(
	void *printer,
	int		message,
	char	auxStr[],
	uint16	msgType);

void
InformPrinterWithStr(
	int		printerNumber,
	char	*pserverName,
	int		message,
	char	auxStr[],
	uint16	msgType);
void
InformConfigFilePath( 
	int     message,  
	char    *pathStr);

#endif

