/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:crtefil.c	1.5"
#include "nwclient.h"
#include "ntypes.h"
#include "nwaccess.h"

#include "ncpfile.h"
#include "nwnamspc.h"
#include "nwfile.h"


/*
** BEGIN_MANUAL_ENTRY (NWCreateFile, api/file/createfi.le)
** END_MANUAL_ENTRY
*/

NWCCODE N_API NWCreateFile
(
	NWCONN_HANDLE	conn,
	NWDIR_HANDLE	dirHandle,
	pnstr8			fileName,
	nuint8			fileAttributes,
	NWFILE_HANDLE NWPTR pFileHandle,
	nflag32			createFlag
)
{
	NWCCODE			cCode;
	nuint8		abuNWHandle[6];
	NWNCPFileInfo	fileInfo;
	nuint8			nameLen;
	NWCDeclareAccess(access);

	NWCSetConn(access, conn);

	nameLen = (nuint8)NWCStrLen((char *)fileName);

/*
**	Switch on create option flag.
*/
	if(createFlag == NWOVERWRITE_FILE)
	{
		cCode = (NWCCODE)NWNCP67FileCreate(&access,
                                       (nuint8)dirHandle,
                                       fileAttributes,
                                       nameLen,
                                       fileName,
                                       abuNWHandle,
                                       &fileInfo);
	}
	else
	{
		cCode = (NWCCODE)NWNCP77FileCreate(&access,
                                       (nuint8)dirHandle,
                                       fileAttributes,
                                       nameLen,
                                       fileName,
                                       abuNWHandle,
                                       &fileInfo);
	}

   if (cCode == 0)
   {
      /* 0x1F access means read/write/deny read/deny write/exclusive. */
      cCode = NWConvertHandle(conn, 0x1F, abuNWHandle, 6, 0, pFileHandle);
   }

	return ( cCode );
}


