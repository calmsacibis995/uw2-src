/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s9GetVolSwitchInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s9GetVolSwitchInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luStartItemNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluTotalLFSCounters,
            pnuint32                 pluCurLFSCounters,
            pNWNCPFSESwitchInfo      pCounters,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartItemNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluTotalLFSCounters (optional)
      <  pluCurLFSCounters
      <  pCounters

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 09  Volume Switch Information

CHANGES: 24 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s9GetVolSwitchInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartItemNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalLFSCounters,
   pnuint32                 pluCurLFSCounters,
   pNWNCPFSESwitchInfo      pCounters
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 9)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 448)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartItemNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pluTotalLFSCounters)
         NCopyLoHi32(pluTotalLFSCounters, &abuReply[8]);

      NCopyLoHi32(pluCurLFSCounters, &abuReply[12]);

      NCopyLoHi32(&pCounters->luReadFile,                   &abuReply[16]);
      NCopyLoHi32(&pCounters->luWriteFile,                  &abuReply[20]);
      NCopyLoHi32(&pCounters->luDelFile,                    &abuReply[24]);
      NCopyLoHi32(&pCounters->luRenMoveFile,                &abuReply[28]);
      NCopyLoHi32(&pCounters->luOpenFile,                   &abuReply[32]);

      NCopyLoHi32(&pCounters->luCreateFile,                 &abuReply[36]);
      NCopyLoHi32(&pCounters->luCreateAndOpenFile,          &abuReply[40]);
      NCopyLoHi32(&pCounters->luCloseFile,                  &abuReply[44]);
      NCopyLoHi32(&pCounters->luScanDelFile,                &abuReply[48]);
      NCopyLoHi32(&pCounters->luSalvageFile,                &abuReply[52]);

      NCopyLoHi32(&pCounters->luPurgeFile,                  &abuReply[56]);
      NCopyLoHi32(&pCounters->luMigrateFile,                &abuReply[60]);
      NCopyLoHi32(&pCounters->luDeMigrateFile,              &abuReply[64]);
      NCopyLoHi32(&pCounters->luCreateDir,                  &abuReply[68]);
      NCopyLoHi32(&pCounters->luDelDir,                     &abuReply[72]);

      NCopyLoHi32(&pCounters->luDirScans,                   &abuReply[76]);
      NCopyLoHi32(&pCounters->luMapPathToDirNum,            &abuReply[80]);
      NCopyLoHi32(&pCounters->luModifyDirEntry,             &abuReply[84]);
      NCopyLoHi32(&pCounters->luGetAccessRights,            &abuReply[88]);
      NCopyLoHi32(&pCounters->luGetAccessRightsFromIDs,     &abuReply[92]);

      NCopyLoHi32(&pCounters->luMapDirNumToPath,            &abuReply[96]);
      NCopyLoHi32(&pCounters->luGetEntryFromPathStrBase,    &abuReply[100]);
      NCopyLoHi32(&pCounters->luGetOtherNSEntry,            &abuReply[104]);
      NCopyLoHi32(&pCounters->luGetExtDirInfo,              &abuReply[108]);
      NCopyLoHi32(&pCounters->luGetParentDirNum,            &abuReply[112]);

      NCopyLoHi32(&pCounters->luAddTrusteeR,                &abuReply[116]);
      NCopyLoHi32(&pCounters->luScanTrusteeR,               &abuReply[120]);
      NCopyLoHi32(&pCounters->luDelTrusteeR,                &abuReply[124]);
      NCopyLoHi32(&pCounters->luPurgeTrust,                 &abuReply[128]);
      NCopyLoHi32(&pCounters->luFindNextTrustRef,           &abuReply[132]);

      NCopyLoHi32(&pCounters->luScanUserRestNodes,          &abuReply[136]);
      NCopyLoHi32(&pCounters->luAddUserRest,                &abuReply[140]);
      NCopyLoHi32(&pCounters->luDelUserRest,                &abuReply[144]);
      NCopyLoHi32(&pCounters->luRtnDirSpaceRest,            &abuReply[148]);
      NCopyLoHi32(&pCounters->luGetActualAvailDskSp,        &abuReply[152]);

      NCopyLoHi32(&pCounters->luCntOwnedFilesAndDirs,       &abuReply[156]);
      NCopyLoHi32(&pCounters->luMigFileInfo,                &abuReply[160]);
      NCopyLoHi32(&pCounters->luVolMigInfo,                 &abuReply[164]);
      NCopyLoHi32(&pCounters->luReadMigFileData,            &abuReply[168]);
      NCopyLoHi32(&pCounters->luGetVolUsageStats,           &abuReply[172]);

      NCopyLoHi32(&pCounters->luGetActualVolUsageStats,     &abuReply[176]);
      NCopyLoHi32(&pCounters->luGetDirUsageStats,           &abuReply[180]);
      NCopyLoHi32(&pCounters->luNMFileReadsCnt,             &abuReply[184]);
      NCopyLoHi32(&pCounters->luNMFileWritesCnt,            &abuReply[188]);
      NCopyLoHi32(&pCounters->luMapPathToDirNumOrPhant,     &abuReply[192]);

      NCopyLoHi32(&pCounters->luStationHasRightsGranted,    &abuReply[196]);
      NCopyLoHi32(&pCounters->luGetDataStrmLensFromPathStrBs, &abuReply[200]);
      NCopyLoHi32(&pCounters->luCheckAndGetDirEntry,        &abuReply[204]);
      NCopyLoHi32(&pCounters->luGetDelEntry,                &abuReply[208]);
      NCopyLoHi32(&pCounters->luGetOrigNameSpace,           &abuReply[212]);

      NCopyLoHi32(&pCounters->luGetActualFileSize,          &abuReply[216]);
      NCopyLoHi32(&pCounters->luVerifyNSNum,                &abuReply[220]);
      NCopyLoHi32(&pCounters->luVerifyDataStrmNum,          &abuReply[224]);
      NCopyLoHi32(&pCounters->luCheckVolNum,                &abuReply[228]);
      NCopyLoHi32(&pCounters->luCommitFile,                 &abuReply[232]);

      NCopyLoHi32(&pCounters->luVMGetDirEntry,              &abuReply[236]);
      NCopyLoHi32(&pCounters->luCreateDMFileEntry,          &abuReply[240]);
      NCopyLoHi32(&pCounters->luRenameNSEntry,              &abuReply[244]);
      NCopyLoHi32(&pCounters->luLogFile,                    &abuReply[248]);
      NCopyLoHi32(&pCounters->luReleaseFile,                &abuReply[256]);

      NCopyLoHi32(&pCounters->luClearFile,                  &abuReply[260]);
      NCopyLoHi32(&pCounters->luSetVolFlag,                 &abuReply[264]);
      NCopyLoHi32(&pCounters->luClearVolFlag,               &abuReply[268]);
      NCopyLoHi32(&pCounters->luGetOrigInfo,                &abuReply[272]);
      NCopyLoHi32(&pCounters->luCreateMigratedDir,          &abuReply[276]);

      NCopyLoHi32(&pCounters->luF3OpenCreate,               &abuReply[280]);
      NCopyLoHi32(&pCounters->luF3InitFileSearch,           &abuReply[284]);
      NCopyLoHi32(&pCounters->luF3ContFileSearch,           &abuReply[288]);
      NCopyLoHi32(&pCounters->luF3RenFile,                  &abuReply[292]);
      NCopyLoHi32(&pCounters->luF3ScanForTrustees,          &abuReply[296]);

      NCopyLoHi32(&pCounters->luF3ObtainFileInfo,           &abuReply[300]);
      NCopyLoHi32(&pCounters->luF3ModifyInfo,               &abuReply[304]);
      NCopyLoHi32(&pCounters->luF3EraseFile,                &abuReply[308]);
      NCopyLoHi32(&pCounters->luF3SetDirHandle,             &abuReply[312]);
      NCopyLoHi32(&pCounters->luF3AddTrustees,              &abuReply[316]);

      NCopyLoHi32(&pCounters->luF3DelTrustees,              &abuReply[320]);
      NCopyLoHi32(&pCounters->luF3AllocDirHandle,           &abuReply[324]);
      NCopyLoHi32(&pCounters->luF3ScanSalvagedFiles,        &abuReply[328]);
      NCopyLoHi32(&pCounters->luF3RecoverSalvagedFiles,     &abuReply[332]);
      NCopyLoHi32(&pCounters->luF3PurgeSalvageableFile,     &abuReply[336]);

      NCopyLoHi32(&pCounters->luF3GetNSSpecInfo,            &abuReply[340]);
      NCopyLoHi32(&pCounters->luF3ModifyNSSpecInfo,         &abuReply[344]);
      NCopyLoHi32(&pCounters->luF3SearchSet,                &abuReply[348]);
      NCopyLoHi32(&pCounters->luF3GetDirBase,               &abuReply[352]);
      NCopyLoHi32(&pCounters->luF3QueryNSInfo,              &abuReply[356]);

      NCopyLoHi32(&pCounters->luF3GetNSList,                &abuReply[360]);
      NCopyLoHi32(&pCounters->luF3GetHugeInfo,              &abuReply[364]);
      NCopyLoHi32(&pCounters->luF3SetHugeInfo,              &abuReply[368]);
      NCopyLoHi32(&pCounters->luF3GetFullPathStr,           &abuReply[372]);
      NCopyLoHi32(&pCounters->luF3GetEffectDirRights,       &abuReply[376]);

      NCopyLoHi32(&pCounters->luParseTree,                  &abuReply[380]);
      NCopyLoHi32(&pCounters->luGetRefCntFromEntry,         &abuReply[384]);
      NCopyLoHi32(&pCounters->luAllocExtDir,                &abuReply[388]);
      NCopyLoHi32(&pCounters->luReadExtDir,                 &abuReply[392]);
      NCopyLoHi32(&pCounters->luWriteExtDir,                &abuReply[396]);

      NCopyLoHi32(&pCounters->luCommitExtDir,               &abuReply[400]);
      NCopyLoHi32(&pCounters->luClaimExtDir,                &abuReply[404]);
      NCopyLoHi32(&pCounters->luReturnExtDir,               &abuReply[408]);
      NCopyLoHi32(&pCounters->luSetOwningNS,                &abuReply[412]);
      NCopyLoHi32(&pCounters->luRemoveFile,                 &abuReply[416]);

      NCopyLoHi32(&pCounters->luRemoveFileCompletely,       &abuReply[420]);
      NCopyLoHi32(&pCounters->luGetNSInfo,                  &abuReply[424]);
      NCopyLoHi32(&pCounters->luClearPhantom,               &abuReply[428]);
      NCopyLoHi32(&pCounters->luGetMaxUserRestric,          &abuReply[432]);
      NCopyLoHi32(&pCounters->luGetCurrDiskUsedAmt,         &abuReply[436]);

      NCopyLoHi32(&pCounters->luFlushVol,                   &abuReply[440]);
      NCopyLoHi32(&pCounters->luSetCompressedFileSize,      &abuReply[444]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s9.c,v 1.7 1994/09/26 17:33:04 rebekah Exp $
*/
