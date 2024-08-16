/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:spil.c	1.3"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/spil.c,v 1.7 1994/09/26 17:19:02 rebekah Exp $";
#endif

/*
**	NetWare/SRC
**
**	SCCS:	@(#) spil.c 1.3@(#)	
**
**	Description:
**		A set of library calls to control NCP connection and 
**		communication functions.  This library communicates with a
**		set of standard netware api library calls as defined by NOVELL.
**
**		Copyright 1990 NOVELL, Inc.   All rights reserved.
*/



#include <nw/nwclient.h>
#include "nwapidef.h"
#include "nwxtypes.h"
#include <nw/nwcaldef.h>

#include "nwapi.h"
#include "nwerrors.h"

#include <sys/tiuser.h>
#include <sys/nwtdr.h>
#include "nucinit.h"
#include "nucnwapi.h"



/*********************************************************************
	NWNCPRequest: NCP Requestor
*********************************************************************/
/*
**	Used to make a request of a server on a connection.  
*/
N_EXTERN_FUNC_C (nuint32)
NWNCPRequest( 
	nuint32				conn,
	pNWU_FRAGMENT		reqFrag,
	pNWU_FRAGMENT		repFrag
)
{
	nuint32 	ccode;
	

/*
**	NOTE: The NUC requester does not peek inside a NCP reply
**		packet for a completion code, so we will do that here.
*/

	ccode = ncp_request(conn, 0,
							reqFrag->fragAddress, reqFrag->fragSize,
							repFrag->fragAddress, &repFrag->fragSize);
	if(!ccode)
	{
		if(repFrag->fragAddress[6])
		{
#ifdef REQ_DEBUG
			printf("error from server received: %x\n",
									repFrag->fragAddress[6]);
#endif
			ccode = (NWERR_SERVER | repFrag->fragAddress[6]);
		}
	}
								
	return( ccode );
}

