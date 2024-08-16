/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/rprinter.h	1.3"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/rprinter.h,v 1.4 1994/08/18 16:32:42 vtag Exp $
 */
#include "sys/nwportable.h"
#include "rptune.h"
#include "spxapi.h"
#include "prtapi.h"

#define RP_ENTRY_STATUS_INIT_HAVE_NAME		1
#define RP_ENTRY_STATUS_INIT_HAVE_ADDRESS	2
#define RP_ENTRY_STATUS_INIT_HAVE_SOCKET	3
#define RP_ENTRY_STATUS_WAITING_FOR_JOB		4
#define RP_ENTRY_STATUS_PROCESSING_A_JOB	5

#define PJ_STATUS_PROCESS_JOB				1
#define PJ_STATUS_PAUSED					2
#define PJ_STATUS_RELEASED					3

#define RP_TROUBLE_CODE_OFFLINE				0x01
#define RP_TROUBLE_CODE_OUT_OF_PAPER		0x02

#define NWMAX_OBJECT_NAME_LENGTH			48

typedef struct {
	uint8	printerNumber;
	uint8	needBlocks;
	uint8	finishedBlocks;
	uint8	troubleCode;
	uint8	inSideband;
} RPrinterStatus_t;

typedef struct {
	int				 entryStatus;
	int				 toBeDeleted;
	int				 problemCount;
	int				 idleCount;
	char			 pserverName[NWMAX_OBJECT_NAME_LENGTH];
	RPrinterStatus_t printerStatus;
	int				 spxTransportOpen;
	int				 spxConnection;
	SPXHandle_t		 spxHandle;
	SPXAddress_ta	 pserverSPXAddress;
	PRTHandle_t		 hostPrinterHandle;
	char			 hostPrinterName[MAX_HOST_PRINTER_NAME_LENGTH];
	int				 hostNameChange;
	int				 hasCommand;
	int				 endOfJobCountDown;
	int				 printJobStatus;
	long			 printJobSize;
} RPrinterInfo_t;
