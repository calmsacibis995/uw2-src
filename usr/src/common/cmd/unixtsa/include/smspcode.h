/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smspcode.h	1.3"

#if !defined(_SMSPCODE_H_INCLUDED_)
#define _SMSPCODE_H_INCLUDED_

#define SMSP_VERSION_NUMBER							0x6L
//define SMSP_ConnectToRemoteResource					obsolete
#define SMSP_ScanSMDRModule							0xF001
#define SMSP_SendData									0xF002
#define SMSP_ReceiveData								0xF003
#define SMSP_GetEncryptionKey							0xF004
#define SMSP_ScanSMDRModules							0xF005
#define SMSP_GetModuleVersionInfo						0xF006
#define SMSP_GetResponderVersionInfo					0xF007
#define SMSP_ConnectToRemoteResource					0xF008

#define SMSP_TSA_PROC_BEGIN							0x0000

#define SMSP_ConnectToTargetService					0x0000
#define SMSP_ReleaseTargetService						0x0001
#define SMSP_ScanDataSetBegin							0x0002
#define SMSP_ScanNextDataSet							0x0003
#define SMSP_ScanDataSetEnd							0x0004
#define SMSP_SetRestoreOptions							0x0005
#define SMSP_OpenDataSetForBackup						0x0006
#define SMSP_OpenDataSetForRestore						0x0007
#define SMSP_RenameDataSet								0x0008
#define SMSP_SetArchiveStatus							0x0009
#define SMSP_ReadDataSet								0x000A
#define SMSP_WriteDataSet								0x000B
#define SMSP_ScanTargetServiceName						0x000C
#define SMSP_ScanTargetServiceResource					0x000D
#define SMSP_GetTargetScanTypeString					0x000E
#define SMSP_GetTargetSelectionTypeStr					0x000F
#define SMSP_TSAConvertError							0x0010
#define SMSP_IsDataSetExcluded							0x0011
#define SMSP_DeleteDataSet								0x0012
#define SMSP_ReturnToParent							0x0013
#define SMSP_ScanSupportedNameSpaces			 		0x0014
#define SMSP_BuildResourceList					 		0x0015
#define SMSP_CatDataSetName							0x0016
#define SMSP_ParseDataSetName							0x0017
#define SMSP_SeparateDataSetName		  				0x0018
#define SMSP_GetTargetResourceInfo						0x0019
#define SMSP_GetNameSpaceTypeInfo						0x001A
#define SMSP_CloseDataSet								0x001B
#define SMSP_GetOpenModeOptionString					0x001C
#define SMSP_GetTargetServiceType						0x001D
#define SMSP_GetRedirectedAddress						0x001E
#define SMSP_GetUnsupportedOptions						0x001F
#define SMSP_GetTargetServiceAddress					0x0020
#define SMSP_AuthenticateTS							0x0021
#define SMSP_FixDataSetName		  				0x0022
#define SMSP_BeginRestoreSession					0x0023

#define SMSP_TSA_PROC_END								0x0030

#define SMSP_SDI_PROC_BEGIN							0x1000

#define SMSP_ConnectToSDI								0x1000
#define SMSP_ReleaseSDI								0x1001
#define SMSP_ListDevices								0x1002
#define SMSP_ListMedia									0x1003
#define SMSP_SubjugateDevice							0x1004
#define SMSP_EmancipateDevice							0x1005
#define SMSP_SubjugateMedia							0x1006
#define SMSP_EmancipateMedia							0x1007
#define SMSP_MountMedia								0x1008
#define SMSP_DismountMedia								0x1009
#define SMSP_OpenSessionForWriting						0x100A
#define SMSP_OpenSessionForReading						0x100B
#define SMSP_CloseSession								0x100C
#define SMSP_WriteSessionData							0x100D
#define SMSP_ReadSessionData							0x100E
#define SMSP_CancelDataTransfer						0x100F
#define SMSP_LabelMedia								0x1010
#define SMSP_DeleteMedia								0x1011
//define SMSP_ReturnMediaHeader							0x1012
#define SMSP_PositionMedia								0x1013
#define SMSP_MoveMedia									0x1014
#define SMSP_GetDeviceStatus							0x1015
#define SMSP_GetMediaStatus							0x1016
#define SMSP_GetDeviceCharacteristics					0x1017
#define SMSP_GetMediaCharacteristics					0x1018
#define SMSP_LabelDevice		 						0x1019
#define SMSP_SetSpanningSequence						0x101A
#define SMSP_SetReadSDIDefaults						0x101B
#define SMSP_RegisterAlertRoutine						0x101C
#define SMSP_ConvertValueToMessage						0x101D
#define SMSP_SDIConvertError							0x101E
#define SMSP_AlertResponse								0x101F
#define SMSP_FormatMedia								0x1020
#define SMSP_ReturnMediaHeader							0x1021

#define SMSP_SDI_PROC_END								0x1030
//                                      arrow indicates maximum length
#endif

