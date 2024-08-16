/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:conneng.c	1.8"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/conneng.c,v 1.12.4.2 1995/02/14 02:40:52 hashem Exp $";
#endif


/***********************************************************************
 *
 * Filename:	  conneng.c
 *
 * Date Created:  January 5, l990
 *
 * Version:	  1.0
 *
 * Programmers:	  Mark Worwetz
 *
 * Comments:	  Modified for NWU 4.0.	 This version incorporates the
 *				  three layer model of the cross platform API's
 *
 * COPYRIGHT (c) 1992 by Novell, Inc.  All Rights Reserved.
 *
 **********************************************************************/

#include <nw/nwclient.h>
#include <nw/nwcaldef.h>
#include "nwaccess.h"
#include <nw/nwerror.h>
#include "nwerrors.h"
#include "nwapidef.h"



/************************************************************************
 **	   FUNCTION:	_INTKeyedLogin
 **	DESCRIPTION:	This function issues a KeyedLogin NCP request
 *************************************************************************/
/* ARGSUSED */
N_EXTERN_FUNC_C (NWRCODE)
_INTKeyedLogin
(
	NWCONN_HANDLE	conn,
	pnuint8			pbuKeyB8,
	nuint16			suObjType,
	nuint8			buObjNameLen,
	pnstr8			pbstrObjName
)
{
	NWCDeclareAccess( access );

	NWCSetConn( access, conn );

	return( (NWCCODE) NWNCP23s24KeyedObjLogin( &access, pbuKeyB8,
			suObjType, buObjNameLen, pbstrObjName, NULL ));
}


/*************************************************************************
 **	   FUNCTION:	_NWLoginToFileServer
 **	DESCRIPTION:	This function handles the TDR issues for this API.
 *************************************************************************/
/* ARGSUSED */
N_EXTERN_FUNC_C (NWRCODE)
_NWLoginToFileServer(
	pNWAccess		pAccess,
	pnstr			objName,
	nuint16			objType,
	pnstr			password
)
{
	nuint32			ccode;
	NWCONN_HANDLE	conn;
	nuint8			keyBuf[8];
	nuint8			keyedPassword[8];
	nuint8			cryptPass[16];
	char			authenBuffer[24];
	nuint8			objNameLength;
	nuint32			objectID;

#ifdef REQ_DEBUG

	int		i;
	printf("_NWLoginToFileServer\n");
#endif

	conn = NWCGetConnP( pAccess );

	if( (ccode = make_signature_decision( conn )) == 
		( NWE_BIND_SECURITY_INVALID ) ) 
		return( ccode );

/*
**	Verify that connection is not already logged in.
*/
	if (GetLoginKey( conn, keyBuf ))
		return(NWE_REQUESTER_FAILURE);

#ifdef REQ_DEBUG
	printf("LoginKey = '");
	for(i=0; i<8; i++)
		printf("%x ", keyBuf[i]);
	printf("\n");
#endif
	
	objNameLength = (nuint8) NWCStrLen( (char *)objName );
	ccode = NWNCP23s53GetObjID( pAccess, objType, objNameLength, objName,
							&objectID, NULL, NULL );
	if (ccode != NWSUCCESSFUL )
		return( ccode );

	
/*
**	Ensure that the objectID is in HI_LO byte order
*/
	EncryptPassword( NSwapHiLo32( objectID ), password,
					NWCStrLen((char *)password), cryptPass);

	GetPasswordKey( keyBuf, cryptPass, keyedPassword );
	
	
#ifdef REQ_DEBUG
	printf("keyedPassword = '");
	for(i=0; i<8; i++)
		printf("%x ", keyedPassword[i]);
	printf("\n");
	
	printf("CryptPass = '");
	for(i=0; i<16; i++)
		printf("%x ", cryptPass[i]);
	printf("\n");
#endif
	
/*
**	We got what we needed, let's try the login.
*/
	ccode = _INTKeyedLogin( conn, keyedPassword, objType, 
							objNameLength, (pnstr8)objName );
	if( ccode != NWSUCCESSFUL )
	{
#ifdef REQ_DEBUG
		printf("INTKeyedLogin failure");
#endif
		return( ccode );
	}

/*
**	Make a SPIL request to finish initialization of the connection
**	connection.
*/
	NWCMemSet(authenBuffer, 0, sizeof(authenBuffer));
	NWCMemCpy(authenBuffer, (char *)cryptPass, 16);
	NWCMemCpy(&authenBuffer[16], (char *)keyBuf, 8);
	
	ccode = authenticate_task( conn, (pnuint8)authenBuffer,
							objectID, NWC_AUTH_STATE_BINDERY );

	if( ccode != NWSUCCESSFUL )
	{
#ifdef REQ_DEBUG
		printf("_PDAuthenticateConn/authenticate_task failure");
#endif
		return( ccode );
	}
	
	return( 0 );
}
