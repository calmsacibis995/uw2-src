#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsasmsp.C	1.6"
/**********************************************************************

Program Name:		Storage Management Services

Filename:			tsasmsp.c
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <smdr.h>
#include <smsdrerr.h>
#include "smstserr.h"
#include "smspcode.h"
#include "smspcall.h"
#include "tsasmsp.h"
#include <tsad.h>


/**********************************************************************
Function:		ServiceSMSP

Purpose:		

Parameters:	smsp, thread

Design Notes:	
**********************************************************************/
CCODE ServiceSMSP(UINT16 smsp, SMDR_THREAD *thread)
{
	switch(smsp)
	{
		case SMSP_GetModuleVersionInfo:
			DNGetModuleVersionInfo(thread);
			break;

		case SMSP_GetResponderVersionInfo:
			DNGetResponderVersionInfo(thread);
			break;

		case SMSP_ScanTargetServiceName:
			DNScanTargetServiceName(thread);
			break;

		case SMSP_ConnectToTargetService:
			DNConnectToTargetService(thread);
			break;

		case SMSP_AuthenticateTS:
			DNAuthenticateTS(thread);
			break;

		case SMSP_ReleaseTargetService:
			DNReleaseTargetService(thread);
			break;

		case SMSP_ScanDataSetBegin:
			DNScanDataSetBegin(thread);
			break;

		case SMSP_ScanNextDataSet:
			DNScanNextDataSet(thread);
			break;

		case SMSP_ScanDataSetEnd:
			DNScanDataSetEnd(thread);
			break;

		case SMSP_ScanSupportedNameSpaces:
			DNScanSupportedNameSpaces(thread);
			break;

		case SMSP_IsDataSetExcluded:
			DNIsDataSetExcluded(thread);
			break;

		case SMSP_ReturnToParent:
			DNReturnToParent(thread);
			break;

		case SMSP_OpenDataSetForBackup:
			DNOpenDataSetForBackup(thread);
			break;

		case SMSP_SetRestoreOptions:
			DNSetRestoreOptions(thread);
			break;

		case SMSP_GetOpenModeOptionString:
			DNGetOpenModeOptionString(thread);
			break;

		case SMSP_OpenDataSetForRestore:
			DNOpenDataSetForRestore(thread);
			break;

		case SMSP_ReadDataSet:
			DNReadDataSet(thread);
			break;

		case SMSP_WriteDataSet:
			DNWriteDataSet(thread);
			break;

		case SMSP_CloseDataSet:
			DNCloseDataSet(thread);
			break;

		case SMSP_RenameDataSet:
			DNRenameDataSet(thread);
			break;

		case SMSP_DeleteDataSet:
			DNDeleteDataSet(thread);
			break;

		case SMSP_GetNameSpaceTypeInfo:
			DNGetNameSpaceTypeInfo(thread);
			break;

		case SMSP_ParseDataSetName:
			DNParseDataSetName(thread);
			break;

		case SMSP_CatDataSetName:
			DNCatDataSetName(thread);
			break;

		case SMSP_SeparateDataSetName:
			DNSeparateDataSetName(thread);
			break;

		case SMSP_ScanTargetServiceResource:
			DNScanTargetServiceResource(thread);
			break;

		case SMSP_GetTargetResourceInfo:
			DNGetTargetResourceInfo(thread);
			break;

		case SMSP_BuildResourceList:
			DNBuildResourceList(thread);
			break;

		case SMSP_GetTargetScanTypeString:
			DNGetTargetScanTypeString(thread);
			break;

		case SMSP_GetTargetSelectionTypeStr:
			DNGetTargetSelectionTypeStr(thread);
			break;

		case SMSP_TSAConvertError:
			DNConvertError(thread);
			break;

		case SMSP_GetTargetServiceType:
			DNGetTargetServiceType(thread);
			break;

		case SMSP_GetUnsupportedOptions:
			DNGetUnsupportedOptions(thread);
			break;

		case SMSP_FixDataSetName:
			DNFixDataSetName(thread);
			break;

		case SMSP_BeginRestoreSession:
			DNBeginRestoreSession(thread);
			break;

		default:
			*(thread->ccode) = SwapUINT32(NWSMTS_UNSUPPORTED_FUNCTION);
	}

	return 0;
}

/********************************************************************/
