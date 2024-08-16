/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/requester.h	1.7"
#ident	"$Id: requester.h,v 2.51.2.1 1994/12/12 01:29:10 stevbam Exp $"

/*
 *  Netware Unix Client
 */

#ifndef NWCLIENT_H
#define NWCLIENT_H

#ifdef _KERNEL
#define N_FAR
#define SESSION_KEY_LENGTH          8

typedef struct {
	struct netbuf	*address;
	nwcred_t	*credential;
	uint32		connectionReference;
} comargs_t;

#endif /* _KERNEL */

#define NWCSINGLE_BYTE  1
#define NWCDOUBLE_BYTE  2

#define NWC_NUM_VECTORS 6

/* Maximum defines */
#define NWC_MAX_SERVER_NAME_LEN        48

/* Name Format Type (nuint value) */
#define NWC_NAME_FORMAT_NDS            0x0001
#define NWC_NAME_FORMAT_BIND           0x0002
#define NWC_NAME_FORMAT_BDP            0x0004
#define NWC_NAME_FORMAT_NDS_TREE       0x0008
#define NWC_NAME_FORMAT_WILD           0x8000

/* String types - (nuint value) */
#define NWC_STRING_TYPE_ASCII          0x0001
#define NWC_STRING_TYPE_UNICODE        0x0002

/* Transport Type - (nuint value) */
#define NWC_TRAN_TYPE_IPX              0x0001
#define NWC_TRAN_TYPE_UDP              0x0002
#define NWC_TRAN_TYPE_DDP              0x0003
#define NWC_TRAN_TYPE_ASP              0x0004
#define NWC_TRAN_TYPE_WILD             0x8000

/* Open connection flags - (nuint value) */
#define NWC_OPEN_LICENSED              0x0001
#define NWC_OPEN_UNLICENSED            0x0002
#define NWC_OPEN_PRIVATE               0x0004
#define NWC_OPEN_PUBLIC                0x0008

/* Connection Info Levels (nuint value) */
#define NWC_CONN_INFO_INFO_VERSION     0x0001
#define NWC_CONN_INFO_AUTH_STATE       0x0002
#define NWC_CONN_INFO_BCAST_STATE      0x0003
#define NWC_CONN_INFO_CONN_REF         0x0004
#define NWC_CONN_INFO_TREE_NAME        0x0005
#define NWC_CONN_INFO_WORKGROUP_ID     0x0006
#define NWC_CONN_INFO_SECURITY_STATE   0x0007
#define NWC_CONN_INFO_CONN_NUMBER      0x0008
#define NWC_CONN_INFO_USER_ID          0x0009
#define NWC_CONN_INFO_SERVER_NAME      0x000A
#define NWC_CONN_INFO_TRAN_ADDR        0x000B
#define NWC_CONN_INFO_NDS_STATE        0x000C
#define NWC_CONN_INFO_MAX_PACKET_SIZE  0x000D
#define NWC_CONN_INFO_LICENSE_STATE    0x000E
#define NWC_CONN_INFO_PUBLIC_STATE     0x000F
#define NWC_CONN_INFO_SERVICE_TYPE     0x0010
#define NWC_CONN_INFO_DISTANCE         0x0011
#define NWC_CONN_INFO_RETURN_ALL       0xFFFF

/* Information verions (nuint value) */
#define NWC_INFO_VERSION_1                     0x0001

/* Authentication states (nuint value) */
#define NWC_AUTH_STATE_NONE            0x0000
#define NWC_AUTH_STATE_BINDERY         0x0001
#define NWC_AUTH_STATE_NDS             0x0002
#define NWC_AUTH_STATE_PNW             0x0003

/* Broadcast states (nuint value) */
#define NWC_BCAST_PERMIT_ALL           0x0000
#define NWC_BCAST_PERMIT_SYSTEM        0x0001
#define NWC_BCAST_PERMIT_NONE          0x0002

/* Security states (uint32 value) */
#define NWC_SECURITY_SIGNING_NOT_IN_USE   0x00000000
#define NWC_SECURITY_SIGNING_IN_USE       0x00000001
#define NWC_SECURITY_LEVEL_CHECKSUM       0x00000100
#define NWC_SECURITY_LEVEL_SIGN_HEADERS   0x00000200
#define NWC_SECURITY_LEVEL_SIGN_ALL       0x00000400
#define NWC_SECURITY_LEVEL_ENCRYPT        0x00000800

/* NDS states (nuint value) */
#define NWC_NDS_NOT_CAPABLE            0x0000
#define NWC_NDS_CAPABLE                0x0001

/* License states (nuint value) */
#define NWC_NOT_LICENSED               0x0000
#define NWC_CONNECTION_LICENSED        0x0001
#define NWC_HANDLE_LICENSED            0x0002

/* Public states (nuint value) */
#define NWC_CONN_PUBLIC                0x0000
#define NWC_CONN_PRIVATE               0x0001

/* Scan connection information flags (nuint value) */
#define NWC_MATCH_NOT_EQUALS           0x0000
#define NWC_MATCH_EQUALS               0x0001
#define NWC_RETURN_PUBLIC              0x0002
#define NWC_RETURN_PRIVATE             0x0004
#define NWC_RETURN_LICENSED            0x0008
#define NWC_RETURN_UNLICENSED          0x0010

/* Authentication types */
#define NWC_AUTHENT_BIND               0x0001
#define NWC_AUTHENT_NDS                0x0002
#define NWC_AUTHENT_PNW                0x0003

/* Tagged data store flags */
#define NWC_TDS_PRE_ZERO               0x0001
#define NWC_TDS_POST_ZERO              0x0002
#define NWC_TDS_ENCRYPT                0x0004

typedef struct tagNWCTranAddr
{
   uint32    uType;
   uint32    uLen;
   uint8  *pbuBuffer;
} NWCTranAddr, N_FAR *pNWCTranAddr;

typedef struct tagNWCConnInfo
{
   uint32          uInfoVersion;
   uint32          uAuthenticationState;
   uint32          uBroadcastState;
   uint32        luConnectionReference;
   uint8          *pstrTreeName;
   uint8          *pstrWorkGroupId;
   uint32        luSecurityState;
   uint32          uConnectionNumber;
   uint32        luUserId;
   uint8          *pstrServerName;
   pNWCTranAddr   pTranAddr;
   uint32          uNdsState;
   uint32          uMaxPacketSize;
   uint32          uLicenseState;
   uint32          uPublicState;
   uint8          *pstrServiceType;
   uint32          uDistance;
} NWCConnInfo, N_FAR *pNWCConnInfo;

typedef struct tagNWCConnString
{
   uint8 *pString;
   uint32 uStringType;
   uint32 uNameFormatType;
} NWCConnString, N_FAR *pNWCConnString;

typedef struct tagNWCDBCSVector
{
  uint8 buLowVal;
  uint8 buHighVal;
} NWCDBCSVector;

extern NWCDBCSVector _NWCDBCSVector[NWC_NUM_VECTORS];

#endif /* NWCLIENT_H */
