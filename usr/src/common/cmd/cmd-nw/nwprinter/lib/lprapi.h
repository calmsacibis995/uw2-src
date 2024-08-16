/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/lprapi.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/lprapi.h,v 1.1 1994/02/11 18:18:42 nick Exp $
 */
#ifndef __LPRAPI_H__
#define __LPRAPI_H__

int LprOpen(char *hostName);
char *LprGetOpenAddrStr(void);
int LprStartPrinter(int fd, char *printerName);
int LprRequestQueueState(int fd, char *printerName, char *userName);
int LprRemoveQueueEntry(int fd, char *printerName, char *userName);
int LprClose(int fd);
int LprGetAck(int fd);
int LprSendFileDataCommand(int fd, int size, char *fileName);
int LprSendControlDataCommand(int fd, int size, char *fileName);
int LprSendData(int fd, int size, char *data, int *retCount);
int LprSendDataNoBlock(int fd, int size, char *data, int *retCount);
int LprSendDataEnd(int fd );
char *LprGetHostName(void);
int LprReadQueueState(int fd, int maxSize, char *buf);
int LprSetNoBlock(int fd);
int LprSetBlock(int fd);
#endif
