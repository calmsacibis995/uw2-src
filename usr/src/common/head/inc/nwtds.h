/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwtds.h	1.3"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
/****************************************************************************
 ****************************************************************************
 BEGIN_MANUAL_ENTRY ( nwtds.h() )

 SOURCE MODULE: nwntds.h

 API NAME     :

 SYNTAX       :

 DESCRIPTION  :

 PARAMETERS   :   -> input          <-output

 INCLUDE      :

 RETURN       :

 MODIFICATIONS:

     January 20, 1992 - Art Nevarez, rolled-in from the OS/2 team.

 NCP CALLS
 ---------

 API CALLS
 ---------

 SEE ALSO:

 @Q1 HANDLES STRINGS? (Y\N):

 @Q2 HANDLES PATH? (Y\N):

 END_MANUAL_ENTRY
****************************************************************************/
#ifndef NWTDS_INC
#define NWTDS_INC

#define TDS_RESOURCE_TAG      272
#define TDS_ALREADY_ALLOCATED 0x8842

/* NWClient DS prototypes */
NWCCODE N_API NWCDSSetNDSCEIInfo
(
   NWCONN_HANDLE  conn,
   nuint32        entryID
);

NWCCODE N_API NWAllocTDSData
(
   nuint16  tag,
   nuint16  bytesRequested,
   nuint8   flags
);

NWCCODE N_API NWFreeTDSData
(
   nuint16  tag
);

NWCCODE N_API NWReadFromTDS
(
   nuint16  tag,
   nuint16  bytesRequested,
   nuint16  offset,  
   pnuint8  buffer,
   pnuint   bytesRead
);

NWCCODE N_API NWGetTDSControlInfo
(
   nuint16  tag,
   pnuint16 maxSize,
   pnuint16 dataSize,
   pnuint8  flags
);

NWCCODE N_API NWWriteToTDS
(
   nuint16  tag,
   nuint16  numBytes,
   nuint16  offset,
   pnuint8  buffer
);

NWCCODE N_API NWDSSetNDSCEIInfo
(
   NWCONN_HANDLE  conn,
   nuint32        entryID
);

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/nwtds.h,v 1.3 1994/06/08 23:35:48 rebekah Exp $
*/
