/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/lowerlib.h	1.1"

#include <smspcode.h>
#include <smstypes.h>
#include <smdr.h>
#include <smstsapi.h>
#include <smsdrerr.h>
#define STRSIZE(s)	(strlen((char*)s)+1)

#if defined(__TURBOC__) || defined(MSC)
#include <mem.h>
#endif

#include <string.h>
#ifndef UNIX
#include <process.h>
#endif

extern SERVICE_SMSP SDIServiceSMSP, TSAServiceSMSP;

#define SendRcode(x)		(*(smdrThread->ccode) = x)
#define SMDR_ID			(smdrThread->connectionID)
#define SMDRLoad(b,s)		smdrThread->LoadSMDR(smdrThread, b, s)

typedef
CCODE SERVICE_PACKETS(		
		SMDR_THREAD						*thread);

SERVICE_PACKETS
	DNScanTargetServiceName,
	DNConnectToTargetService,
	DNAuthenticateTS,
	DNReleaseTargetService,
	DNScanDataSetBegin,
	DNScanNextDataSet,
	DNScanDataSetEnd,
	DNScanSupportedNameSpaces,
	DNIsDataSetExcluded,
	DNReturnToParent,
	DNOpenDataSetForBackup,
	DNSetRestoreOptions,
	DNGetOpenModeOptionString,
	DNOpenDataSetForRestore,
	DNReadDataSet,
	DNWriteDataSet,
	DNCloseDataSet,
	DNRenameDataSet,
	DNSetArchiveStatus,
	DNDeleteDataSet,
	DNGetNameSpaceTypeInfo,
	DNParseDataSetName,
	DNCatDataSetName,
	DNSeparateDataSetName,
	DNScanTargetServiceResource,
	DNGetTargetResourceInfo,
	DNBuildResourceList,
	DNGetTargetScanTypeString,
	DNGetTargetSelectionTypeStr,
	DNConvertTSAError,
	DNGetTargetServiceType,
	DNGetRedirectedAddress,
	DNGetUnsupportedOptions,
	DNGetTargetServiceAddress;

_NWSMTSScanTargetServiceName		ScanTargetServiceName;
_NWSMTSConnectToTargetService		ConnectToTargetService;
_NWSMTSAuthenticateTS                         AuthenticateTS;
_NWSMTSReleaseTargetService		ReleaseTargetService;
_NWSMTSScanDataSetBegin			ScanDataSetBegin;
_NWSMTSScanNextDataSet				ScanNextDataSet;
_NWSMTSScanDataSetEnd				ScanDataSetEnd;
_NWSMTSScanSupportedNameSpaces		ScanSupportedNameSpaces;
_NWSMTSIsDataSetExcluded			IsDataSetExcluded;
_NWSMTSReturnToParent				ReturnToParent;
_NWSMTSOpenDataSetForBackup		OpenDataSetForBackup;
_NWSMTSSetRestoreOptions			SetRestoreOptions;
_NWSMTSGetOpenModeOptionString		GetOpenModeOptionString;
_NWSMTSOpenDataSetForRestore		OpenDataSetForRestore;
_NWSMTSReadDataSet					ReadDataSet;
_NWSMTSWriteDataSet				WriteDataSet;
_NWSMTSCloseDataSet				CloseDataSet;
_NWSMTSRenameDataSet				RenameDataSet;
_NWSMTSSetArchiveStatus			SetArchiveStatus;
_NWSMTSDeleteDataSet				DeleteDataSet;
_NWSMTSGetNameSpaceTypeInfo		GetNameSpaceTypeInfo;
_NWSMTSParseDataSetName			ParseDataSetName;
_NWSMTSCatDataSetName				CatDataSetName;
_NWSMTSSeparateDataSetName			SeparateDataSetName;
_NWSMTSScanTSResource				ScanTargetServiceResource;
_NWSMTSGetTargetResourceInfo		GetTargetResourceInfo;
_NWSMTSBuildResourceList			BuildResourceList;
_NWSMTSGetTargetScanTypeString		GetTargetScanTypeString;
_NWSMTSGetTargetSelTypeStr			GetTargetSelectionTypeStr;
_NWSMTSConvertError				ConvertTSAError;
_NWSMTSGetTargetServiceType		GetTargetServiceType;
_NWSMTSGetUnsupportedOptions		GetUnsupportedOptions;
_NWSMTSGetTargetServiceAddress		GetTargetServiceAddress;

typedef
CCODE _NWSMTSGetRedirectedAddress(
			UINT32					connectionID,
			UINT32					*protocolType,
			UINT16					*addressSize,
			void					**newAddress);

_NWSMTSGetRedirectedAddress		GetRedirectedAddress;

CCODE TSAInit(void);
CCODE  TSAExit(void);
CCODE SDIInit(void);
CCODE  SDIExit(void);
