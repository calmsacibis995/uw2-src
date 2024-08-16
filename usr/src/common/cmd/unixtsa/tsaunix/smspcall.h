/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/smspcall.h	1.4"
#ifndef _SMSPCALL_H_INCLUDED
#define _SMSPCALL_H_INCLUDED
//****************************************************************************
// SMSPCALL.H
//****************************************************************************

//****************************************************************************
//  Function Prototypes
//****************************************************************************

CCODE DNScanTargetServiceName(SMDR_THREAD *smdrThread);

static void DisplayEncryption(UINT8 *key, UINT8 *encryption);

CCODE DNConnectToTargetService(SMDR_THREAD *smdrThread);

CCODE DNAuthenticateTS(SMDR_THREAD *smdrThread);

CCODE DNReleaseTargetService(SMDR_THREAD *smdrThread);

CCODE DNBuildResourceList(SMDR_THREAD *smdrThread);

CCODE DNBeginRestoreSession(SMDR_THREAD *smdrThread);

CCODE DNIsDataSetExcluded(SMDR_THREAD *smdrThread);

CCODE DNRenameDataSet(SMDR_THREAD *smdrThread);

CCODE DNDeleteDataSet(SMDR_THREAD *smdrThread);

CCODE DNSetArchiveStatus(SMDR_THREAD *smdrThread);

CCODE DNOpenDataSetForRestore(SMDR_THREAD *smdrThread);

CCODE DNOpenDataSetForBackup(SMDR_THREAD *smdrThread);

CCODE DNReadDataSet(SMDR_THREAD *smdrThread);

CCODE DNWriteDataSet(SMDR_THREAD *smdrThread);

CCODE DNCloseDataSet(SMDR_THREAD *smdrThread);

CCODE DNScanSupportedNameSpaces(SMDR_THREAD *smdrThread);

CCODE DNSetRestoreOptions(SMDR_THREAD *smdrThread);

CCODE DNGetTargetResourceInfo(SMDR_THREAD *smdrThread);

CCODE DNParseDataSetName(SMDR_THREAD *smdrThread);

CCODE DNCatDataSetName(SMDR_THREAD *smdrThread);

CCODE DNGetNameSpaceTypeInfo(SMDR_THREAD *smdrThread);

CCODE DNSeparateDataSetName(SMDR_THREAD *smdrThread);

CCODE DNScanTargetServiceResource(SMDR_THREAD *smdrThread);

CCODE DNScanDataSetBegin(SMDR_THREAD *smdrThread);

CCODE DNScanNextDataSet(SMDR_THREAD *smdrThread);

CCODE DNScanDataSetEnd(SMDR_THREAD *smdrThread);

CCODE DNReturnToParent(SMDR_THREAD *smdrThread);

CCODE DNConvertError(SMDR_THREAD *smdrThread);

CCODE DNGetOpenModeOptionString(SMDR_THREAD *smdrThread);

CCODE DNGetTargetScanTypeString(SMDR_THREAD *smdrThread);

CCODE DNGetTargetSelectionTypeStr(SMDR_THREAD *smdrThread);

CCODE DNGetTargetServiceType(SMDR_THREAD *smdrThread);

CCODE DNGetUnsupportedOptions(SMDR_THREAD *smdrThread);

CCODE DNBeginRestoreSession(SMDR_THREAD *smdrThread);

CCODE DNFixDataSetName(SMDR_THREAD *smdrThread);

CCODE DNGetModuleVersionInfo(SMDR_THREAD *smdrThread);

CCODE DNGetResponderVersionInfo(SMDR_THREAD *smdrThread);

#endif
