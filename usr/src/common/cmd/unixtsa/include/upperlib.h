/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/upperlib.h	1.1"
/* **************************************************************************
* Program Name:		Storage Management Services (SDAPI)
*
* Filename:			upperlib.h
*
* Date Created:		Feburary 17, 1992
*
* Version:			4.0
*
* Files used:		smstypes.h, smsdefns.h, and smstserr.h.
*
* Date Modified:	
*
* Modifications:	
*
* Comments:			
*
**************************************************************************/

#if !defined(_ULIB_H_INCLUDED_)
#define _ULIB_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <smstypes.h>
#include "smdr.h"
#include <smspcode.h>
#include <smstsapi.h>
#include <smstserr.h>

#include <smsdrerr.h>
#include <smspcode.h>

#if defined(NLM)
#include <nwbindry.h>
#endif

#if defined(NETWARE_V320) && defined(NLM)
	extern char **programMesgTable;
#else
	extern char *programMesgTable[];
#endif

typedef struct
{
	CCODE (*ConnectToTargetService)();
	CCODE (*AuthenticateTS)();
	CCODE (*ReleaseTargetService)();
}
CONNECTION_PROCS;

typedef struct
{
	CCODE (*OpenDataSetForRestore)();
	CCODE (*OpenDataSetForBackup)();
	CCODE (*ReadDataSet)();
	CCODE (*WriteDataSet)();
	CCODE (*CloseDataSet)();
	CCODE (*RenameDataSet)();

	CCODE (*DeleteDataSet)();
	CCODE (*SetArchiveStatus)();
}
COPY_PROCS;

typedef struct
{
	CCODE (*IsDataSetExcluded)();
	CCODE (*BuildResourceList)();
	CCODE (*SetRestoreOptions)();
}
OPTION_PROCS;

typedef struct
{
	CCODE (*ParseDataSetName)();
	CCODE (*CatDataSetName)();
	CCODE (*SeparateDataSetName)();
	CCODE (*GetNameSpaceTypeInfo)();
	CCODE (*ScanSupportedNameSpaces)();
}	
PARSE_PROCS;

typedef struct
{
	CCODE (*ReturnToParent)();
	CCODE (*ScanTargetServiceName)();
	CCODE (*GetTargetResourceInfo)();
	CCODE (*ScanTargetServiceResource)();
	CCODE (*ScanDataSetBegin)();
	CCODE (*ScanNextDataSet)();
	CCODE (*ScanDataSetEnd)();
}
SCAN_PROCS;

typedef struct
{
	CCODE (*GetTargetSelectionTypeStr)();
	CCODE (*GetTargetScanTypeString)();
	CCODE (*GetOpenModeOptionString)();
	CCODE (*ConvertError)();
}
STRING_PROCS;

extern SMDR_PROCEDURES smdrProcs;
extern RESOURCE_PROCEDURES TSA_SMSPs[];
extern char smdrIsImported;

typedef
CCODE _NWSMTSGetRedirectedAddress(
			UINT32					connectionID,
			UINT32					*protocolType,
			UINT16					*addressSize,
			void					**newAddress);

_NWSMTSGetRedirectedAddress GetRedirectedAddress;
SMDR_CONNECTION* ValidConnection(UINT32 connectionID);
UINT32 NewConnectionID(SMDR_CONNECTION *addition);
CCODE DeleteConnection(SMDR_CONNECTION *deletion);
void ImportSMDR(void);
void GatherPacketizers(void);
CCODE GetTSAFunction(UINT32 connectionID, UINT16 smspcode);

_NWSMTSScanTargetServiceName		UPScanTargetServiceName;
_NWSMTSConnectToTargetService		UPConnectToTargetService;
_NWSMTSAuthenticateTS				UPAuthenticateTS;
_NWSMTSReleaseTargetService		UPReleaseTargetService;
_NWSMTSScanDataSetBegin			UPScanDataSetBegin;
_NWSMTSScanNextDataSet				UPScanNextDataSet;
_NWSMTSScanDataSetEnd				UPScanDataSetEnd;
_NWSMTSScanSupportedNameSpaces		UPScanSupportedNameSpaces;
_NWSMTSIsDataSetExcluded			UPIsDataSetExcluded;
_NWSMTSReturnToParent				UPReturnToParent;
_NWSMTSOpenDataSetForBackup		UPOpenDataSetForBackup;
_NWSMTSSetRestoreOptions			UPSetRestoreOptions;
_NWSMTSGetOpenModeOptionString		UPGetOpenModeOptionString;
_NWSMTSOpenDataSetForRestore		UPOpenDataSetForRestore;
_NWSMTSReadDataSet					UPReadDataSet;
_NWSMTSWriteDataSet				UPWriteDataSet;
_NWSMTSCloseDataSet				UPCloseDataSet;
_NWSMTSRenameDataSet				UPRenameDataSet;
_NWSMTSSetArchiveStatus			UPSetArchiveStatus;
_NWSMTSDeleteDataSet				UPDeleteDataSet;
_NWSMTSGetNameSpaceTypeInfo		UPGetNameSpaceTypeInfo;
_NWSMTSParseDataSetName			UPParseDataSetName;
_NWSMTSCatDataSetName				UPCatDataSetName;
_NWSMTSSeparateDataSetName			UPSeparateDataSetName;
_NWSMTSScanTSResource				UPScanTargetServiceResource;
_NWSMTSGetTargetResourceInfo		UPGetTargetResourceInfo;
_NWSMTSBuildResourceList			UPBuildResourceList;
_NWSMTSGetTargetScanTypeString		UPGetTargetScanTypeString;
_NWSMTSGetTargetSelTypeStr			UPGetTargetSelectionTypeStr;
_NWSMTSConvertError				UPConvertTSAError;
_NWSMTSGetTargetServiceType		UPGetTargetServiceType;
_NWSMTSGetUnsupportedOptions		UPGetUnsupportedOptions;
_NWSMTSGetTargetServiceAddress		UPGetTargetServiceAddress;


#if !defined(STRSIZE)
#define STRSIZE(s)	(strlen((char*)s)+1)
#endif

#if defined(DEBUG_CODE)
#define STATIC
#else
#define STATIC static
#endif

#if defined(NLM)
#define Calloc(a,b,c)	_calloc(a,b,c)
#define Malloc(a,b)		_malloc(a,b)
#else
#define Calloc(a,b,c)	calloc(b,c)
#define Malloc(a,b)		malloc(b)
#endif

void *_calloc (int ID, UINT32 n, UINT32 size);
void *_malloc (int ID, UINT32 size);

#endif
