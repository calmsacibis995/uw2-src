/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smstsapi.h	1.1"

#if !defined(_SMSTSAPI_H_INCLUDED_)
#define _SMSTSAPI_H_INCLUDED_

#if !defined (_CLIB_HDRS_INCLUDED)
#define _CLIB_HDRS_INCLUDED
#include <string.h>
#include <time.h>
#define NETWARE3X
#include <stdlib.h>
#if defined(NLM) && defined(NETWARE_V320)
#include <nwlocale.h>
#endif
#endif
#if !defined(_SMSTYPES_H_INCLUDED)
#include <smstypes.h>
#endif
#if !defined(_SMSTSERR_H_INCLUDED)
#include <smstserr.h>
#endif

typedef 
CCODE _NWSMTSConnectToTargetService(
			UINT32					 *connectionID, 
			STRING					  targetServiceName,
			STRING					  targetUserName, 
			void					 *authentication);
typedef
CCODE _NWSMTSAuthenticateTS(
			UINT32                  *connectionHandle,
           STRING                   targetServiceName,
           UINT32					 authType,
           NWSM_LongByteStream     *authData);

typedef 
CCODE _NWSMTSReleaseTargetService(
			UINT32					 *connectionID);

typedef
CCODE _NWSMTSReturnToParent(
			UINT32					  connectionID, 
			UINT32					 *sequence);

typedef
CCODE _NWSMTSScanTargetServiceName(
			UINT32					  connectionID, 
			UINT32					 *sequence,
			STRING					  pattern, 
			STRING					  name);

typedef
CCODE _NWSMTSGetTargetServiceType(
			UINT32					  connectionID, 
			STRING					  name,
			STRING					  type, 
			STRING					  version);

typedef
CCODE _NWSMTSGetTargetServiceAddress(
			UINT32					  connectionID,
			STRING					  targetServiceName,
			UINT32					  *addressType,
			STRING					  address);

typedef
CCODE _NWSMTSGetTargetResourceInfo(
			UINT32					  connectionID, 
			STRING					  resource,
			UINT16					 *blocksize, 
			UINT32					 *totalblocks, 
			UINT32					 *freeblocks,
			NWBOOLEAN				 *isRemoveable, 
			UINT32					 *purgableblocks, 
			UINT32					 *unpurgedblocks,
			UINT32					 *migratedSectors,
			UINT32					 *preCompressedSectors,
			UINT32					 *compressedSectors);

typedef
CCODE _NWSMTSGetUnsupportedOptions(
			UINT32					  connectionID, 
			UINT32					 *unsupportedBackupOptions,
			UINT32					 *unsupportedRestoreOptions);

typedef
CCODE _NWSMTSScanTSResource(
			UINT32					  connectionID, 
			UINT32					 *sequence,
			STRING					  resource);

typedef
CCODE _NWSMTSListTSResources(
			UINT32						connectionID, 
			NWSM_NAME_LIST				**serviceResourceList);
			
typedef
CCODE _NWSMTSScanDataSetBegin(
			UINT32					  connectionID,
			NWSM_DATA_SET_NAME_LIST	 *resourceName,
			NWSM_SCAN_CONTROL		 *scanControl, 
			NWSM_SELECTION_LIST		 *selectionList,
			UINT32					 *sequence, 
			NWSM_SCAN_INFORMATION	**scanInformation, 
			NWSM_DATA_SET_NAME_LIST	**dataSetNames);

typedef
CCODE _NWSMTSScanNextDataSet(
			UINT32					  connectionID, 
			UINT32					 *sequence, 
			NWSM_SCAN_INFORMATION	**scanInformation,
			NWSM_DATA_SET_NAME_LIST	**dataSetNames);

typedef
CCODE _NWSMTSScanDataSetEnd(
			UINT32					  connectionID, 
			UINT32					 *sequence, 
			NWSM_SCAN_INFORMATION	**scanInformation,
			NWSM_DATA_SET_NAME_LIST	**dataSetNames);

typedef
CCODE _NWSMTSRenameDataSet(
			UINT32					  connectionID, 
			UINT32					  sequence, 
			UINT32					  nameSpaceType, 
			LSTRING					  newDataSetName);

typedef
CCODE _NWSMTSDeleteDataSet(
			UINT32					  connectionID, 
			UINT32					  sequence);

typedef
CCODE _NWSMTSSetArchiveStatus(
			UINT32  				  connectionID, 
			UINT32  				  handle, 
			UINT32  				  setFlag, 
			UINT32  				  archivedDateAndTime);

typedef
CCODE _NWSMTSOpenDataSetForRestore(
			UINT32					  connectionID, 
			UINT32					  parentHandle, 
			NWSM_DATA_SET_NAME_LIST	 *dataSetName, 
			UINT32					  mode, 
			UINT32					 *handle);

typedef
CCODE _NWSMTSOpenDataSetForBackup(
			UINT32					  connectionID, 
			UINT32  				  sequence, 
			UINT32  				  mode, 
			UINT32 					 *handle);

typedef
CCODE _NWSMTSCloseDataSet(
			UINT32					  connectionID, 
			UINT32					 *handle);

typedef
CCODE _NWSMTSReadDataSet(
			UINT32					  connectionID, 
			UINT32					  handle, 
			UINT32					  bytesToRead, 
			UINT32					 *bytesRead, 
			BUFFERPTR				  buffer);

typedef
CCODE _NWSMTSWriteDataSet(
			UINT32					  connectionID, 
			UINT32					  handle, 
			UINT32					  bytesToWrite, 
			BUFFERPTR				  buffer);
	
typedef
CCODE _NWSMTSIsDataSetExcluded(
			UINT32					  connectionID, 
			NWBOOLEAN				  isParent,
			NWSM_DATA_SET_NAME_LIST	 *namelist);

typedef
CCODE _NWSMTSBuildResourceList(
			UINT32					  connectionID);

typedef
CCODE _NWSMTSSetRestoreOptions(
			UINT32					  connectionID, 
			NWBOOLEAN				  checkCRC, 
			NWBOOLEAN				  dontCheckSelectionList, 
			NWSM_SELECTION_LIST		 *selectionList);

typedef
CCODE _NWSMTSParseDataSetName(
			UINT32  				  connectionID, 
			UINT32  				  nameSpaceType,
			STRING  				  dataSetName, 
			UINT16					 *count, 
			UINT16_BUFFER 			**namePositions,
			UINT16_BUFFER 			**separatorPositions);

typedef
CCODE _NWSMTSCatDataSetName(
			UINT32 					  connectionID, 
			UINT32 					  nameSpaceType,
			STRING 					  dataSetName, 
			STRING 					  terminalName, 
			NWBOOLEAN				  terminalIsParent,
			STRING_BUFFER			**newDataSetName);

typedef
CCODE _NWSMTSSeparateDataSetName(
			UINT32					  connectionID, 
			UINT32					  nameSpaceType,
			STRING					  dataSetName, 
			STRING_BUFFER 			**parentDataSetName,
			STRING_BUFFER 			**childDataSetName);

typedef
CCODE _NWSMTSGetNameSpaceTypeInfo(
			UINT32 					  connectionID,
			UINT32 					  nameSpaceType, 
			NWBOOLEAN				 *reverseOrder, 
			STRING_BUFFER 			**sep1, 
			STRING_BUFFER 			**sep2);

typedef
CCODE _NWSMTSScanSupportedNameSpaces(
			UINT32 					  connectionID, 
			UINT32					 *sequence,
			STRING 					  resourceName, 
			UINT32					 *nameSpaceType, 
			STRING 					  nameSpaceName);

typedef
CCODE  _NWSMTSListSupportedNameSpaces(
			UINT32						connectionID, 
			STRING						resourceName,
			NWSM_NAME_LIST 				**nameSpaceList);
		
typedef
CCODE _NWSMTSGetTargetSelTypeStr(
			UINT32					  connectionID, 
			UINT8					  typeNumber, 
			STRING					  selectionTypeString1, 
			STRING					  selectionTypeString2);

typedef
CCODE _NWSMTSGetTargetScanTypeString(
			UINT32					  connectionID,
			UINT8					  typeNumber, 
			STRING					  scanTypeString, 
			UINT32					 *required,
			UINT32					 *disallowed);

typedef
CCODE _NWSMTSGetOpenModeOptionString(
			UINT32					  connectionID, 
			UINT8					  optionNumber, 
			STRING					  optionString);

typedef
CCODE _NWSMTSConvertError(
			UINT32					  connectionID, 
			CCODE					  error, 
			STRING					  message);

typedef 
CCODE _NWSMGetVersionInfo(
			UINT32					  connectionID,
			NWSM_MODULE_VERSION_INFO *info);
 
CCODE NWSMConnectToTSA(
		char						 *TSA, 
		UINT32						 *connectionID);

CCODE NWSMReleaseTSA(
		UINT32						 *connectionID);


_NWSMTSConnectToTargetService		NWSMTSConnectToTargetService;
_NWSMTSAuthenticateTS				NWSMTSAuthenticateTS;
_NWSMTSReleaseTargetService		NWSMTSReleaseTargetService;
_NWSMTSReturnToParent				NWSMTSReturnToParent;
_NWSMTSScanTargetServiceName		NWSMTSScanTargetServiceName;
_NWSMTSGetTargetServiceType		NWSMTSGetTargetServiceType;
_NWSMTSGetTargetServiceAddress		NWSMTSGetTargetServiceAddress;
_NWSMTSGetTargetResourceInfo		NWSMTSGetTargetResourceInfo;
_NWSMTSScanTSResource	            NWSMTSScanTargetServiceResource;
_NWSMTSScanDataSetBegin			NWSMTSScanDataSetBegin;
_NWSMTSScanNextDataSet				NWSMTSScanNextDataSet;
_NWSMTSScanDataSetEnd				NWSMTSScanDataSetEnd;
_NWSMTSRenameDataSet				NWSMTSRenameDataSet;
_NWSMTSDeleteDataSet				NWSMTSDeleteDataSet;
_NWSMTSSetArchiveStatus			NWSMTSSetArchiveStatus;
_NWSMTSOpenDataSetForRestore		NWSMTSOpenDataSetForRestore;
_NWSMTSOpenDataSetForBackup		NWSMTSOpenDataSetForBackup;
_NWSMTSCloseDataSet				NWSMTSCloseDataSet;
_NWSMTSReadDataSet					NWSMTSReadDataSet;
_NWSMTSWriteDataSet				NWSMTSWriteDataSet;
_NWSMTSIsDataSetExcluded			NWSMTSIsDataSetExcluded;
_NWSMTSBuildResourceList			NWSMTSBuildResourceList;
_NWSMTSSetRestoreOptions			NWSMTSSetRestoreOptions;
_NWSMTSParseDataSetName			NWSMTSParseDataSetName;
_NWSMTSCatDataSetName				NWSMTSCatDataSetName;
_NWSMTSSeparateDataSetName			NWSMTSSeparateDataSetName;
_NWSMTSGetNameSpaceTypeInfo		NWSMTSGetNameSpaceTypeInfo;
_NWSMTSScanSupportedNameSpaces		NWSMTSScanSupportedNameSpaces;
_NWSMTSGetTargetSelTypeStr	        NWSMTSGetTargetSelectionTypeStr;
_NWSMTSGetTargetScanTypeString		NWSMTSGetTargetScanTypeString;
_NWSMTSGetOpenModeOptionString		NWSMTSGetOpenModeOptionString;
_NWSMTSConvertError				NWSMTSConvertError;
_NWSMTSListSupportedNameSpaces		NWSMTSListSupportedNameSpaces;
_NWSMTSListTSResources				NWSMTSListTSResources;
_NWSMTSGetUnsupportedOptions		NWSMTSGetUnsupportedOptions;
_NWSMGetVersionInfo				NWSMGetRequestorVersionInfo,
									NWSMGetResponderVersionInfo,
									NWSMGetSMSModuleVersionInfo;

void DestroyConnectionList(void);
#endif

