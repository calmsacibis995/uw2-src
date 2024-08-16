/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:requtils.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/requtils.c,v 1.6 1994/09/26 17:19:01 rebekah Exp $";
#endif

/***********************************************************************
 *
 * Filename:	  requtils.c
 *
 * Date Created:  19 Jan 90 
 *
 * Version:	  	  1.0
 *
 * Comments:
 *			Utility routines used by the NCP layer and other TDR
 *			routines.
 *
 * COPYRIGHT (c) 1987 by Novell, Inc.  All Rights Reserved.
 *
 **********************************************************************/

#include "nwaccess.h"
#include <sys/nwtdr.h>
#include "tdr.h"
#include "nwClnt.h"

N_INTERN_VAR nuint8 *requestBuffer;
N_INTERN_VAR nuint8 *replyBuffer;


/*
**	Returns addresses to buffers needed for packets that go
**	onto or come off of the wire.
*/
N_EXTERN_FUNC_C (nint)
NWGetTDRBuffers
(
	pnuint8 *requestBufferPtr,
	pnuint8 *replyBufferPtr
)
{

#ifdef _REENTRANT
	if( FIRST_OR_NO_THREAD ) {
		if( requestBuffer == NULL ) {
			requestBuffer = (uint8 *)calloc(MAXIMUM_PACKET_SIZE, sizeof(uint8));
			if( requestBuffer == NULL ) {
				replyBuffer = NULL;
				return(-1);
			}
		} 
		if( replyBuffer == NULL ) {
			replyBuffer = (uint8 *)calloc(MAXIMUM_PACKET_SIZE, sizeof(uint8) );
			if( replyBuffer == NULL ) {
				free( requestBuffer );
				requestBuffer = NULL;
				return(-1);
			}
		}
		*requestBufferPtr = requestBuffer;
		*replyBufferPtr = replyBuffer;
		
	} else {
		*requestBufferPtr = (uint8 *)_mt_get_thr_specific_storage(
			_nwClnt_req_buffer_key, MAXIMUM_PACKET_SIZE );
		if( *requestBufferPtr == NULL ) {
			return(-1);
		}
		*replyBufferPtr = (uint8 *)_mt_get_thr_specific_storage(
			_nwClnt_reply_buffer_key, MAXIMUM_PACKET_SIZE );
		if( *replyBufferPtr == NULL ) {
			free( *requestBufferPtr );
			*requestBufferPtr = NULL;
			return(-1);
		}
	}

#else /* ! _REENTRANT */

    if( requestBuffer == NULL ) {
        requestBuffer = (nuint8 *) calloc( MAXIMUM_PACKET_SIZE, sizeof(uint8) );
        if( requestBuffer == NULL ) {
            replyBuffer = NULL;
            return(-1);
        }
    }
    if( replyBuffer == NULL ) {
        replyBuffer = (nuint8 *) calloc( MAXIMUM_PACKET_SIZE, sizeof(uint8) );
        if( replyBuffer == NULL ) {
            free( requestBuffer );
            requestBuffer = NULL;
            return(-1);
        }
    }
	*requestBufferPtr = requestBuffer;
	*replyBufferPtr = replyBuffer;

#endif /* _REENTRANT */
	return(0);
}


/*
**	Extracts the standard NCP header information from a reply packet.
*/
N_EXTERN_FUNC_C (nuint8)
NWBuildRequestHeader
(
	pnuint8	requestPacket,
	nuint16	requestType,
	nuint8	functionCode
)
{
	pnuint8 packetPtr;

	packetPtr = requestPacket;

	HI_LO_UINT16_FROM( requestType );					/* 0 */

/*
**	NUC Requester passes a full packet header
*/
	packetPtr += 4;

	COPY_UINT8_FROM( functionCode );					/* 2 */

	return ( 0 );
}

