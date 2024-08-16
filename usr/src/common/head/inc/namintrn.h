/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/namintrn.h	1.5"
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

#ifndef  _NAMINTRN_HEADER_
#define  _NAMINTRN_HEADER_

#include <npackon.h>

#define  ABBREV_TABLE_SIZE          7
#define  ISO_3611_COUNTRY_NAME_LEN  2
#define  NO_MAP_CHAR                0x0000

typedef  enum
{
   ORGANIZATION_NAME_NODE = 1,
   ORGANIZATION_UNIT_NODE
}  Node_Type_T;

#define  CANON_TOKEN          '.'

typedef struct _rdn_ptr
{
   unicode N_FAR  *name;
   unsigned       nameLen;
   unicode N_FAR  *value;
   unsigned       valueLen;
   struct _rdn_ptr   N_FAR *next;
   struct _rdn_ptr   N_FAR *multiAVA;
}  RDN_Ptr_T;

typedef  struct   _rdn_list
{
   int               numNodes;
   RDN_Ptr_T   N_FAR *list;
}  RDN_List_T;


#define MAXTOKENLEN        40
#define MAXTOKENPUSHBACK   10

typedef enum
{
   TT_BAD         = -1,
   TT_NULL        = 0,
   TT_DELIM_RDN   = DELIM_RDN,
   TT_DELIM_DV    = DELIM_DV,
   TT_DELIM_VALUE = DELIM_VALUE,
   TT_IDENT       = 'a'
} NDSTokenType;

typedef struct
{
   NDSTokenType   type;
   unicode N_FAR  *start;
} TokenStackNode;

/* NWClient DS prototypes */
N_EXTERN_LIBRARY( void )
NWCPushToken
(
   NDSTokenType   t,
   unicode N_FAR  *start
);

N_EXTERN_LIBRARY( void )
NWCInitTokens
(
   void
);

N_EXTERN_LIBRARY( NDSTokenType )
NWCGetToken
(
   unicode N_FAR * N_FAR   *input,
   unicode N_FAR * N_FAR   *start,
   int     N_FAR           *len
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWCAbbreviateUnicodeName
(
   NWDSContextHandle context,
   unicode N_FAR     *objectName,
   unicode N_FAR     *abbrevObjectName
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWCRemoveTypesWithoutAbbrev
(
   NWDSContextHandle    context,
   unicode  N_FAR       *objectDN,
   unicode  N_FAR       *typelessName
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWCCanonicalizeUnicodeName
(
   NWDSContextHandle context,
   unicode N_FAR     *name,
   unicode N_FAR     *outName
);

int _NWCSameCIString
(
   unsigned       charsInA,
   unicode N_FAR  *nameA,
   unsigned       charsInB,
   unicode N_FAR  *nameB
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NWCNDSCreateRDNList
(
   unicode     N_FAR *objectName,
   RDN_List_T  N_FAR *RDNList
);

N_EXTERN_LIBRARY( void )
_NWCNDSFreeRDNList
(
   RDN_List_T N_FAR *RDNList
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NWCNDSApplyDefaultRule
(
   int         N_FAR          *numNodes,
   RDN_Ptr_T   N_FAR * N_FAR  *nodePtr,
   int                        limit
);

N_EXTERN_LIBRARY( NWDSCCODE )
AbbreviateUnicodeName
(
   NWDSContextHandle context,
   unicode N_FAR     *objectName,
   unicode N_FAR     *abbrevObjectName
);

N_EXTERN_LIBRARY( NWDSCCODE )
CanonicalizeUnicodeName
(
   NWDSContextHandle context,
   unicode N_FAR     *name,
   unicode N_FAR     *outName
);

N_EXTERN_LIBRARY( void )
_NDSFreeRDNList
(
   RDN_List_T N_FAR *RDNList
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSApplyDefaultRule
(
   int         N_FAR          *numNodes,
   RDN_Ptr_T   N_FAR * N_FAR  *nodePtr,
   int                        limit
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSApplyDefaultNameContextRule
(
   int         N_FAR          *numNodes,
   RDN_Ptr_T   N_FAR * N_FAR  *nodePtr,
   int                        limit
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSCreateRDNList
(
   unicode     N_FAR *objectName,
   RDN_List_T  N_FAR *RDNList
);

N_EXTERN_LIBRARY( void )
_NDSRemoveWhiteSpace
(
   unicode N_FAR *inStr,
   unicode N_FAR *outStr
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSRemWhiteSpaceFromName
(
   unicode N_FAR *inStr,
   unicode N_FAR *outStr
);

N_EXTERN_LIBRARY( void )
_NDSRemoveNodeUsingDefaultRule
(
   RDN_Ptr_T   N_FAR *nodePtr,
   int               numNodes
);

N_EXTERN_LIBRARY( NWDSCCODE )
RemoveTypesWithoutAbbrev
(
   NWDSContextHandle    context,
   unicode  N_FAR       *objectDN,
   unicode  N_FAR       *typelessName
);

int _SameCIString
(
   unsigned       charsInA,
   unicode N_FAR  *nameA,
   unsigned       charsInB,
   unicode N_FAR  *nameB
);


N_EXTERN_LIBRARY( void )
InitTokens
(
   void
);

N_EXTERN_LIBRARY( NDSTokenType )
GetToken
(
   unicode N_FAR * N_FAR   *input,
   unicode N_FAR * N_FAR   *start,
   int     N_FAR           *len
);

N_EXTERN_LIBRARY( void )
PushToken
(
   NDSTokenType   t,
   unicode N_FAR  *start
);

unicode N_FAR * NWPASCAL GetTokenType
(
   NDSTokenType t
);

#include <npackoff.h>

#endif                           /* #ifndef _NAMINTRN_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/namintrn.h,v 1.7 1994/09/26 17:09:23 rebekah Exp $
*/
