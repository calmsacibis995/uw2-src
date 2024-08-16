/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwintcon.h	1.2"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
/****************************************************************************
 ****************************************************************************
 BEGIN_MANUAL_ENTRY ( nwintcon.h() )

 SOURCE MODULE: nwintcon.h

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
#ifndef NWINTCON_INC
#define NWINTCON_INC

#define CONVERT_TO_LICENSED      0x4
#define CONVERT_TO_NON_LICENSED  0x8
#define CONVERT_TO_BINDERY       0X10
#define CONVERT_TO_DS            0x20

/* Resource definition bits for NWDSChangeResourceOnConnection API */
#define INCREMENT_GLOBAL_HARD_RESOURCE    0x0
#define DECREMENT_GLOBAL_HARD_RESOURCE    0x1
#define INCREMENT_TASK_HARD_RESOURCE      0x2
#define DECREMENT_TASK_HARD_RESOURCE      0x3
#define INCREMENT_GLOBAL_SOFT_RESOURCE    0x4
#define DECREMENT_GLOBAL_SOFT_RESOURCE    0x5
#define INCREMENT_TASK_SOFT_RESOURCE      0x6
#define DECREMENT_TASK_SOFT_RESOURCE      0x7
#define NWSESSION_KEY_SIZE                16
#define NWCHALLENGE_KEY_SIZE              8
#define NWSESSION_KEY_PRIMITIVE_SIZE      24


#ifdef __cplusplus
	extern "C" {
#endif

/* NWClient DS prototypes */
/* This calls will not lock the connection slot */
N_EXTERN_LIBRARY( NWCCODE )
__NWCDSGetConnectionSlot
(
   BYTE         connType,
   BYTE         transType,
   DWORD        transLen,
   BYTE N_FAR   *transBuf,
   NWCONN_HANDLE N_FAR *conn
);

N_EXTERN_LIBRARY( NWCCODE )
_NWCGetConnectionIDFromAddress
(
   BYTE           transType,
   DWORD          transLen,
   BYTE N_FAR     *transBuf,
   NWCONN_HANDLE  N_FAR *conn
);

N_EXTERN_LIBRARY( NWCCODE )
NWCDSSetConnectionInfo
(
   NWCONN_HANDLE  conn,
   BYTE           connType
);

N_EXTERN_LIBRARY( NWCCODE )
NWCDSChangeResourceOnConnection
(
   NWCONN_HANDLE  conn,
   BYTE           resource
);

N_EXTERN_LIBRARY( NWCCODE )
NWDSSetConnectionInfo
(
   NWCONN_HANDLE  conn,
   BYTE           connType
);

N_EXTERN_LIBRARY( NWCCODE )
_NWFreeConnectionSlot
(
   NWCONN_HANDLE  conn,
   BYTE           disconnectType
);

/* This calls will not lock the connection slot */
N_EXTERN_LIBRARY( NWCCODE )
_NWDSGetConnectionSlot
(
   BYTE         connType,
   BYTE         transType,
   DWORD        transLen,
   BYTE N_FAR   *transBuf,
   NWCONN_HANDLE N_FAR *conn
);

N_EXTERN_LIBRARY( NWCCODE )
NWDSChangeResourceOnConnection
(
   NWCONN_HANDLE  conn,
   BYTE           resource
);

N_EXTERN_LIBRARY( NWCCODE )
NWConvertConnection
(
   NWCONN_HANDLE  conn1,
   BYTE N_FAR     *flag,
   NWCONN_HANDLE  N_FAR *conn2
);

N_EXTERN_LIBRARY( NWCCODE )
_NWGetConnectionIDFromAddress
(
   BYTE           transType,
   DWORD          transLen,
   BYTE N_FAR     *transBuf,
   NWCONN_HANDLE  N_FAR *conn
);

N_EXTERN_LIBRARY( NWCCODE )
_NWGetConnectionIDFromName
(
   DWORD          nameLen,
   BYTE N_FAR     *name,
   NWCONN_HANDLE  N_FAR *conn
);

#ifdef __cplusplus
	}
#endif

#endif
