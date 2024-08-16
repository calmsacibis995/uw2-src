/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsattr.h	1.4"
#ifndef  _NWDSATTR_HEADER_
#define  _NWDSATTR_HEADER_

/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef __NWDSTYPE_H
#ifdef N_PLAT_UNIX
#include <nw/nwdstype.h>
#else
#include <nwdstype.h>
#endif
#endif

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>
#else
#include <nwdsdc.h>
#endif
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

#define  TIME_BITMAP_LEN               42
#define  NUM_POSTAL_ADDRESS_ELEMENTS   6

typedef  pnstr8   CE_String_T;
typedef  pnstr8   Class_Name_T;
typedef  pnstr8   CI_String_T;
typedef  pnstr8   CN_String_T;
typedef  pnstr8   DN_T;
typedef  nint32   Integer_T;
typedef  nuint8   Boolean_T;
typedef  pnstr8   NU_String_T;
typedef  pnstr8   Postal_Address_T[NUM_POSTAL_ADDRESS_ELEMENTS];
typedef  pnstr8   PR_String_T;
typedef  pnstr8   Secure_Name_T;
typedef  pnstr8   TN_String_T;
typedef  nuint32  Counter_T;

typedef struct
{
   nuint32  remoteID;
   pnstr8   objectName;
} Back_Link_T, N_FAR *pBack_Link_T;

typedef struct
{
   nuint32  numOfBits;
   nptr     data;       /* previously defined as "uint8 NWFAR *" */
} Bit_String_T, N_FAR *pBit_String_T;

typedef  struct _ci_list
{
   struct _ci_list   N_FAR *next;
   pnstr8                  s;
} CI_List_T, N_FAR *pCI_List_T;

typedef  struct
{
   pnstr8         telephoneNumber;
   Bit_String_T   parameters;
}Fax_Number_T, N_FAR *pFax_Number_T;

typedef struct
{
   pnstr8   objectName;
   nuint32  level;
   nuint32  interval;
} Typed_Name_T, N_FAR *pTyped_Name_T;

typedef struct
{
   nuint32  addressType;
   nuint32  addressLength;
   nptr     address;
} Net_Address_T, N_FAR *pNet_Address_T;

typedef  struct
{
   pnstr8   protectedAttrName;
   pnstr8   subjectName;
   nuint32  privileges;
} Object_ACL_T, N_FAR *pObject_ACL_T;

typedef  struct
{
   nuint32  length;
   nptr     data;
} Octet_String_T, N_FAR *pOctet_String_T;

typedef Octet_String_T  Stream_T;
typedef pOctet_String_T pStream_T;

typedef  struct _octet_list
{
   struct _octet_list   N_FAR *next;
   nuint32                    length;
   nptr                       data;
} Octet_List_T, N_FAR *pOctet_List_T;

typedef struct
{
   pnstr8   objectName;
   nuint32  amount;
} Hold_T, N_FAR *pHold_T;

typedef struct
{
   pnstr8         serverName;
   nint32         replicaType;
   nint32         replicaNumber;
   nuint32        count;
   Net_Address_T  replicaAddressHint[1];
} Replica_Pointer_T, N_FAR *pReplica_Pointer_T;

typedef struct
{
   nuint32  type;
   nptr     address;
} EMail_Address_T, N_FAR *pEMail_Address_T;

typedef struct
{
   nuint32  nameSpaceType;
   pnstr8   volumeName;
   pnstr8   path;
} Path_T, N_FAR *pPath_T;

typedef struct
{
   nuint32 wholeSeconds;
   nuint32 eventID;
} NWDS_TimeStamp_T, N_FAR *pNWDS_TimeStamp_T;

typedef struct
{
   nuint32  wholeSeconds;
   nuint16  replicaNum;
   nuint16  eventID;
} TimeStamp_T, N_FAR *pTimeStamp_T;

typedef struct
{
   pnstr8   attrName;
   nuint32  syntaxID;
   nuint32  valueLen;
   nptr     value;
} Unknown_Attr_T, N_FAR *pUnknown_Attr_T;

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif                           /* #ifndef _NWDSATTR_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsattr.h,v 1.5 1994/06/08 23:32:42 rebekah Exp $
*/
