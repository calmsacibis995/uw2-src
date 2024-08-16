/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/psapi.c	1.2"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/psapi.c,v 1.3 1994/08/18 16:27:16 vtag Exp $";
#endif

#include "spxapi.h"
#include "psapi.h"
#include "pstdr_proto.h"

#define MAX_ERROR_MESSAGE_LENGTH			18


/*********************************************************************
** BEGIN_MANUAL_ENTRY( PSRequestRemotePrinter (PS API), \
**                     api/utils/rprinter/psapi/psreques )
**			A PSERVER client protocol routine used to request that a
**			specified printer slot be assigned to us
**
** SYNOPSIS
**			#include <psapi.h>
**
**			int ccode;
**			SPXHandle_t spxHandle;
**			uint8 printerNumber;
**			PSRPrinterInfo_t rprinterInfo;
**
**			ccode = PSRequestRemotePrinter( &spxHandle,
**				printerNumber, &rprinterInfo );
**
** INPUT
**			spxHandle			a handle to the SPX protocol
**								connection to the PSERVER
**
**			printerNumber		the number of the printer slot
**								desired on the PSERVER
**
** OUTPUT
**			spxHandle			when ccode receives -1,
**								the fields errnoType and errno
**								are set to indicate the error
**								condition, otherwise they remain
**								unchanged
**
**			rprinterInfo		when ccode receives 0, all of the
**								fields in this structure are set;
**								Portable NetWare RPRINTER ignores
**								all of the fields in this structure
**								except the socket number
**
** RETURN VALUES
**			ccode				receives either 0 for success or
**								-1 for failure
**
**			When ccode receives -1 for failure, the errno field
**			of the SPX handle may be set to any of the following
**			values when errnoType is SPXET_USER:
**				PSE_NOT_ENOUGH_MEMORY		0x0301
**				PSE_NO_SUCH_PRINTER			0x0302
**				PSE_ALREADY_IN_USE			0x0308
**				PSE_DOWN					0x030C
**
**			For other values of errno see standard AT&T TLI
**			documentation.
**
** DESCRIPTION
**			This is the Portable version of the routine as
**			documented in "NETWARE PRINT SERVER".
**
**			Significant global routines called:
**				TDRRequestRPrinterQuery
**				SPXSend
**				SPXReceive
**				TDRRequestRPrinterResponse
**
** SEE ALSO
**			PSDisplayError
**
** END_MANUAL_ENTRY ( 11/10/90 )
**/

int
PSRequestRemotePrinter(
	SPXHandle_t		 *spxHandle,
	uint8			  printerNumber,
	PSRPrinterInfo_t *rprinterInfo)
{
	int		length;
	int		more;
	uint16	ccode;
	uint8	queryPacket[REQUEST_RPRINTER_QUERY_LENGTH];
	uint8	responsePacket[REQUEST_RPRINTER_RESPONSE_LENGTH];

	TDRRequestRPrinterQuery( printerNumber, queryPacket );

	more = FALSE;
	length = REQUEST_RPRINTER_QUERY_LENGTH;
	if (SPXSend( spxHandle, queryPacket, length, NULL, more ) == FAILURE)
		return FAILURE;

	length = REQUEST_RPRINTER_RESPONSE_LENGTH;
	if (SPXReceive( spxHandle, responsePacket, &length) == FAILURE)
		return FAILURE;

	TDRRequestRPrinterResponse( responsePacket, length, &ccode,
		rprinterInfo );
	if (ccode) {
		spxHandle->errnoType = SPXET_USER;
		spxHandle->errno = ccode;
		return FAILURE;
	}

	return SUCCESS;
}


/*********************************************************************
** BEGIN_MANUAL_ENTRY( PSDisplayError (PS API), \
**                     api/utils/rprinter/psapi/psdispla )
**			A PSERVER client protocol routine used to create a terse
**			message string indicating a specific error condition
**
** SYNOPSIS
**			char *messagePtr;
**			SPXHandle_t spxHandle;
**
**			messagePtr = PSDisplayError( &spxHandle );
**
** INPUT
**			spxHandle			a pointer to a structure which
**								contains the two fields (errnoType
**								and errno) used to create the message
**
** RETURN VALUES
**			messagePtr			receives the address to the string
**								created
**
** DESCRIPTION
**			When a PS API routine returns -1, the error condition
**			is stored in the SPX handle. This routine is used to
**			make a terse message indicating the specific error
**			condition and its numeric value te be displayed.
**			Sample output follows in double quotes:
**				"errno = 71"		-	EPROTO
**				"t_errno = 10"		-	TBADDATA
**				"tlook = 32"		-	T_ERROR
**				"t_disconnect = 3"	-	CONNECTION_TERMINATED
**				"ps_error = 0x308"	-	PSE_ALREADY_IN_USE
**
**			Significant global routines called:
**				SPXDisplayErrno
**
** END_MANUAL_ENTRY ( 11/10/90 )
**/

char *
PSDisplayError(
	SPXHandle_t *spxHandle)
{
	static char errMsg[MAX_ERROR_MESSAGE_LENGTH];

	if (spxHandle->errnoType == SPXET_USER) {
		sprintf( errMsg, "ps_error = 0x%04X", spxHandle->errno );
		return errMsg;
	} else {
		return SPXDisplayErrno( spxHandle );
	}
}
