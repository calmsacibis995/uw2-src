/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/encrypt.h	1.3"
#ifndef ENCRYPT_H   /* encryption routines for login.c and nwauditc.c */
#define ENCRYPT_H

#ifndef NWCALDEF_H
#include "nwcaldef.h"
#endif

/* ************************************************************************ */
/*  defines that match defines in OS audit.h */

#define AUDIT_ENCRYPT_CONSTANT   12345678L
#define AUDIT_DUP_RCD_HDR_SIZE   32

/* ************************************************************************ */


N_EXTERN_LIBRARY( void )
NWCGetPasswordKey
(
  pnuint8 inputKey,
  pnuint8 cryptPass,
  pnuint8 outputKey
);

N_EXTERN_LIBRARY( void )
NWCEncryptPassword
(
  nuint32 ID,
  pnstr8 password,
  nuint16 length,
  pnuint8 output
);

N_EXTERN_LIBRARY( void )
NWCEncrypt
(
  pnuint8 input,
  pnuint8 output
);

N_EXTERN_LIBRARY( void )
NWCEncode
(
  pnuint8 key,
  pnuint8 input,
  pnuint8 output
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetLoginKey
(
	pNWAccess       pAccess,
	pnuint8 			pbuKeyB8
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetLoginPasswordKey
(
   pNWAccess       pAccess,
   pnuint8 			pbuCryptPwd,
   pnuint8 			pbuPwdKey
);

N_EXTERN_LIBRARY( void )
GetPasswordKey
(
  pnuint8 inputKey,
  pnuint8 cryptPass,
  pnuint8 outputKey
);

N_EXTERN_LIBRARY( void )
EncryptPassword
(
  nuint32 ID,
  pnstr8 password,
  nuint16 length,
  pnuint8 output
);

N_EXTERN_LIBRARY( void )
Encrypt
(
  pnuint8 input,
  pnuint8 output
);

N_EXTERN_LIBRARY( void )
Encode
(
  pnuint8 key,
  pnuint8 input,
  pnuint8 output
);

N_EXTERN_LIBRARY( NWCCODE )
GetLoginKey
(
   NWCONN_HANDLE  conn,
	pnuint8 			pbuKeyB8
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetLoginPasswordKey
(
   NWCONN_HANDLE  conn,
   pnuint8 			pbuCryptPwd,
   pnuint8 			pbuPwdKey
);

#endif
