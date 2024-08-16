/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tempfile.h	1.1"
//****************************************************************************
// TEMPFILE.H
//****************************************************************************

#define	ERROR_LOG_TYPE			1
#define	SKIPPED_DATA_TYPE		2

#define	TEMP_CREATE_FAILED	-1
#define	TEMP_DELETE_FAILED	-1

#define ERROR_LOG_SEQUENCE					0x00000001
#define SKIPPED_DATA_SETS_SEQUENCE		0x00000002

//****************************************************************************
//  Function Prototypes
//****************************************************************************

CCODE CreateTempFile(TEMP_FILE_HANDLE *tempHandle, UINT16 fileType);

CCODE CloseAndDeleteTempFile(TEMP_FILE_HANDLE *tempHandle);

CCODE WriteTempFile(TEMP_FILE_HANDLE *tempHandle, BUFFERPTR buffer,
		UINT16 bytesToWrite, UINT16 *bytesWritten);

CCODE ReadTempFile(TEMP_FILE_HANDLE *tempHandle, NWBOOLEAN adjustBuffer,
		BUFFERPTR *buffer, UINT16 *bytesToRead, UINT16 *bytesRead);

void  LogSkippedDataSets(SEQUENCE_STRUCT *dataSetSequence, CCODE ccode);

void	LogError(UINT32 connection, UINT16 messageNumber);
