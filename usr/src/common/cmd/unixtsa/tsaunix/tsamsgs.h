/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsamsgs.h	1.7"

#ifndef _TSAMSGS_H_INCLUDED
#define _TSAMSGS_H_INCLUDED

#define SZ_UNKNOWN_MESSAGE		0x0000
#define EXCLUDE_DEVICE_SPECIAL_FILES	0x0001
#define INCLUDE_REMOVABLE_FS		0x0002
#define T_SND_FAILED	0x0003
#define NFS_NAME_SPACE_STRING		0x0004
#define WORKSTATION_RESOURCE		0x0005
#define EXCLUDE_RESOURCES		0x0006
#define INCLUDE_RESOURCES		0x0007
#define EXCLUDE_DIRECTORIES		0x0008
#define INCLUDE_DIRECTORIES		0x0009
#define EXCLUDE_FILES			0x000A
#define INCLUDE_FILES			0x000B
#define EXCLUDE_PATH_FILES		0x000C
#define INCLUDE_PATH_FILES		0x000D
#define UNIX_PATH_SEPARATOR		0x000E
#define ERROR_LOG			0x000F
#define SKIPPED_DATA_SETS		0x0010
#define ROOT_OF_FS			0x0011
#define UNIX_FS_SEPARATOR		0x0012
#define DONT_TRAVERS_SUBDIRS		0x0013
#define T_SND_ERROR	0x0014
#define UNKNOWN_DATA_SET_NAME		0x0015
#define T_RCV_ERROR	0x0016
#define CR_LF_INDENT			0x0017
#define CR_LF_CR_LF			0x0018
#define BACKSLASH			0x0019
#define READLINK_ERROR			0x001A
#define INVALID_FULL_PATH_FORMAT			0x001B
#define DONT_BACKUP_DATA_STREAMS	0x001C
#define BUFFER_STATE_PROBLEM			0x001D
#define POLL_FAILED 			0x001E
#define T_RCV_FAILED			0x001F
#define TSA_ACCESS_DENIED		0x0020 /* TSA_BEGIN_ERROR_CODES */
#define T_SYNC_FAILED	0x0021
#define TSA_BUFFER_UNDERFLOW		0x0022
#define TSA_CANT_ALLOC_DIR_HANDLE	0x0023
#define T_NONBLOCKING_FAILED		0x0024
#define TSA_CREATE_DIR_ENTRY_ERR	0x0025
#define TSA_CREATE_ERROR		0x0026
#define TSA_DATA_SET_ALREADY_EXISTS	0x0027
#define TSA_DATA_SET_EXCLUDED		0x0028
#define TSA_DATA_SET_EXECUTE_ONLY	0x0029
#define TSA_DATA_SET_IN_USE		0x002a
#define TSA_DATA_SET_IS_OLDER		0x002b
#define TSA_DATA_SET_IS_OPEN		0x002c
#define TSA_DATA_SET_NOT_FOUND		0x002d
#define TSA_DELETE_ERR			0x002e
#define TSA_EXPECTING_HEADER		0x002f
#define TSA_EXPECTING_TRAILER		0x0030
#define TSA_GET_BIND_OBJ_NAME_ERR	0x0031
#define TSA_GET_DATA_STREAM_NAME_ERR	0x0032
#define TSA_GET_ENTRY_INDEX_ERR		0x0033
#define TSA_GET_NAME_SPACE_ENTRY_ERR	0x0034
#define TSA_GET_NAME_SPACE_SIZE_ERR	0x0035
#define DUP_FAILED		0x0036
#define TSA_GET_VOL_NAME_SPACE_ERR	0x0037
#define TSA_INVALID_CONNECTION_HANDL	0x0038
#define TSA_INVALID_DATA		0x0039
#define TSA_INVALID_DATA_SET_HANDLE	0x003a
#define TSA_INVALID_DATA_SET_NAME	0x003b
#define TSA_INVALID_DATA_SET_TYPE	0x003c
#define TSA_INVALID_HANDLE		0x003d
#define TSA_INVALID_MESSAGE_NUMBER	0x003e
#define TSA_INVALID_NAME_SPACE_TYPE	0x003f
#define TSA_INVALID_OBJECT_ID		0x0040
#define TSA_INVALID_OPEN_MODE_TYPE	0x0041
#define TSA_INVALID_PARAMETER		0x0042
#define TSA_INVALID_PATH		0x0043
#define TSA_INVALID_SCAN_TYPE		0x0044
#define TSA_INVALID_SEL_LIST_ENTRY	0x0045
#define TSA_INVALID_SELECTION_TYPE	0x0046
#define TSA_INVALID_SEQUENCE_NUMBER	0x0047
#define TSA_LOGIN_DENIED		0x0048
#define TSA_LOGOUT_ERROR		0x0049
#define TSA_NAME_SP_PATH_NOT_UPDATED	0x004a
#define TSA_NOT_READY			0x004b
#define TSA_NO_CONNECTION		0x004c
#define TSA_NO_MORE_DATA		0x004d
#define TSA_NO_MORE_DATA_SETS		0x004e
#define TSA_NO_MORE_NAMES		0x004f
#define TSA_NO_SEARCH_PRIVILEGES	0x0050
#define TSA_NO_SUCH_PROPERTY		0x0051
#define TSA_OPEN_DATA_STREAM_ERR	0x0052
#define TSA_OPEN_ERROR			0x0053
#define TSA_OPEN_MODE_TYPE_NOT_USED	0x0054
#define TSA_OUT_OF_DISK_SPACE		0x0055
#define TSA_OUT_OF_MEMORY		0x0056
#define TSA_OVERFLOW			0x0057
#define TSA_READ_EA_ERR			0x0058
#define TSA_READ_ERROR			0x0059
#define TSA_RESOURCE_NAME_NOT_FOUND	0x005a
#define TSA_SCAN_ERROR			0x005b
#define TSA_SCAN_FILE_ENTRY_ERR		0x005c
#define TSA_SCAN_IN_PROGRESS		0x005d
#define TSA_SCAN_NAME_SPACE_ERR		0x005e
#define OPEN_MNTTAB_FAILED		0x005f
#define TSA_SCAN_TYPE_NOT_USED		0x0060
#define TSA_SELECTION_TYPE_NOT_USED	0x0061
#define TSA_SET_FILE_INFO_ERR		0x0062
#define TSA_TRANSPORT_FAILURE		0x0063
#define TSA_TRANSPORT_PACKET_SIZE_ER	0x0064
#define TSA_TSA_NOT_FOUND		0x0065
#define TSA_UNSUPPORTED_FUNCTION	0x0066
#define TSA_VALID_PARENT_HANDLE		0x0067
#define TSA_WRITE_EA_ERR		0x0068
#define TSA_WRITE_ERROR_SHORT		0x0069
#define TSA_WRITE_ERROR			0x006a
#define TSA_REDIRECT_TRANSPORT		0x006b
#define TSA_MAX_CONNECTIONS		0x006c
#define TSA_COMPRESSION_CONFLICT	0x006d
#define TSA_INTERNAL_ERROR		0x006e
#define TSA_END_ERROR_CODES		TSA_INTERNAL_ERROR
#define TSA_BEGIN_ERROR_CODES		TSA_ACCESS_DENIED
#define TSA_USAGE		0x006F
#define TSA_NO_DATA_STREAMS	0x0070
#define TSA_NONE_STRING	0x0071
#define TSA_TARGET_SERVICE_TYPE	0x0072

#endif  // _TSAMSGS_H_INCLUDED