/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/auditds.h	1.1"
#ifndef AUDITDS_H
#define AUDITDS_H

#include <npackoff.h>

#define DS_VOLUME 0

#define ADS_EVENT_NOT_AUDITED					-1

/* 														15 MAY 1992 	 */
#define AUDITING_DS_VERSION_DATE		((1992 - 1980) << 9) + (5 << 5) + 15

/* 														02 OCT 1992 	 */
#define AUDIT_DS_FILE_VERSION_DATE		((1992 - 1980) << 9) + (10 << 5) + 2

/* 														26 SEP 1992 	 */
#define AUDIT_DS_SKULK_VERSION_DATE		((1992 - 1980) << 9) + (9 << 5) + 26

#define MAX_DS_AUDIT_RCD_SIZE 4096

#define AUDITING_ENABLED		TRUE
#define AUDITING_CONTAINER_FULL	3

#define AUDIT_FULL_FLAG			0x80
#define AUDIT_THRESHOLD_FLAG 	0x40
#define AUDIT_WRITE_ERR_FLAG	0x20

struct audit_container_rcd_hdr
{
	WORD	replicaNumber;
	WORD	eventTypeID;
	LONG	recordNumber;
	LONG	dosDateTime;
	LONG	userID;
	LONG	processUniqueID;
	LONG	successFailureStatusCode;
};
#define AUDIT_C_RCD_HDR struct audit_container_rcd_hdr

/* *********************************************************************** */

struct replica_table_entry
{
	LONG recordNumber;
	LONG fileOffset;
	WORD replicaNumber;
};
#define REPLICA_TBL_ENTRY struct replica_table_entry

/* *********************************************************************** */

struct audit_container_control_block
{
	LONG	containerID;
	TIMESTAMP creationTS;
	LONG	bitMap;
	BYTE	auditFlags;
	BYTE	enabledFlag;
	WORD	replicaNumber;
	LONG	ccbListNdx;
	LONG	auditFileHandle;
	LONG	oldAuditFileHandle;
	LONG	auditFileSize;
	LONG	auditFileMaxSize;
	LONG	auditFileSizeThreshold;
	LONG	auditRecordCount;
	LONG	auditSemaphore;
	LONG	replicaRecordCount;
	LONG	prevConnectionNumber;
	BYTE	errMsgDelayMinutes;
	BYTE	auditMsgFlags;
	BYTE	delayMinutesRemaining;
	BYTE	skulkFlags;
	WORD	numberReplicaSlots;
	WORD	numberReplicaEntries;
	void	*replicaTable;
	LONG	auditDisabledCounter;
	LONG	auditEnabledCounter;
	LONG	hdrModifiedCounter;
	LONG	fileResetCounter;
};
#define AUDIT_CCB struct audit_container_control_block

/* skulk flags */
#define skulkEnabledFlag	1
#define skulkNeededFlag		2

/* *********************************************************************** */

struct audit_container_file_hdr
{
	WORD	fileVersionDate;
	BYTE	auditFlags;
	BYTE	errMsgDelayMinutes;
	LONG	containerID;
	LONG	spareLong0;
	TIMESTAMP creationTS;
	LONG	bitMap;
	LONG	auditFileMaxSize;
	LONG	auditFileSizeThreshold;
	LONG	auditRecordCount;
	WORD	replicaNumber;
	BYTE	enabledFlag;
	BYTE	spareBytes[3];
	WORD	numberReplicaEntries;
	LONG	spareLongs[9];
	LONG	auditDisabledCounter;
	LONG	auditEnabledCounter;
	BYTE	encryptPassword[16];
	BYTE	encryptPassword2[16];
	LONG	hdrModifiedCounter;
	LONG	fileResetCounter;
};
#define AUDIT_DS_FILE_HDR struct audit_container_file_hdr

/* *********************************************************************** */


enum audited_ds_event_ids
{
	AUDITING_DISABLE_CNT_AUDIT		= 91,
	AUDITING_ENABLE_CNT_AUDITING	= 92,
	AUDITING_RESET_HISTORY_FILE		= 93, /* unused */
	AUDITING_CLOSE_CNT_AUDITING		= 94,
	AUDITING_CHANGE_USER_AUDITED	= 95,

	/*  */
	/* Dir Service event ids are defined in dsaudit.h */
	ADS_LAST_PLUS_ONE				= 132
};

#if 0
/* Dir Service bit map ids are defined in dsaudit.h */
enum audit_ds_bit_map_ids
{
	/* first 32 (0-31) bit numbers are reserved for DirService auditing */
	ADS_BIT_ADD_ENTRY = 					1,	/* first bit no. is 1 */
	ADS_BIT_REMOVE_ENTRY = 					2,
	ADS_BIT_RENAME_OBJECT = 				3,
	ADS_BIT_MOVE_ENTRY = 					4,
	ADS_BIT_ADD_SECURITY_EQUIV =			5,
	ADS_BIT_REMOVE_SECURITY_EQUIV = 		6,
	ADS_BIT_ADD_ACL = 						7,
	ADS_BIT_REMOVE_ACL =					8,
	ADS_BIT_LOGIN =							9,
	ADS_BIT_LOGOUT	=						10,
	ADS_BIT_CHANGE_PASSWORD	=				11,
	ADS_BIT_USER_LOCKED =					12,
	ADS_BIT_USER_UNLOCKED =					13
};
#endif

enum audit_ds_ncp_subfunction_codes
{	/* Dir Service sub functions are handled off of DS NCP 104 */
	A_DSNCP_RET_CNT_AUDIT_STATUS = 200, /*  must be 1st DS one and = 200 */
	A_DSNCP_RESERVED1			= 201,
	A_DSNCP_ADD_AUDITOR_ACCESS = 202,
	A_DSNCP_CHANGE_AUDIT_PASSWORD = 203,
	A_DSNCP_CHECK_AUDITOR_ACCESS = 204,
	A_DSNCP_RESERVED2			= 205,
	A_DSNCP_DISABLE_CNT_AUDITING = 206,
	A_DSNCP_ENABLE_CNT_AUDITING = 207,
	A_DSNCP_RESERVED3			= 208,
	A_DSNCP_READ_AUDIT_FILE_HDR = 209,
	A_DSNCP_READ_AUDITING_FILE = 210,
	A_DSNCP_REMOVE_AUDITOR_ACCESS = 211,
	A_DSNCP_RESET_AUDIT_FILE = 212,
	A_DSNCP_RESET_HISTORY_FILE = 213, /* unused */
	A_DSNCP_WRITE_AUDIT_FILE_HDR = 214,
	A_DSNCP_CHANGE_AUDIT_PASSWORD2= 215,
	A_DSNCP_RETURN_AUDIT_FLAGS = 216,
	A_DSNCP_CLOSE_OLD_AUDIT_FILE = 217,
	A_DSNCP_DELETE_OLD_AUDIT_FILE = 218,
	A_DSNCP_CHECK_LEVEL_TWO_ACCESS = 219,
	A_DSNCP_CHECK_OBJECT_AUDITED =	220,
	A_DSNCP_CHANGE_OBJECT_AUDITED =	221,
	/*  */
	A_DSNCP_SKULK_CONTAINER = 		240,
	A_DSNCP_SKULK_ENABLE = 			241,
	A_DSNCP_SKULK_HDR_MODIFIED = 	242,
	A_DSNCP_SKULK_RESET = 			243,
	A_DSNCP_SKULK_REPLICA = 		244,
	A_DSNCP_SKULK_SYNC_REPLICA =	245,
	A_DSNCP_SKULK_DISABLE =			246,
	A_DSNCP_SKULK_BACK =			247,
	A_DSNCP_SKULK_RESTART =			248,
	A_DSNCP_SKULK_CONTAINER_FULL =	249
};

struct a_ds_pkt_hdr
{
	BYTE	subFuncReq;
	LONG	containerID;
};
#define A_DS_PKT_HDR struct a_ds_pkt_hdr

union A_DS_PKT_UNION
{
	struct A_DS_PKT_CHANGE_AUDIT_PASSWORD
	{
		BYTE	newPassword[16]; /* encrypted & encoded with old encrypted */
	} A_PKT_CAP;				 /* password */

 	struct A_DS_PKT_ENABLE_VOLUME_AUDITING
	{
		WORD	auditingVersionDate;
		BYTE	cryptPassword[16];	/* encrypted */
	} A_PKT_EVA;

	struct a_ds_pkt_pass_key
	{
		WORD	auditingVersionDate;
		BYTE	passwordKey[8];
	} A_PKT_PASS_KEY;

	struct A_DS_PKT_IS_USER_AUDITED
	{
		LONG	userID;
	} A_PKT_IUA;

	struct A_DS_PKT_CHG_USER_AUDITED
	{
		LONG	userID;
		LONG	auditFlag;
	} A_PKT_CUA;

	struct A_DS_PKT_READ_AUDITING_FILE
	{
		WORD fileCode;
		WORD byteCount;
		LONG offset;
	} A_PKT_RDAF;

	struct a_skulk
	{
		WORD		skulkVersionDate;
		WORD		replicaNumber;
		LONG		remoteID;
		TIMESTAMP	creationTS;
		LONG		hdrModifiedCounter;
	} A_PKT_SKULK;

	struct a_skulk_container
	{
		WORD		skulkVersionDate;
		WORD		replicaNumber;
		LONG		remoteID;
		TIMESTAMP	creationTS;
		LONG		hdrModifiedCounter;
		LONG		auditDisabledCounter;
		LONG		auditEnabledCounter;
		LONG		fileResetCounter;
	} A_PKT_SKULK_CNT;

	struct a_skulk_enable		/* enable container auditing */
	{
		WORD		skulkVersionDate;
		WORD		replicaNumber;
		LONG		remoteID;
		TIMESTAMP	creationTS;
		LONG		hdrModifiedCounter;
		AUDIT_DS_FILE_HDR aFileHdr;
	} A_PKT_SKULK_ENABLE;

	struct a_skulk_replica
	{
		WORD		skulkVersionDate;
		WORD		replicaNumber;
		LONG		remoteID;
		TIMESTAMP	creationTS;
		LONG		hdrModifiedCounter;
		LONG		startRecordNumber;
		LONG		endRecordNumber;
		LONG		size;
		BYTE		record[1];
	} A_PKT_SKULK_REPLICA;

	struct A_DS_PKT_WRITE_AUDIT_FILE_HDR
	{
		AUDIT_DS_FILE_HDR aFileHdr;
	} A_PKT_WAFH;
};

struct a_skulk_ncp_req
{
	BYTE		subFunc;
	LONG		containerID;
	WORD		skulkVersionDate;
	WORD		replicaNumber;
	LONG		remoteID;
	TIMESTAMP	creationTS;
	LONG		hdrModifiedCounter;
};

struct a_skulk_mod_counters
{
	LONG		auditDisabledCounter;
	LONG		auditEnabledCounter;
	LONG		fileResetCounter;
};

struct a_skulk_replica_records
{
	struct a_skulk_ncp_req	req;
	LONG	startRecordNumber;
	LONG	endRecordNumber;
	LONG	size;
	char	bfr[1];
};

struct a_skulk_ncp_reply
{
	WORD		skulkVersionDate;
	WORD		code;
	WORD		replicaNumber;
	LONG		containerID;
	LONG		recordNumber;
	LONG		hdrModifiedCounter;
	LONG		auditDisabledCounter;
	LONG		auditEnabledCounter;
	LONG		fileResetCounter;
};

struct audit_container_close_rcd
{
	AUDIT_C_RCD_HDR arHdr;
	LONG	startEntryIndex;
	LONG	endEntryIndex;
	BYTE	replicaTable[1];
};

#include <npackoff.h>

#endif
