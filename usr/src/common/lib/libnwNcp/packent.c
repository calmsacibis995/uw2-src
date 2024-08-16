/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:packent.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"
#include "nwnamspc.h"

/*manpage*NWNCPUnpackEntryUnion*********************************************
SYNTAX:  N_EXTERN_LIBRARY( void )
         NWNCPUnpackEntryUnion
         (
            pNWNCPEntryUnion  pEntryUnion,
            pnuint8           pbuDataB128,
            nint              iNcpNum
         )

REMARKS: Unpacks a byte stream to a NWNCPEntryUnion

ARGS: <  pEntryUnion
      >  pbuDataB128
      >  iNcpNum

INCLUDE: ncpfile.h

NCP:

CHANGES: 22 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWNCPUnpackEntryUnion
(
   pNWNCPEntryUnion  pEntryUnion,
   pnuint8           pbuDataB128,
   nint              iNcpNum
)
{
   NCopyLoHi32(&pEntryUnion->luSubdir,   &pbuDataB128[0]);
   NCopyLoHi32(&pEntryUnion->luAttrs,    &pbuDataB128[4]);
   pEntryUnion->buUniqueID =              pbuDataB128[8];
   pEntryUnion->buFlags    =              pbuDataB128[9];
   pEntryUnion->buNamSpc   =              pbuDataB128[10];
   pEntryUnion->buNameLen  =              pbuDataB128[11];

   if (pEntryUnion->buNamSpc == NW_NS_DOS)
   {
      switch( iNcpNum )
      {
         case 40:
         {
            NWCMemMove(pEntryUnion->info.entry.abuName,
               &pbuDataB128[12], (nuint) 12);
            NCopyLoHi32(&pEntryUnion->info.entry.luCreationDateAndTime,
               &pbuDataB128[24]);
            NCopyHiLo32(&pEntryUnion->info.entry.luOwnerID,
               &pbuDataB128[28]);
            NCopyLoHi32(&pEntryUnion->info.entry.luArchivedDateAndTime,
               &pbuDataB128[32]);
            NCopyHiLo32(&pEntryUnion->info.entry.luArchiverID,
               &pbuDataB128[36]);
            NCopyLoHi32(&pEntryUnion->info.entry.luModifiedDateAndTime,
               &pbuDataB128[40]);
            NCopyHiLo32(&pEntryUnion->info.entry.luModifierID,
               &pbuDataB128[44]);
            NCopyLoHi32(&pEntryUnion->info.entry.luDataForkSize,
               &pbuDataB128[48]);
            NCopyLoHi32(&pEntryUnion->info.entry.luDataForkFirstFAT,
               &pbuDataB128[52]);
            NCopyLoHi32(&pEntryUnion->info.entry.luNextTrusteeEntry,
               &pbuDataB128[56]);
            NWCMemMove(pEntryUnion->info.entry.abuReserved1,
               &pbuDataB128[60], (nuint) 36);
            NCopyLoHi16(&pEntryUnion->info.entry.suInheritedRightsMask,
               &pbuDataB128[96]);
            NCopyLoHi16(&pEntryUnion->info.entry.suLastAccessedDate,
               &pbuDataB128[98]);
            NCopyLoHi32(&pEntryUnion->info.entry.luDeletedFileTime,
               &pbuDataB128[100]);
            NCopyLoHi32(&pEntryUnion->info.entry.luDeletedDateAndTime,
               &pbuDataB128[104]);
            NCopyHiLo32(&pEntryUnion->info.entry.luDeletorID,
               &pbuDataB128[108]);
            NWCMemMove(pEntryUnion->info.entry.abuReserved2,
               &pbuDataB128[112], (nuint) 8);
            NCopyLoHi32(&pEntryUnion->info.entry.luPrimaryEntry,
               &pbuDataB128[120]);
            NCopyLoHi32(&pEntryUnion->info.entry.luNameList,
               &pbuDataB128[124]);
         }
         break;
         default:
         {
            if (pEntryUnion->luAttrs & FA_DIRECTORY)
            {
               NWCMemMove(pEntryUnion->info.dir.abuName,
                  &pbuDataB128[12], (nuint) 12);
               NCopyLoHi32(&pEntryUnion->info.dir.luCreationDateAndTime,
                  &pbuDataB128[24]);
               NCopyHiLo32(&pEntryUnion->info.dir.luOwnerID,
                  &pbuDataB128[28]);
               NCopyLoHi32(&pEntryUnion->info.dir.luArchivedDateAndTime,
                  &pbuDataB128[32]);
               NCopyHiLo32(&pEntryUnion->info.dir.luArchiverID,
                  &pbuDataB128[36]);
               NCopyLoHi32(&pEntryUnion->info.dir.luModifiedDateAndTime,
                  &pbuDataB128[40]);
               NCopyLoHi32(&pEntryUnion->info.dir.luNextTrusteeEntry,
                  &pbuDataB128[44]);
               NWCMemMove(pEntryUnion->info.dir.abuReserved1,
                  &pbuDataB128[48], (nuint) 48);
               NCopyLoHi32(&pEntryUnion->info.dir.luMaxSpace,
                  &pbuDataB128[96]);
               NCopyLoHi16(&pEntryUnion->info.dir.suInheritedRightsMask,
                  &pbuDataB128[100]);
               NWCMemMove(pEntryUnion->info.dir.abuReserved2,
                  &pbuDataB128[102], 14);
               NCopyHiLo32(&pEntryUnion->info.dir.luVolObjID,
                  &pbuDataB128[116]);
               NWCMemMove(pEntryUnion->info.dir.abuReserved3,
                  &pbuDataB128[120], 8);
            }
            else
            {
               NWCMemMove(pEntryUnion->info.file.abuName,
                  &pbuDataB128[12], (nuint) 12);
               NCopyLoHi32(&pEntryUnion->info.file.luCreationDateAndTime,
                  &pbuDataB128[24]);
               NCopyHiLo32(&pEntryUnion->info.file.luOwnerID,
                  &pbuDataB128[28]);
               NCopyLoHi32(&pEntryUnion->info.file.luArchivedDateAndTime,
                  &pbuDataB128[32]);
               NCopyHiLo32(&pEntryUnion->info.file.luArchiverID,
                  &pbuDataB128[36]);
               NCopyLoHi32(&pEntryUnion->info.file.luModifiedDateAndTime,
                  &pbuDataB128[40]);
               NCopyHiLo32(&pEntryUnion->info.file.luModifierID,
                  &pbuDataB128[44]);
               NCopyLoHi32(&pEntryUnion->info.file.luFileSize,
                  &pbuDataB128[48]);
               NWCMemMove(pEntryUnion->info.file.abuReserved1,
                  &pbuDataB128[52], (nuint) 44);
               NCopyLoHi16(&pEntryUnion->info.file.suInheritedRightsMask,
                  &pbuDataB128[96]);
               NCopyLoHi16(&pEntryUnion->info.file.suLastAccessedDate,
                  &pbuDataB128[98]);
               NWCMemMove(pEntryUnion->info.file.abuReserved2,
                  &pbuDataB128[100], (nuint) 28);
            }
         }
      }
   }
   else
   {
      NWCMemMove(pEntryUnion->info.mac.abuFileName,
         &pbuDataB128[12], (nuint) 32);
      NCopyLoHi32(&pEntryUnion->info.mac.luResourceFork,
         &pbuDataB128[44]);
      NCopyLoHi32(&pEntryUnion->info.mac.luResourceForkSize,
         &pbuDataB128[48]);
      NWCMemMove(pEntryUnion->info.mac.abuFinderInfo,
         &pbuDataB128[52], (nuint) 32);
      NWCMemMove(pEntryUnion->info.mac.abuProDosInfo,
         &pbuDataB128[84], (nuint) 6);
      NWCMemMove(pEntryUnion->info.mac.abuReserved,
         &pbuDataB128[90], (nuint) 38);
   }
   return;
}


/*manpage*NWNCPPackEntryUnion*********************************************
SYNTAX:  N_EXTERN_LIBRARY( void )
         NWNCPPackEntryUnion
         (
            pnuint8           pbuDataB128,
            pNWNCPEntryUnion  pEntryUnion,
            nint              iNcpNum
         )

REMARKS: This packs an NWNCPEntryUnion into a bytestream

ARGS: <  pbuDataB128
      >  pEntryUnion
      >  iNcpNum

INCLUDE: ncpfile.h

NCP:

CHANGES: 22 Sep 1993 - written - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWNCPPackEntryUnion
(
   pnuint8           pbuDataB128,
   pNWNCPEntryUnion  pEntryUnion,
   nint              iNcpNum
)
{
   NCopyLoHi32(&pbuDataB128[0],  &pEntryUnion->luSubdir);
   NCopyLoHi32(&pbuDataB128[4],  &pEntryUnion->luAttrs);
   pbuDataB128[8]  =              pEntryUnion->buUniqueID;
   pbuDataB128[9]  =              pEntryUnion->buFlags;
   pbuDataB128[10] =              pEntryUnion->buNamSpc;
   pbuDataB128[11] =              pEntryUnion->buNameLen;

   if (pEntryUnion->buNamSpc==NW_NS_DOS)
   {
      switch( iNcpNum )
      {
         case 40:
         {
            NWCMemMove(&pbuDataB128[12],
               pEntryUnion->info.entry.abuName, (nuint) 12);
            NCopyLoHi32(&pbuDataB128[24],
               &pEntryUnion->info.entry.luCreationDateAndTime);
            NCopyHiLo32(&pbuDataB128[28],
               &pEntryUnion->info.entry.luOwnerID);
            NCopyLoHi32(&pbuDataB128[32],
               &pEntryUnion->info.entry.luArchivedDateAndTime);
            NCopyHiLo32(&pbuDataB128[36],
               &pEntryUnion->info.entry.luArchiverID);
            NCopyLoHi32(&pbuDataB128[40],
               &pEntryUnion->info.entry.luModifiedDateAndTime);
            NCopyHiLo32(&pbuDataB128[44],
               &pEntryUnion->info.entry.luModifierID);
            NCopyLoHi32(&pbuDataB128[48],
               &pEntryUnion->info.entry.luDataForkSize);
            NCopyLoHi32(&pbuDataB128[52],
               &pEntryUnion->info.entry.luDataForkFirstFAT);
            NCopyLoHi32(&pbuDataB128[56],
               &pEntryUnion->info.entry.luNextTrusteeEntry);
            NWCMemMove(&pbuDataB128[60],
               pEntryUnion->info.entry.abuReserved1, (nuint) 36);
            NCopyLoHi16(&pbuDataB128[96],
               &pEntryUnion->info.entry.suInheritedRightsMask);
            NCopyLoHi16(&pbuDataB128[98],
               &pEntryUnion->info.entry.suLastAccessedDate);
            NCopyLoHi32(&pbuDataB128[100],
               &pEntryUnion->info.entry.luDeletedFileTime);
            NCopyLoHi32(&pbuDataB128[104],
               &pEntryUnion->info.entry.luDeletedDateAndTime);
            NCopyHiLo32(&pbuDataB128[108],
               &pEntryUnion->info.entry.luDeletorID);
            NWCMemMove(&pbuDataB128[112],
               pEntryUnion->info.entry.abuReserved2, (nuint) 8);
            NCopyLoHi32(&pbuDataB128[120],
               &pEntryUnion->info.entry.luPrimaryEntry);
            NCopyLoHi32(&pbuDataB128[124],
               &pEntryUnion->info.entry.luNameList);
         }
         break;
         default:
         {
            if (pEntryUnion->luAttrs & FA_DIRECTORY)
            {
               NWCMemMove(&pbuDataB128[12],
                  pEntryUnion->info.dir.abuName, (nuint) 12);
               NCopyLoHi32(&pbuDataB128[24],
                  &pEntryUnion->info.dir.luCreationDateAndTime);
               NCopyHiLo32(&pbuDataB128[28],
                  &pEntryUnion->info.dir.luOwnerID);
               NCopyLoHi32(&pbuDataB128[32],
                  &pEntryUnion->info.dir.luArchivedDateAndTime);
               NCopyHiLo32(&pbuDataB128[36],
                  &pEntryUnion->info.dir.luArchiverID);
               NCopyLoHi32(&pbuDataB128[40],
                  &pEntryUnion->info.dir.luModifiedDateAndTime);
               NCopyLoHi32(&pbuDataB128[44],
                  &pEntryUnion->info.dir.luNextTrusteeEntry);
               NWCMemMove(&pbuDataB128[48],
                  pEntryUnion->info.dir.abuReserved1, (nuint) 48);
               NCopyLoHi32(&pbuDataB128[96],
                  &pEntryUnion->info.dir.luMaxSpace);
               NCopyLoHi16(&pbuDataB128[100],
                  &pEntryUnion->info.dir.suInheritedRightsMask);
               NWCMemMove(&pbuDataB128[102],
                  pEntryUnion->info.dir.abuReserved2, (nuint) 26);
            }
            else
            {
               NWCMemMove(&pbuDataB128[12],
                  pEntryUnion->info.file.abuName, (nuint) 12);
               NCopyLoHi32(&pbuDataB128[24],
                  &pEntryUnion->info.file.luCreationDateAndTime);
               NCopyHiLo32(&pbuDataB128[28],
                  &pEntryUnion->info.file.luOwnerID);
               NCopyLoHi32(&pbuDataB128[32],
                  &pEntryUnion->info.file.luArchivedDateAndTime);
               NCopyHiLo32(&pbuDataB128[36],
                  &pEntryUnion->info.file.luArchiverID);
               NCopyLoHi32(&pbuDataB128[40],
                  &pEntryUnion->info.file.luModifiedDateAndTime);
               NCopyHiLo32(&pbuDataB128[44],
                  &pEntryUnion->info.file.luModifierID);
               NCopyLoHi32(&pbuDataB128[48],
                  &pEntryUnion->info.file.luFileSize);
               NWCMemMove(&pbuDataB128[52],
                  pEntryUnion->info.file.abuReserved1, (nuint) 44);
               NCopyLoHi16(&pbuDataB128[96],
                  &pEntryUnion->info.file.suInheritedRightsMask);
               NCopyLoHi16(&pbuDataB128[98],
                  &pEntryUnion->info.file.suLastAccessedDate);
               NWCMemMove(&pbuDataB128[100],
                  pEntryUnion->info.file.abuReserved2, (nuint) 28);
            }
         }
      }
   }
   else
   {
      NWCMemMove(&pbuDataB128[12],
         pEntryUnion->info.mac.abuFileName, (nuint) 32);
      NCopyLoHi32(&pbuDataB128[44],
         &pEntryUnion->info.mac.luResourceFork);
      NCopyLoHi32(&pbuDataB128[48],
         &pEntryUnion->info.mac.luResourceForkSize);
      NWCMemMove(&pbuDataB128[52],
         pEntryUnion->info.mac.abuFinderInfo, (nuint) 32);
      NWCMemMove(&pbuDataB128[84],
         pEntryUnion->info.mac.abuProDosInfo, (nuint) 6);
      NWCMemMove(&pbuDataB128[90],
         pEntryUnion->info.mac.abuReserved, (nuint) 38);
   }
   return;
}

/*manpage*NWNCPUnpackEntryStruct**********************************************
SYNTAX:  N_EXTERN_LIBRARY( void )
         NWNCPUnpackEntryStruct
         (
            pNWNCPEntryStruct pEntryUnion,
            pnuint8           pbuDataB77,
            nuint32           luRetMask
         )

REMARKS: Unpacks a pEntryUnion from an array buffer of 77 bytes.  Does not
         include trailing file name of 256 bytes.

         If bit 0 of luRetMask is set (NCP 87 01, *03*, 06, 16, 20, 29, 30)
         then a name structure will be returned and a name length will be
         unpacked in this function. Bit 0 will always be set when this
         function is called from NCP 87 03, because it is set whether or
         not the user passes it in that way.

ARGS: <  pEntryUnion
       > pbuDataB77
       > luRetMask
         The calling NCP must pass in the luRetMask from its parameter
         list; if bit 0 is set, a name length is extracted, otherwise
         it's set to zero

INCLUDE: ncpfile.h

CHANGES: 22 Sep 1993 - written - djharris
         13 Feb 1994 - modified to conditionally unpack length - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWNCPUnpackEntryStruct
(
   pNWNCPEntryStruct pEntryUnion,
   pnuint8           pbuDataB77,
   nuint32           luRetMask
)
{
   NCopyLoHi32(&pEntryUnion->luSpaceAlloc,          &pbuDataB77[0]);
   NCopyLoHi32(&pEntryUnion->luAttrs,               &pbuDataB77[4]);
   NCopyLoHi16(&pEntryUnion->suFlags,               &pbuDataB77[8]);
   NCopyLoHi32(&pEntryUnion->luDataStreamSize,      &pbuDataB77[10]);
   NCopyLoHi32(&pEntryUnion->luTotalStreamSize,     &pbuDataB77[14]);
   NCopyLoHi16(&pEntryUnion->suNumStreams,          &pbuDataB77[18]);
   NCopyLoHi16(&pEntryUnion->suCreationTime,        &pbuDataB77[20]);
   NCopyLoHi16(&pEntryUnion->suCreationDate,        &pbuDataB77[22]);
   NCopyHiLo32(&pEntryUnion->luCreatorID,           &pbuDataB77[24]);
   NCopyLoHi16(&pEntryUnion->suModifiedTime,        &pbuDataB77[28]);
   NCopyLoHi16(&pEntryUnion->suModifiedDate,        &pbuDataB77[30]);
   NCopyHiLo32(&pEntryUnion->luModifierID,          &pbuDataB77[32]);
   NCopyLoHi16(&pEntryUnion->suAccessedDate,        &pbuDataB77[36]);
   NCopyLoHi16(&pEntryUnion->suArchivedTime,        &pbuDataB77[38]);
   NCopyLoHi16(&pEntryUnion->suArchivedDate,        &pbuDataB77[40]);
   NCopyHiLo32(&pEntryUnion->luArchiverID,          &pbuDataB77[42]);
   NCopyLoHi16(&pEntryUnion->suInheritedRightsMask, &pbuDataB77[46]);
   NCopyLoHi32(&pEntryUnion->luDirBase,             &pbuDataB77[48]);
   NCopyLoHi32(&pEntryUnion->luDosDirBase,          &pbuDataB77[52]);
   NCopyLoHi32(&pEntryUnion->luVolNum,              &pbuDataB77[56]);
   NCopyLoHi32(&pEntryUnion->luEADataSize,          &pbuDataB77[60]);
   NCopyLoHi32(&pEntryUnion->luEAKeyCount,          &pbuDataB77[64]);
   NCopyLoHi32(&pEntryUnion->luEAKeySize,           &pbuDataB77[68]);
   NCopyLoHi32(&pEntryUnion->luNamSpcCreator,       &pbuDataB77[72]);

   /* the name length and name only come back from NCP 87 03 and
      any time when the info struct comes back and bit 0 is set in
      the luRetMask field (an input to the NCP) */

   if (luRetMask & 1)
      pEntryUnion->buNameLen = pbuDataB77[76];
   else
      pEntryUnion->buNameLen = 0;

   return;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/packent.c,v 1.7 1994/09/26 17:41:36 rebekah Exp $
*/
