/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsapi.h	1.1"

#include <smstypes.h>
#include <smsdefns.h>

#include <smstsapi.h>
#include <smstserr.h>

#if !defined(_TSAPI_H_INCLUDED_)
#define _TSAPI_H_INCLUDED_

// The following lines implement the prototypes inherited from smstsapi.h for
// the identifiers given.  

_NWSMTSBuildResourceList			BuildResourceList;
_NWSMTSCatDataSetName				CatDataSetName;
_NWSMTSCloseDataSet				CloseDataSet;
_NWSMTSConnectToTargetService		ConnectToTargetService;
_NWSMTSConvertError				ConvertError;
_NWSMTSDeleteDataSet				DeleteDataSet;
_NWSMTSGetNameSpaceTypeInfo		GetNameSpaceTypeInfo;
_NWSMTSGetOpenModeOptionString		GetOpenModeOptionString;
_NWSMTSGetTargetResourceInfo		GetTargetResourceInfo;
_NWSMTSGetTargetScanTypeString		GetTargetScanTypeString;
_NWSMTSGetTargetSelTypeStr	        GetTargetSelectionTypeStr;
_NWSMTSGetTargetServiceType		GetTargetServiceType;
_NWSMTSIsDataSetExcluded			IsDataSetExcluded;
_NWSMTSOpenDataSetForBackup		OpenDataSetForBackup;
_NWSMTSOpenDataSetForRestore		OpenDataSetForRestore;
_NWSMTSParseDataSetName	  		ParseDataSetName;
_NWSMTSReadDataSet					ReadDataSet;
_NWSMTSReleaseTargetService	 	ReleaseTargetService;
_NWSMTSRenameDataSet				RenameDataSet;
_NWSMTSReturnToParent				ReturnToParent;
_NWSMTSScanDataSetBegin			ScanDataSetBegin;
_NWSMTSScanDataSetEnd				ScanDataSetEnd;
_NWSMTSScanNextDataSet				ScanNextDataSet;
_NWSMTSScanSupportedNameSpaces		ScanSupportedNameSpaces;
_NWSMTSScanTargetServiceName		ScanTargetServiceName;
_NWSMTSScanTSResource	            ScanTargetServiceResource;
_NWSMTSSeparateDataSetName			SeparateDataSetName;
_NWSMTSSetArchiveStatus			SetArchiveStatus;
_NWSMTSSetRestoreOptions			SetRestoreOptions;
_NWSMTSWriteDataSet				WriteDataSet;

#endif // _TSAPI_H_INCLUDED_

