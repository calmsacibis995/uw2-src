/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/ncpcodes.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/ncpcodes.h 1.4 (Novell) 7/30/91
 */

/*
** The NCP Request/Reply Types
*/

#define NCP_CONNECT_RT				0x1111  /* (base/connect) */
#define NCP_REQUEST_RT				0x2222
#define NCP_REPLY_RT				0x3333
#define NCP_DISCONNECT_RT			0x5555  /* (base/connect) */
#define NCP_BEING_PROCESSED_RT		0x9999



/*
** The GENERAL_SERVICES NCP Group
*/

#define NCP_GENERAL_SERVICES					23

	/* Accounting Services (general/accounting) */
#define NCP_GET_ACCOUNT_STATUS					150
#define NCP_SUBMIT_ACCOUNT_CHARGE				151
#define NCP_SUBMIT_ACCOUNT_HOLD					152
#define NCP_SUBMIT_ACCOUNT_NOTE					153

	/* Bindery Services (general/bindery) */
#define NCP_ADD_OBJECT_TO_GROUP					65
#define NCP_CHANGE_OBJECT_PASSWORD				64
#define NCP_CHANGE_OBJECT_SECURITY				56
#define NCP_CHANGE_PROPERTY_SECURITY			59
#define NCP_CHANGE_USER_PASSWORD				1		/* old */
#define NCP_CLOSE_BINDERY						68
#define NCP_CREATE_OBJECT						50
#define NCP_CREATE_PROPERTY						57
#define NCP_DELETE_OBJECT						51
#define NCP_DELETE_OBJECT_FROM_GROUP			66
#define NCP_DELETE_PROPERTY						58
#define NCP_GET_BINDERY_ACCESS_LEVEL			70
#define NCP_GET_GROUP_NAME						8		/* old */
#define NCP_GET_GROUP_NUMBER					7		/* old */
#define NCP_GET_MEMBER_SET_OF_GROUP				9		/* old */
#define NCP_GET_OBJECT_ACCESS_LEVEL				72
#define NCP_GET_OBJECT_NAME						54
#define NCP_GET_OBJECT_NUMBER					53
#define NCP_GET_USER_NAME						4		/* old */
#define NCP_GET_USER_NUMBER						3		/* old */
#define NCP_IS_OBJECT_IN_GROUP					67
#define NCP_OPEN_BINDERY						69
#define NCP_READ_PROPERTY_VALUE					61
#define NCP_RENAME_OBJECT						52
#define NCP_SCAN_OBJECT							55
#define NCP_SCAN_OBJECT_TRUSTEE_PATHS			71
#define NCP_SCAN_PROPERTY						60
#define NCP_VERIFY_OBJECT_PASSWORD				63
#define NCP_WRITE_PROPERTY_VALUE				62

	/* Misc. Connection Services (general/conn-misc) */
#define NCP_GET_INTERNET_ADDRESS				19
#define NCP_GET_LOGIN_KEY						23	 /* NO DOC */
#define NCP_GET_OBJECT_CONNECTION_LIST			21
#define NCP_GET_STATION_LOGGED_INFO				5		/* old */
#define NCP_GET_STATIONS_LOGGED_INFO			22
#define NCP_GET_USER_CONNECTION_LIST			2		/* old */
#define NCP_IS_OBJECT_MANAGER					73
#define NCP_KEYED_CHANGE_PASSWORD				75	 /* NO DOC */
#define NCP_KEYED_LOGIN							24	 /* NO DOC */
#define NCP_KEYED_VERIFY_PASSWORD				74	 /* NO DOC */
#define NCP_LIST_OBJECT_RELATIONS				76
#define NCP_LOGIN_OBJECT						20
#define NCP_LOGIN_USER							0		/* old */

	/* Misc. File Services (general/file-misc) */
#define NCP_SCAN_FILE_INFORMATION				15
#define NCP_SET_FILE_INFORMATION				16

	/* File Server Services (general/server) */
#define NCP_CHECK_CONSOLE_PRIVILEGES			200
#define NCP_CLEAR_CONNECTION_NUMBER				210
#define NCP_DISABLE_FILE_SERVER_LOGIN			203
#define NCP_DISABLE_TRANSACTION_TRACKING		207
#define NCP_DOWN_FILE_SERVER					211
#define NCP_ENABLE_FILE_SERVER_LOGIN			204
#define NCP_ENABLE_TRANSACTION_TRACKING			208
#define NCP_GET_CONNECTION_S_OPEN_FILES			219
#define NCP_GET_CONNECTION_S_SEMAPHORES			225
#define NCP_GET_CONNECTION_S_TASK_INFORMATION	218
#define NCP_GET_CONNECTION_S_USAGE_STATISTICS	229
#define NCP_GET_CONNECTIONS_USING_A_FILE		220
#define NCP_GET_DISK_CHANNEL_STATISTICS			217
#define NCP_GET_DISK_UTILIZATION				14
#define NCP_GET_DRIVE_MAPPING_TABLE				215
#define NCP_GET_FILE_SERVER_DESCRIPTION_STRINGS	201
#define NCP_GET_FILE_SERVER_INFORMATION			17
#define NCP_GET_FILE_SERVER_LAN_I/O_STATISTICS	231
#define NCP_GET_FILE_SERVER_LOGIN_STATUS		205
#define NCP_GET_FILE_SERVER_MISC_INFORMATION	232
#define NCP_GET_FILE_SYSTEM_STATISTICS			212
#define NCP_GET_LAN_DRIVER_CONFIG_INFORMATION	227
#define NCP_GET_LOGICAL_RECORDS_BY_CONNECTION	223
#define NCP_GET_LOGICAL_RECORD_INFORMATION		224
#define NCP_GET_NETWORK_SERIAL_NUMBER			18
#define NCP_GET_OBJECT_REMAINING_DISK_SPACE		230
#define NCP_GET_PHYSICAL_RECORD_LOCKS_CONN_FILE 221
#define NCP_GET_PHYSICAL_RECORD_LOCKS_BY_FILE	222
#define NCP_GET_SEMAPHORE_INFORMATION			226
#define NCP_GET_TRANSACTION_TRACKING_STATISTICS	213
#define NCP_GET_VOLUME_INFORMATION				233
#define NCP_PURGE_ALL_ERASED_FILES				206
#define NCP_READ_DISK_CACHE_STATISTICS			214
#define NCP_READ_PHYSICAL_DISK_STATISTICS		216
#define NCP_RELEASE_A_RESOURCE					252
#define NCP_SEND_CONSOLE_BROADCAST				209
#define NCP_SET_FILE_SERVER_DATE_AND_TIME		202
#define NCP_VERIFY_SERIALIZATION				12

	/* Queue Services (general/queue) */
#define NCP_ABORT_SERVICING_QUEUE_JOB			115
#define NCP_ATTACH_SERVER_TO_QUEUE				111
#define NCP_CHANGE_QUEUE_JOB_ENTRY				109
#define NCP_CHANGE_QUEUE_JOB_POSITION			110 /* NO DOC */
#define NCP_CHANGE_TO_CLIENT_RIGHTS				116
#define NCP_CLOSE_FILE_AND_START_QUEUE_JOB		105
#define NCP_CREATE_QUEUE						100
#define NCP_CREATE_QUEUE_JOB_AND_FILE			104
#define NCP_DESTROY_QUEUE						101
#define NCP_DETACH_QUEUE_SERVER_FROM_Q			112
#define NCP_FINISH_SERVICING_QUEUE_JOB			114
#define NCP_GET_QUEUE_JOB_FILE_SIZE				120
#define NCP_GET_QUEUE_JOB_LIST					107
#define NCP_READ_QUEUE_CURRENT_STATUS			102
#define NCP_READ_QUEUE_JOB_ENTRY				108
#define NCP_READ_QUEUE_SERVER_CURRENT_STATUS	118
#define NCP_REMOVE_JOB_FROM_QUEUE				106
#define NCP_RESTORE_QUEUE_SERVER_RIGHTS			117
#define NCP_SERVICE_QUEUE_JOB					113
#define NCP_SET_QUEUE_CURRENT_STATUS			103
#define NCP_SET_QUEUE_SERVER_CURRENT_STATUS		119



/*
** The DIRECTORY_SERVICES NCP Group
*/

#define NCP_DIRECTORY_SERVICES					22

	/* Directory Services (directory) */
#define NCP_ADD_OBJECT_DISK_LIMIT_ON_VOLUME		33		/* 3.0 */
#define NCP_ADD_TRUSTEE_TO_FILE_DIR				39		/* 3.0 */
#define NCP_ADD_TRUSTEE_TO_DIRECTORY			13
#define NCP_ALLOC_PERMANENT_DIR_HANDLE			18
#define NCP_ALLOC_SPECIAL_TEMPORARY_DIR_HANDLE	22
#define NCP_ALLOC_TEMPORARY_DIR_HANDLE			19
#define NCP_CREATE_DIRECTORY					10
#define NCP_DEALLOCATE_DIR_HANDLE				20
#define NCP_DELETE_DIRECTORY					11
#define NCP_DELETE_TRUSTEE_FROM_DIRECTORY		14
#define NCP_EXTRACT_A_BASE_HANDLE				23
#define NCP_GET_A_DIRECTORY_ENTRY				31		/* 3.0 */
#define NCP_GET_DIRECTORY_DISK_LIMITS			35		/* 3.0 */
#define NCP_GET_DIRECTORY_INFORMATION			45		/* 3.0 */
#define NCP_GET_DIRECTORY_PATH					1
#define NCP_GET_EFFECTIVE_DIRECTORY_RIGHTS		3
#define NCP_GET_EFFECTIVE_RIGHTS_BY_DIR_ENTRY	42		/* 3.0 */
#define NCP_GET_NAME_SPACE_DIRECTORY_ENTRY		48		/* 3.0 */
#define NCP_GET_OBJECT_DISK_LIMIT_ON_VOLUME		41		/* 3.0 */
#define NCP_GET_PATH_NAME_OF_A_VOL_DIR_PAIR		26
#define NCP_GET_VOLUME_INFO_WITH_HANDLE			21
#define NCP_GET_VOLUME_AND_PURGE_INFORMATION	44		/* 3.0 */
#define NCP_GET_VOLUME_NAME						6
#define NCP_GET_VOLUME_NUMBER					5
#define NCP_MODIFY_MAXIMUM_RIGHTS_MASK			4
#define NCP_OPEN_DATA_STREAM					49		/* 3.0 */
#define NCP_PURGE_ERASED_FILES					16 /* File Service */
#define NCP_PURGE_SALVAGEABLE_FILE				29		/* 3.0 */
#define NCP_RECOVER_SALVAGEABLE_FILE			28		/* 3.0 */
#define NCP_REMOVE_OBJECT_DISK_LIMIT_ON_VOLUME	34		/* 3.0 */
#define NCP_REMOVE_EXTENDED_TRUSTEE				43		/* 3.0 */
#define NCP_RENAME_DIRECTORY					15
#define NCP_RENAME_OR_MOVE						46		/* 3.0 */
#define NCP_RESTORE_AN_EXTRACTED_BASE_HANDLE	24
#define NCP_RESTORE_ERASED_FILE					17 /* File Service */
#define NCP_RETURN_NAME_SPACE_INFO				47		/* 3.0 */
#define NCP_SCAN_A_DIRECTORY					30		/* 3.0 */
#define NCP_SCAN_A_DIRECTORY_DISK_SPACE			40		/* 3.0 */
#define NCP_SCAN_OBJECT_DISK_LIMITS_ON_VOLUME	32		/* 3.0 */
#define NCP_SCAN_DIRECTORY_FOR_TRUSTEES			12
#define NCP_SCAN_DIRECTORY_INFORMATION			2
#define NCP_SCAN_TRUSTEES_FOR_FILE_DIR			38		/* 3.0 */
#define NCP_SCAN_SALVAGEABLE_FILES				27		/* 3.0 */
#define NCP_SET_DIRECTORY_DISK_LIMIT			36		/* 3.0 */
#define NCP_SET_DIRECTORY_ENTRY_INFORMATION		37		/* 3.0 */
#define NCP_SET_DIRECTORY_HANDLE				0
#define NCP_SET_DIRECTORY_INFORMATION			25



/*
** The MESSAGE_SERVICES NCP Group
*/

#define NCP_MESSAGE_SERVICES					21

	/* Message Services (message) */
#define NCP_BROADCAST_TO_CONSOLE				9
#define NCP_CHECK_PIPE_STATUS					8
#define NCP_CLOSE_MESSAGE_PIPE					7
#define NCP_DISABLE_BROADCASTS					2
#define NCP_ENABLE_BROADCASTS					3 
#define NCP_GET_BROADCAST_MESSAGE				1
#define NCP_GET_PERSONAL_MESSAGE				5
#define NCP_LOG_NETWORK_MESSAGE					13
#define NCP_OPEN_MESSAGE_PIPE					6
#define NCP_SEND_BROADCAST_MESSAGE				0
#define NCP_SEND_PERSONAL_MESSAGE				4



/*
** The PRINT_SERVICES NCP Group
*/

#define NCP_PRINT_SERVICES						17

	/* Print Services (print) */
#define NCP_CLOSE_SPOOL_FILE					1
#define NCP_CREATE_SPOOL_FILE					9
#define NCP_DELETE_SPOOL_FILE					5
#define NCP_GET_PRINTER_STATUS					6
#define NCP_GET_PRINTERS_QUEUE					10
#define NCP_SCAN_SPOOL_FILE_QUEUE				4
#define NCP_SET_SPOOL_FILE_FLAGS				2
#define NCP_SPOOL_A_DISK_FILE					3
#define NCP_WRITE_TO_SPOOL_FILE					0



/*
** The SEMAPHORE_SERVICES NCP Group
*/

#define NCP_SEMAPHORE_SERVICES					32

	/* Semaphore Synchronization Services (semaphore) */
#define NCP_CLOSE_SEMAPHORE						4
#define NCP_EXAMINE_SEMAPHORE					1
#define NCP_OPEN_SEMAPHORE						0
#define NCP_SIGNAL_SEMAPHORE					3
#define NCP_WAIT_ON_SEMAPHORE					2



/*
** The TRANSACTION_TRACKING_SERVICES NCP Group
*/

#define NCP_TRANSACTION_TRACKING_SERVICES		34

	/* Transaction Tracking Services (tts) */
#define NCP_TTS_ABORT_TRANSACTION				3
#define NCP_TTS_BEGIN_TRANSACTION				1
#define NCP_TTS_END_TRANSACTION					2
#define NCP_TTS_GET_APPLICATION_THRESHOLDS		5
#define NCP_TTS_GET_TRANSACTION_BITS			9
#define NCP_TTS_GET_WORKSTATION_THRESHOLDS		7
#define NCP_TTS_IS_AVAILABLE					0
#define NCP_TTS_SET_APPLICATION_THRESHOLDS		6
#define NCP_TTS_SET_TRANSACTION_BITS			10
#define NCP_TTS_SET_WORKSTATION_THRESHOLDS		8
#define NCP_TTS_TRANSACTION_STATUS				4


/*
** The Portable NetWare NCP Group
*/
#define NCP_PORTABLE_NETWARE					96

#define NCP_GET_FILE_SERVER_STATS				1
#define NCP_GET_FILE_SERVER_TTS					2
#define NCP_GET_FILE_SERVER_SYNC				3
#define NCP_GET_SHARED_MEMORY_INFO				4

/*
** The NCP's not belonging to a function code group
*/

	/* Misc. Directory Services (base/dir-misc) */
#define NCP_GET_VOLUME_INFO_WITH_NUMBER			18

	/* Connection Services (base/connect) */
#define NCP_END_OF_JOB							24 
#define NCP_LOGOUT								25 
#ifndef NCP_NEGOTIATE_BUFFER_SIZE
#define NCP_NEGOTIATE_BUFFER_SIZE				33 
#endif

	/* File Services (base/file) */
#define NCP_ALLOW_TASK_TO_ACCESS_FILE			78
#define NCP_CLOSE_FILE							66 
#define NCP_COMMIT_FILE							61
#define NCP_SERVER_FILE_COPY					74
#define NCP_CREATE_FILE							67
#define NCP_CREATE_NEW_FILE						77 
#define NCP_ERASE_FILE							68
#define NCP_FILE_SEARCH_CONTINUE				63
#define NCP_FILE_SEARCH_INITIALIZE				62
#define NCP_GET_CURRENT_SIZE_OF_FILE			71
#define NCP_GET_SPARSE_FILE_DATA_BLOCK_BIT_MAP	85		/* 3.0 */
#define NCP_OPEN_FILE							65		/* old */
#define NCP_OPEN_FILE_WITH_ACCESS_RIGHTS		76
#define NCP_READ_FILE							72
#define NCP_RENAME_FILE							69      
#define NCP_SEARCH_FOR_A_FILE					64
#define NCP_SET_FILE_ATTRIBUTES					70
#define NCP_SET_FILE_EXTENDED_ATTRIBUTES		79
#define NCP_SET_FILE_TIME_DATE_STAMP			75
#define NCP_WRITE_FILE							73

	/* Misc. File Server Services (base/server-misc) */
#define NCP_ALLOCATE_A_RESOURCE					15
#define NCP_DEALLOCATE_A_RESOURCE				16
#define NCP_GET_FILE_SERVER_DATE_AND_TIME		20

	/* Synchronization Services (base/synch) */
#define NCP_CLEAR_FILE							7
#define NCP_CLEAR_FILE_SET						8
#define NCP_CLEAR_LOGICAL_RECORD				11
#define NCP_CLEAR_LOGICAL_RECORD_SET			14
#define NCP_CLEAR_PHYSICAL_RECORD				30
#define NCP_CLEAR_PHYSICAL_RECORD_SET			31
#define NCP_LOCK_FILE_SET						4
#define NCP_LOCK_LOGICAL_RECORD_SET				10
#define NCP_LOCK_PHYSICAL_RECORD_SET			27
#define NCP_LOG_FILE							3
#define NCP_LOG_LOGICAL_RECORD					9 
#define NCP_LOG_PHYSICAL_RECORD					26
#define NCP_RELEASE_FILE						5
#define NCP_RELEASE_FILE_SET					6
#define NCP_RELEASE_LOGICAL_RECORD				12
#define NCP_RELEASE_LOGICAL_RECORD_SET			13
#define NCP_RELEASE_PHYSICAL_RECORD				28
#define NCP_RELEASE_PHYSICAL_RECORD_SET			29
