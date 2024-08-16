/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:login.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"
#include "encrypt.h"

#include "nwcaldef.h"
#include "nwcint.h"
/*#include "nwintern.h"*/   /* include for prototypes; include after nwcint.h */
#include "nwbindry.h"
#include "nwerror.h"
#include "nwmisc.h"
#define  NWL_EXCLUDE_TIME
#define  NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwconnec.h"
#include "nwserver.h"
#include "nwundoc.h" 

#if defined( N_PLAT_NLM )
#define _PROLOG_H
#include <nwconn.h>
#endif

/*manpage*NWLoginToFileServer************************************************
 SYNTAX:  NWLoginToFileServer(
            NWCONN_HANDLE conn,
            pnstr8 objName,
            nuint16 objType,
            pnstr8 password)

 REMARKS: Attempts to log into specified server

 INCLUDE: nwserver.h
****************************************************************************/
NWCCODE N_API NWLoginToFileServer
(
  NWCONN_HANDLE   conn,
  pnstr8          objName,
  nuint16         objType,
  pnstr8          password
)
{
#if defined N_PLAT_MSW || \
    defined(N_PLAT_DOS)
	NWCAuthenInfo authenInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   authenInfo.BinderyInfo.luObjectType = NSwap16(objType);

   return ((NWCCODE)NWCAuthenticate(&access, NWC_AUTH_STATE_BINDERY, objName, password,
			 &authenInfo));

}
/* note that the close brace is inside the else. This is because the rest
   of the file until the packet encrypting stuff is non-NT */

#elif defined(N_PLAT_UNIX)
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return(NWCAuthenticateBind(&access, objName, NSwap16(objType),
       password));
}

#elif defined(N_PLAT_NLM)
   NWCAuthenInfo    authenInfo;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);
   authenInfo.BinderyInfo.luObjectType = objType;

   return(NWCAuthenticate(&access, NWC_AUTHENT_BIND,
                          objName, password, &authenInfo));
}
#else

  if(!password)
    return NO_SUCH_OBJECT_OR_BAD_PASSWORD;

}

#endif   /* defined N_PLAT_WNT && defined N_ARCH_32 */

NWCCODE N_API NWVerifyObjectPassword
(
  NWCONN_HANDLE   conn,
  pnstr8          objName,
  nuint16         objType,
  pnstr8          password
)
{
  pnuint8 tempPtr;
  nuint8  cryptPass[16],key[8];
  nuint32 objectID;
  NWCCODE ccode;
  NWCDeclareAccess(access);

  NWCSetConn(access, conn);

  NWGetObjectID(conn, objName, objType, &objectID);

  if((ccode = GetLoginKey(conn, key)) != 0)
    return(ccode);

   for(tempPtr = (pnuint8)objName; *tempPtr; tempPtr = NWNextChar(tempPtr))
   {
      if((*tempPtr == '*') || (*tempPtr == '?'))
         return 0x89F0;
   }

  objectID = NSwapLoHi32(objectID);
  EncryptPassword(objectID, password, (nuint16)NWCStrLen(password), cryptPass);
  GetPasswordKey(key, cryptPass, key);
  return( (NWCCODE) NWNCP23s74KeyedVerifyPwd( &access, key, NSwap16(objType),
                     (nuint8) NWCStrLen( objName ),
                     objName) );
}

NWCCODE N_API NWChangeObjectPassword
(
  NWCONN_HANDLE   conn,
  pnstr8          objName,
  nuint16         objType,
  pnstr8          oldPassword,
  pnstr8          newPassword
)
{
   nuint32 objectID;
   nuint8 oldCryptPass[128], newCryptPass[128], key[8], passLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(GetLoginKey(conn, key))
      return 0x89ff;

   NWGetObjectID(conn, objName, objType, &objectID);

   /* Have to swap because NWGetObjectID returns it swaped */
   objectID = NSwapLoHi32(objectID);

   EncryptPassword(objectID,
                   oldPassword,
                   (nuint16)NWCStrLen(oldPassword),
                   oldCryptPass);

   EncryptPassword(objectID,
                   newPassword,
                   (nuint16)NWCStrLen(newPassword),
                   newCryptPass);

   GetPasswordKey(key, oldCryptPass, key);

   Encode(oldCryptPass, newCryptPass, newCryptPass);
   Encode(oldCryptPass + 8, newCryptPass + 8, newCryptPass + 8);

   /*
      The password length for encrypted passwords is kind of meaningless
      right now.  The upper two bits are always set '01' so as to force
      the password length to be between 64 and 127.  The lower six bits
      are encrypted. This is so the 2.15 servers will be able to use the
      encrypted passwords. This will probably be fixed sometime in the
      future. If the initial password length is for some reason exactly 64
      or 128, it is set to be 63 so when the top two bits are lost, it
      doesn't look like the length is 0. ml 2/27/89
   */

   passLen = (nuint8)NWCStrLen(newPassword);
   if(passLen > 63)
      passLen= 63;

   passLen  = passLen ^ oldCryptPass[0] ^ oldCryptPass[1];
   passLen &= 0x7F;
   passLen |= 0x40;

   return( (NWCCODE) NWNCP23s75KeyedChangePwd( &access, key, NSwap16(objType),
                     (nuint8) NWCStrLen( objName ),
                     objName,
                     passLen, newCryptPass));

}

/****************************************************************************

      This function disallows use of the specified password by the
      specified object.

      This routine adds a encrypted password to the list of old
      passwords maintained in the OLD_PasswordS property. If the
      OLD_PasswordS property does not exist, then this routine will
      check bit 0x2 of the restriction   flags in the LOGIN_CONTROL
      property. If this unique passwords bit is set then the
      OLD_PasswordS property will be created, otherwise a
      BINDERY_FAILURE error code will be returned.The object name and
      type must be specific, no wild cards are allowed.

      This function encrypts the password before storing it in the old
      passwords property.

      BINDERY_FAILURE (0xFF)
        - This error is returned if the login control property for the
          object does not require unique passwords
****************************************************************************/
NWCCODE N_API NWDisallowObjectPassword
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         disallowedPassword
)
{
   #define PROPERTY_SIZE      128     /* bindery object property size*/
   #define PASSWORD_SIZE      16      /* size of password in property */
   #define OLDPASS_READWRITE  0x32    /* object read,supervisor write */
   #define RESTRICTION_FLAGS  62      /* restriction flags offset */
   #define UNIQUE_PASSWORDS   0x02    /* unique passwords bit flag */
   #define BINDERY_FAILURE    0xFF

   NWCCODE ccode;
   nuint32 luObjID;
   nuint8 abuData[PROPERTY_SIZE];
   nuint8 abuLoginData[PROPERTY_SIZE];
   nuint8 abuCryptPwd[PASSWORD_SIZE];

   /* see if the OLD_PasswordS property exists, if so read it */
   ccode = NWReadPropertyValue(conn, objName, objType, (pnstr8)"OLD_PASSWORDS",
                                 1, abuData, 0, BF_STATIC);

   /* continue if the property was read or if we can create if*/
   if((ccode == 0) || (ccode == NO_SUCH_PROPERTY))
   {
      /* OLD_PASSWORDS property don't exist create it */
      if(ccode == NO_SUCH_PROPERTY)
      {
         NWCMemSet(abuData, 0, PROPERTY_SIZE);
         NWReadPropertyValue(conn, objName, objType, (pnstr8)"LOGIN_CONTROL",
                           1, abuLoginData, 0, BF_STATIC);

         /* check if this object uses unique passwords */
         if(abuLoginData[RESTRICTION_FLAGS] & UNIQUE_PASSWORDS)
         ccode = NWCreateProperty(conn, objName, objType, (pnstr8)"OLD_PASSWORDS",
                                    BF_STATIC | BF_ITEM, OLDPASS_READWRITE);
         else
         return BINDERY_FAILURE;
      }
      else
      {
         /* adjust the segment to include the new encrypted password */
         NWCMemMove(&abuData[PASSWORD_SIZE], abuData, PROPERTY_SIZE - PASSWORD_SIZE);
      }

      /* encrpt password so that it can be stored in correct form */
      NWGetObjectID(conn, objName, objType, &luObjID);
      luObjID = NSwapLoHi32( luObjID );
      EncryptPassword(luObjID, disallowedPassword,
                     (nuint16) NWCStrLen(disallowedPassword),
                     abuCryptPwd);

         /* move the new password into the first space of the property */
      NWCMemMove(abuData, abuCryptPwd, PASSWORD_SIZE);

      /* write out adjusted segment to the bindery */
      ccode = NWWritePropertyValue(conn, objName, objType, (pnstr8)"OLD_PASSWORDS",
                                    1, abuData, 0);
   }

   return (ccode);
}



#if !defined(N_PLAT_NLM)  /* NLM does not use functions below */
/***************************************************************************
          _NWCreateSessionKey(NWCONN_HANDLE conn, pnuint8 sessionKey)
***************************************************************************/
NWCCODE N_API _NWCreateSessionKey(NWCONN_HANDLE conn, pnuint8 sessionKey)
{
#if defined(N_PLAT_UNIX) || \
    defined N_PLAT_WNT 
   /* Clear up a compiler warning */
   conn = conn;
   sessionKey = sessionKey;

/*
   return(NWCCreateSessionKey(conn, sessionKey));
*/
   /* !!! REMOVE WHEN RETURN ABOVE IS COMMENTED IN.  */
   /* Clear up a compiler warning */
   return( 0 );

#elif defined(N_PLAT_OS2)
struct
{
   nuint16 subFunction;
   nuint16 connectionID;
   pnuint8 sessionKeyData;
} requestPacket;

   requestPacket.subFunction = 2;
   requestPacket.connectionID = conn;
   requestPacket.sessionKeyData = sessionKey;

   return (NWCCallGate(_NWC_CREATEKEY_CODE, &requestPacket));

#else

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCCreateSessionKey(&access, sessionKey));

#endif
}

/***************************************************************************
          _NWGetSessionKey(NWCONN_HANDLE conn, pnuint8 sessionKey)
***************************************************************************/
NWCCODE N_API _NWGetSessionKey
(
   NWCONN_HANDLE  conn,
   pnuint8        sessionKey
)
{
#if defined(N_PLAT_UNIX) || \
    defined N_PLAT_WNT 
   /* Clear up a compiler warning */
   conn = conn;
   sessionKey = sessionKey;

/*
   return(NWCGetSessionKey(conn, sessionKey));
*/
   /* !!! REMOVE WHEN RETURN ABOVE IS COMMENTED IN.  */
   /* Clear up a compiler warning */
   return( 0 );

#elif defined(N_PLAT_OS2)
struct
{
   nuint16 subFunction;
   nuint16 conn;
   pnuint8 sessionKeyData;
} requestPacket;

   requestPacket.subFunction = 7;
   requestPacket.conn      = conn;
   requestPacket.sessionKeyData = sessionKey;

   return (NWCCallGate(_NWC_CREATEKEY_CODE, &requestPacket));
#else

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCGetSessionKey(&access, sessionKey));

#endif
}

/***************************************************************************
          _NWRenegotiateSecurityLevel(NWCONN_HANDLE conn)
***************************************************************************/
NWCCODE N_API _NWRenegotiateSecurityLevel
(
   NWCONN_HANDLE  conn
)
{
#if defined(N_PLAT_UNIX) || defined( N_PLAT_NLM ) || \
    defined N_PLAT_WNT 
   /* Clear up a compiler warning */
   conn = conn;

/*
   return(NWCRenegotiateSecurityLevel(conn));
*/
   /* !!! REMOVE WHEN RETURN ABOVE IS COMMENTED IN.  */
   /* Clear up a compiler warning */
   return( 0 );

#elif defined(N_PLAT_OS2)
struct
{
   nuint16 suSubFunction;
   nuint16 conn;
} retStr;

   retStr.suSubFunction = 6;
   retStr.conn = conn;

   return (NWCCallGate(_NWC_CREATEKEY_CODE, &retStr));
#else
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCRenegotiateSecurityLevel(&access));


#endif
}

/***************************************************************************
          _NWSetSecurityFlags(nuint16 conn, nuint32 luFlags)
***************************************************************************/
NWCCODE N_API _NWSetSecurityFlags
(
   NWCONN_HANDLE  conn,
   nuint32        flags
)
{
#if defined(N_PLAT_UNIX) || defined( N_PLAT_NLM ) || \
    defined N_PLAT_WNT 
   /* Clear up a compiler warning */
   conn = conn;
   flags = flags;

/*
   return(NWCSetSecurityFlags(conn, flags));
*/
   /* !!! REMOVE WHEN RETURN ABOVE IS COMMENTED IN.  */
   /* Clear up a compiler warning */
   return( 0 );

#elif defined(N_PLAT_OS2)
struct
{
   nuint16 suSubFunction;
   nuint16 conn;
   nuint32 flags;
} retStr;

   retStr.suSubFunction = 4;
   retStr.conn = conn;
   retStr.flags = flags;

   return (NWCCallGate(_NWC_CREATEKEY_CODE, &retStr));
#else
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCSetSecurityFlags(&access, flags));

#endif
}

/***************************************************************************
          _NWGetSecurityFlags(nuint16 conn, pnuint32 pflags)
***************************************************************************/
NWCCODE N_API _NWGetSecurityFlags
(
   NWCONN_HANDLE  conn,
   pnuint32       flags
)
{
#if defined(N_PLAT_UNIX) || \
    defined( N_PLAT_NLM ) || \
    defined N_PLAT_WNT 
   /* Clear up a compiler warning */
   conn = conn;
   flags = flags;

/*
   return(NWCGetSecurityFlags(conn, flags));
*/
   /* !!! REMOVE WHEN RETURN ABOVE IS COMMENTED IN.  */
   /* Clear up a compiler warning */
   return( 0 );

#elif defined(N_PLAT_OS2)
struct
{
   nuint16  suSubFunction;
   nuint16  conn;
   pnuint32 flags;
} retStr;

   retStr.suSubFunction = 1;
   retStr.conn = conn;
   retStr.flags = flags;

   return (NWCCallGate(_NWC_CREATEKEY_CODE, &retStr));

#else
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWCGetSecurityFlags(&access, flags));
#endif
}
#endif   /*  Everything except N_PLAT_NLM  */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/login.c,v 1.7 1994/09/26 17:47:51 rebekah Exp $
*/


