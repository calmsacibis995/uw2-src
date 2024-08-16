/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtvlsinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetVolumeSwitchInfo*********************************************
SYNTAX:  NWCCODE N_API NWGetVolumeSwitchInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_VOLUME_SWITCH_INFO NWPTR fseVolumeSwitchInfo
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseVolumeSwitchInfo

INCLUDE: nwncp.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 09  Volume Switch Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetVolumeSwitchInfo
(
   NWCONN_HANDLE            conn,
   nuint32                  startNum,
   NWFSE_VOLUME_SWITCH_INFO NWPTR fseVolumeSwitchInfo
)
{
   NWCCODE ccode;
   NWNCPFSESwitchInfo info;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE)NWNCP123s9GetVolSwitchInfo(&access, startNum,
         (pNWNCPFSEVConsoleInfo)
         &fseVolumeSwitchInfo->serverTimeAndVConsoleInfo,
         &fseVolumeSwitchInfo->reserved,
         &fseVolumeSwitchInfo->totalLFSCounters,
         &fseVolumeSwitchInfo->CurrentLFSCounters,
         &info);
   if (ccode == 0)
   {
      fseVolumeSwitchInfo->LFSCounters[0]   = info.luCreateDir;
      fseVolumeSwitchInfo->LFSCounters[1]   = info.luDelDir;
      fseVolumeSwitchInfo->LFSCounters[2]   = info.luMapPathToDirNum;
 /*      fseVolumeSwitchInfo->LFSCounters[3]   = info.luEraseFile; */
      fseVolumeSwitchInfo->LFSCounters[4]   = info.luModifyDirEntry;
 /*      fseVolumeSwitchInfo->LFSCounters[5]   = info.luRenameEntry; */
      fseVolumeSwitchInfo->LFSCounters[6]   = info.luGetAccessRights;
      fseVolumeSwitchInfo->LFSCounters[7]   = info.luGetAccessRightsFromIDs;
      fseVolumeSwitchInfo->LFSCounters[8]   = info.luMapDirNumToPath;
 /*      fseVolumeSwitchInfo->LFSCounters[9]   = info.luGetEntryFromPathStr; */
      fseVolumeSwitchInfo->LFSCounters[10]  = info.luGetOtherNSEntry;
 /*      fseVolumeSwitchInfo->LFSCounters[11]  = info.luDirSearch; */
      fseVolumeSwitchInfo->LFSCounters[12]  = info.luGetExtDirInfo;
      fseVolumeSwitchInfo->LFSCounters[13]  = info.luGetParentDirNum;
 /*      fseVolumeSwitchInfo->LFSCounters[14]  = info.luAddTrusteeRights; */
 /*      fseVolumeSwitchInfo->LFSCounters[15]  = info.luScanTrusteeRights; */
 /*      fseVolumeSwitchInfo->LFSCounters[16]  = info.luDelTrusteeRights; */
 /*      fseVolumeSwitchInfo->LFSCounters[17]  = info.luPurgeTrustee; */
 /*      fseVolumeSwitchInfo->LFSCounters[18]  = info.luFindNextTrusteeRef; */
 /*      fseVolumeSwitchInfo->LFSCounters[19]  = info.luScanUserRestrictNodes; */
 /*      fseVolumeSwitchInfo->LFSCounters[20]  = info.luAddUserRestrict; */
 /*      fseVolumeSwitchInfo->LFSCounters[21]  = info.luDelUserRestrict; */
 /*      fseVolumeSwitchInfo->LFSCounters[22]  = info.luReturnDirSpaceRestrict; */
 /*      fseVolumeSwitchInfo->LFSCounters[23]  = info.luGetActualAvailDiskSpc; */
 /*      fseVolumeSwitchInfo->LFSCounters[24]  = info.luCountOwnedFilesAndDirs; */
 /*      fseVolumeSwitchInfo->LFSCounters[25]  = info.luScanDelFiles; */
 /*      fseVolumeSwitchInfo->LFSCounters[26]  = info.luSalvageDelFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[27]  = info.luPurgeDelFile; */
      fseVolumeSwitchInfo->LFSCounters[28]  = info.luOpenFile;
      fseVolumeSwitchInfo->LFSCounters[29]  = info.luCreateFile;
      fseVolumeSwitchInfo->LFSCounters[30]  = info.luCreateAndOpenFile;
      fseVolumeSwitchInfo->LFSCounters[31]  = info.luMigrateFile;
      fseVolumeSwitchInfo->LFSCounters[32]  = info.luDeMigrateFile;
 /*      fseVolumeSwitchInfo->LFSCounters[33]  = info.luMigratedFileInfo; */
 /*      fseVolumeSwitchInfo->LFSCounters[34]  = info.luVolMigrationInfo; */
 /*      fseVolumeSwitchInfo->LFSCounters[35]  = info.luReadMigratedFileData; */
      fseVolumeSwitchInfo->LFSCounters[36]  = info.luReadFile;
      fseVolumeSwitchInfo->LFSCounters[37]  = info.luWriteFile;
 /*      fseVolumeSwitchInfo->LFSCounters[38]  = info.luASyncStartReadFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[39]  = info.luASyncDoReadFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[40]  = info.luASyncStartWriteFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[41]  = info.luASyncDoWriteFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[42]  = info.luASyncCheckWriteThrough; */
 /*      fseVolumeSwitchInfo->LFSCounters[43]  = info.luNewGetVolInfo; */
 /*      fseVolumeSwitchInfo->LFSCounters[44]  = info.luMapPathToDirNumOrPhantom; */
 /*      fseVolumeSwitchInfo->LFSCounters[45]  = info.luStationRightsGrantedBelow; */
 /*      fseVolumeSwitchInfo->LFSCounters[46]  = info.luGetStreamLensFromPathStr; */
      fseVolumeSwitchInfo->LFSCounters[47]  = info.luCheckAndGetDirEntry;
      fseVolumeSwitchInfo->LFSCounters[47]  = info.luGetDelEntry;
 /*      fseVolumeSwitchInfo->LFSCounters[48]  = info.luGetOriginalNS; */
      fseVolumeSwitchInfo->LFSCounters[49]  = info.luGetActualFileSize;
      fseVolumeSwitchInfo->LFSCounters[50]  = info.luVerifyNSNum;
 /*      fseVolumeSwitchInfo->LFSCounters[51]  = info.luVerifyDataStreamNum; */
      fseVolumeSwitchInfo->LFSCounters[52]  = info.luCheckVolNum;
 /*      fseVolumeSwitchInfo->LFSCounters[53]  = info.luGetFileSize; */
 /*      fseVolumeSwitchInfo->LFSCounters[54]  = info.luReadFileNoCheck; */
 /*      fseVolumeSwitchInfo->LFSCounters[55]  = info.luSetFileTimeAndDateStamp; */
 /*      fseVolumeSwitchInfo->LFSCounters[56]  = info.luGetFileHoles; */
 /*      fseVolumeSwitchInfo->LFSCounters[57]  = info.luGetHandleInfoData; */
      fseVolumeSwitchInfo->LFSCounters[58]  = info.luCloseFile;
      fseVolumeSwitchInfo->LFSCounters[59]  = info.luCommitFile;
 /*      fseVolumeSwitchInfo->LFSCounters[60]  = info.luGetDirEntry; */
 /*      fseVolumeSwitchInfo->LFSCounters[61]  = info.luCreateMigratedFileEntry; */
 /*      fseVolumeSwitchInfo->LFSCounters[62]  = info.luRenameNSEntry; */
 /*      fseVolumeSwitchInfo->LFSCounters[63]  = info.luCancelFileLockWait; */
 /*      fseVolumeSwitchInfo->LFSCounters[64]  = info.luCheckAndSetSingleFileLock; */
 /*      fseVolumeSwitchInfo->LFSCounters[65]  = info.luRelSingleFileLock; */
 /*      fseVolumeSwitchInfo->LFSCounters[66]  = info.luEnumFileLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[67]  = info.luCheckAndSetFileLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[68]  = info.luBackoutFileLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[69]  = info.luUnEnumFileLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[70]  = info.luRelFile; */
 /*      fseVolumeSwitchInfo->LFSCounters[71]  = info.luCheckAndSetSingleRecLock; */
 /*      fseVolumeSwitchInfo->LFSCounters[72]  = info.luRelSingleRecLock; */
 /*      fseVolumeSwitchInfo->LFSCounters[73]  = info.luEnumRecLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[74]  = info.luCheckAndSetRecLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[75]  = info.luBackoutRecLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[76]  = info.luUnEnumRecLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[77]  = info.luRelRecLocks; */
 /*      fseVolumeSwitchInfo->LFSCounters[78]  = info.luSetVolFlags; */
 /*      fseVolumeSwitchInfo->LFSCounters[79]  = info.luClearVolFlags; */
      fseVolumeSwitchInfo->LFSCounters[80]  = info.luGetOrigInfo;
      fseVolumeSwitchInfo->LFSCounters[81]  = info.luCreateMigratedDir;
      fseVolumeSwitchInfo->LFSCounters[82]  = info.luF3OpenCreate;
      fseVolumeSwitchInfo->LFSCounters[83]  = info.luF3InitFileSearch;
      fseVolumeSwitchInfo->LFSCounters[84]  = info.luF3ContFileSearch;
      fseVolumeSwitchInfo->LFSCounters[85]  = info.luF3RenFile;
      fseVolumeSwitchInfo->LFSCounters[86]  = info.luF3ScanForTrustees;
      fseVolumeSwitchInfo->LFSCounters[87]  = info.luF3ObtainFileInfo;
      fseVolumeSwitchInfo->LFSCounters[88]  = info.luF3ModifyInfo;
      fseVolumeSwitchInfo->LFSCounters[89]  = info.luF3EraseFile;
      fseVolumeSwitchInfo->LFSCounters[90]  = info.luF3SetDirHandle;
      fseVolumeSwitchInfo->LFSCounters[91]  = info.luF3AddTrustees;
      fseVolumeSwitchInfo->LFSCounters[92]  = info.luF3DelTrustees;
      fseVolumeSwitchInfo->LFSCounters[93]  = info.luF3AllocDirHandle;
      fseVolumeSwitchInfo->LFSCounters[94]  = info.luF3ScanSalvagedFiles;
      fseVolumeSwitchInfo->LFSCounters[95]  = info.luF3RecoverSalvagedFiles;
      fseVolumeSwitchInfo->LFSCounters[96]  = info.luF3PurgeSalvageableFile;
      fseVolumeSwitchInfo->LFSCounters[97]  = info.luF3GetNSSpecInfo;
      fseVolumeSwitchInfo->LFSCounters[98]  = info.luF3ModifyNSSpecInfo;
      fseVolumeSwitchInfo->LFSCounters[99]  = info.luF3SearchSet;
      fseVolumeSwitchInfo->LFSCounters[100] = info.luF3GetDirBase;
      fseVolumeSwitchInfo->LFSCounters[101] = info.luF3QueryNSInfo;
      fseVolumeSwitchInfo->LFSCounters[102] = info.luF3GetNSList;
      fseVolumeSwitchInfo->LFSCounters[103] = info.luF3GetHugeInfo;
      fseVolumeSwitchInfo->LFSCounters[104] = info.luF3SetHugeInfo;
      fseVolumeSwitchInfo->LFSCounters[105] = info.luF3GetFullPathStr;
      fseVolumeSwitchInfo->LFSCounters[106] = info.luF3GetEffectDirRights;
      fseVolumeSwitchInfo->LFSCounters[107] = info.luParseTree;
   }

   return ((NWCCODE) ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtvlsinf.c,v 1.7 1994/09/26 17:47:33 rebekah Exp $
*/

