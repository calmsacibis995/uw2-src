/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpea.h	1.5"
#if !defined(NCPEA_H)
#define NCPEA_H

#if !defined( NTYPES_H )
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#if !defined( NWACCESS_H )
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#define NWEA_CLOSE_IMMEDIATE 0x80
#define NWEA_INFO_LEVEL_0    0x00
#define NWEA_INFO_LEVEL_1    0x10
#define NWEA_INFO_LEVEL_2    0x20
#define NWEA_INFO_LEVEL_3    0x30
#define NWEA_INFO_LEVEL_4    0x40
#define NWEA_INFO_LEVEL_5    0x50
#define NWEA_INFO_LEVEL_6    0x60
#define NWEA_INFO_LEVEL_7    0x70
#define NWEA_CLOSE_ON_ERROR  0x04
#define NWEA_USE_VOL_ENTRY   0x00
#define NWEA_USE_NWHANDLE    0x01
#define NWEA_USE_EAHANDLE    0x02

typedef union tagNWNCPEAHandle
{
   struct
   {
      nuint32  luVolNum;
      nuint32  luDirBase;
   } TYPE_00;

   struct
   {
      nuint8   abuNWHandleB4[4];
      nuint8   abuReservedB4[4];
   } TYPE_01;

   struct
   {
      nuint32  luEAHandle;
      nuint32  luReserved;
   } TYPE_10;

} NWNCPEAHandle, N_FAR *pNWNCPEAHandle;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP86s1EACloseHandle
(
   pNWAccess pAccess,
   nuint16  suReserved,
   nuint32  luEAHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP86s5EADuplicate
(
   pNWAccess       pAccess,
   nuint16        suSrcFlags,
   nuint16        suDstFlags,
   pNWNCPEAHandle pSrcEAHandle,
   pNWNCPEAHandle pDstEAHandle,
   pnuint32       pluCount,
   pnuint32       pluDataSize,
   pnuint32       pluKeySize
);

typedef struct tagNWNCPEALevel1
{
   nuint32  luValueLen;
   nuint16  suKeyLen;
   nuint32  luAccessFlag;
   nuint8   abuKey[1];
} NWNCPEALevel1, N_FAR *pNWNCPEALevel1;

typedef struct tagNWNCPEALevel6
{
   nuint32  luValueLen;
   nuint16  suKeyLen;
   nuint32  luAccessFlag;
   nuint32  luKeyExtants;
   nuint32  luValueExtants;
   nuint8   abuKey[1];
} NWNCPEALevel6, N_FAR *pNWNCPEALevel6;

typedef struct tagNWNCPEALevel7
{
   nuint8   buKeyLen;
   nuint8   abuKey[1];
} NWNCPEALevel7, N_FAR *pNWNCPEALevel7;

typedef union tagNW_EA_ENUM
{
   struct
   {
      nuint16  suReserved1;
      nuint16  suReserved2;
   } lvl_0_6;

   struct
   {
      nuint16  suSequence;
      nuint16  suEnumEAStructCount;
   } lvl_1_7;
} NWNCPEAEnum, N_FAR *pNWNCPEAEnum;

N_EXTERN_LIBRARY( NWRCODE )
NWNCP86s4EAEnum
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luInspectSize,
   nuint16        suSequence,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint32       pluErrorCode,
   pnuint32       pluTtlEAs,
   pnuint32       pluTtlEAsDataSize,
   pnuint32       pluTtlEAsKeySize,
   pnuint32       pluNewEAHandle,
   pNWNCPEAEnum   pEAEnum,
   pnuint8        pbuEnumData
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP86s3EARead
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luReadPosition,
   nuint32        luInspectSize,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint32       pluErrorCode,
   pnuint32       pluTtlValuesLen,
   pnuint32       pluNewEAHandle,
   pnuint32       pluAccessFlag,
   pnuint16       psuValueLen,
   pnuint8        pbuValue
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP86s2EAWrite
(
   pNWAccess       pAccess,
   nuint16        suFlags,
   pNWNCPEAHandle pEAHandle,
   nuint32        luTtlWriteDataSize,
   nuint32        luWritePosition,
   nuint32        luAccessFlagSize,
   nuint16        suValueLen,
   nuint16        suKeyLen,
   pnuint8        pbuKey,
   pnuint8        pbuValue,
   pnuint32       pluErrorCode,
   pnuint32       pluBytesWritten,
   pnuint32       pluNewEAHandle
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpea.h,v 1.8 1994/09/26 17:11:15 rebekah Exp $
*/
