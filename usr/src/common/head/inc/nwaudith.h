/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwaudith.h	1.1"
#ifndef NWAUDITH_H
#define NWAUDITH_H

#include <npackon.h>

 /* *************************************************************************
 *
 *	(C) Unpublished Copyright Novell, Inc.  All Rights Reserved.
 *
 *	No part of this file may be duplicated, revised, translated, localized or
 *	modified in any manner or compiled, linked or uploaded or downloaded to or
 *	from any computer system without the prior written consent of Novell, Inc.
 *
 *  $release$
 *  $modname: NWAUDITH.h$
 *  $version: 1.30$
 *  $date: Wed, Sep 4, 1991$
 *
 ************************************************************************** */

 /*
 	FILE NWAUDITH.H
	System Auditing Application Interface Definitions
 */

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef LONG
#define LONG unsigned long
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define AUDIT_NUMBER_EVENT_BITS		512

#define AUDIT_RECORD_BUFFER_SIZE	2048

/*                                            11 MAR 92 */
#define AUDITING_VERSION_DATE		((1992 - 1980) << 9) + (3 << 5) + 11

/* 											  11 MAR 1992 */
#define AUDIT_FILE_VERSION_DATE  	((1992 - 1980) << 9) + (3 << 5) + 11

/* 														15 MAY 1992 	 */
#define AUDITING_DS_VERSION_DATE		((1992 - 1980) << 9) + (5 << 5) + 15

/* 														02 OCT 1992 	 */
#define AUDIT_DS_FILE_VERSION_DATE		((1992 - 1980) << 9) + (10 << 5) + 2

/* define file codes for reading audit file via NCP */
#define AUDIT_FILE_CODE				0
#define AUDIT_HISTORY_FILE_CODE		1
#define AUDIT_OLD_FILE_CODE			2

/* define the network address types - for auditcon display */
#define CHAR_STR_NET_ADDRESS_TYPE	0	/* length preceeded char string */
#define CLIB_NET_ADDRESS_TYPE		1
#define NCP_NET_ADDRESS_TYPE		2	/* xxxxxxxx:xxxxxxxxxxxx 		*/
#define AFP_NET_ADDRESS_TYPE		3	/* xxxxxxxx:xx			 		*/
#define FTAM_NET_ADDRESS_TYPE		4	/* 20 hex digits				*/

struct auditRcd
{
       WORD         eventTypeID;
	   WORD			chkWord;
	   LONG         connectionID;
       LONG         processUniqueID;
       LONG         successFailureStatusCode;
       WORD         dosDate;
       WORD         dosTime;
/* 	   BYTE			extra[0];	start of 'union EventUnion'  */
};
#define AuditRecord struct auditRcd

struct auditDSRcd
{
	   WORD			replicaNumber;
       WORD         eventTypeID;
	   LONG			recordNumber;
	   LONG			dosDateTime;
	   LONG         userID;
       LONG         processUniqueID;
       LONG         successFailureStatusCode;
/* 	   BYTE			extra[0];	start of 'union EventUnion'  */
};
#define AuditDSRecord struct auditDSRcd

typedef struct
{
       WORD         auditingVersionDate;
       WORD         auditFileVersionDate;
       LONG         auditingEnabledFlag;
       LONG         volumeAuditFileSize;
       LONG         volumeAuditHistoryFileSize;
       LONG         volumeAuditFileMaxSize;
       LONG         volumeAuditFileSizeThreshold;
       LONG         auditRecordCount;
       LONG         historyRecordCount;
} VolumeAuditStatus;

struct eventBitMap
{
       BYTE  bitMap[ AUDIT_NUMBER_EVENT_BITS / 8 ];
};
#define AuditEventBitMap struct eventBitMap

struct configHdr
{
       WORD         fileVersionDate;
       BYTE         auditFlags;
       BYTE         errMsgDelayMinutes;
       BYTE         reserved[16];
       LONG         volumeAuditFileMaxSize;
       LONG         volumeAuditFileSizeThreshold;
       LONG         auditRecordCount;
       LONG         historyRecordCount;
       LONG         spareLongs[7];
       AuditEventBitMap volumeAuditEventBitMap;
};
#define AuditConfigHdr struct configHdr

typedef struct TIMESTAMP
{
	LONG seconds;
	WORD replicaNumber;
	WORD event;
} TIMESTAMP;

/* audit flags */
#define DiscardAuditRcdsOnErrorFlag 0x01
#define ConcurrentVolAuditorAccess	0x02
#define DualLevelPasswordsActive	0x04
#define BroadcastWarningsToAllUsers 0x08
#define LevelTwoPasswordSet			0x10
/*  */

enum auditedEventIDs
{
	A_EVENT_BIND_CHG_OBJ_SECURITY	= 1,
	A_EVENT_BIND_CHG_PROP_SECURITY	= 2,
	A_EVENT_BIND_CREATE_OBJ			= 3,
	A_EVENT_BIND_CREATE_PROPERTY	= 4,
	A_EVENT_BIND_DELETE_OBJ			= 5,
	A_EVENT_BIND_DELETE_PROPERTY	= 6,
	A_EVENT_CHANGE_DATE_TIME		= 7,
	A_EVENT_CHANGE_EQUIVALENCE		= 8,
	A_EVENT_CHANGE_SECURITY_GROUP	= 9,
	A_EVENT_CLOSE_FILE				= 10,
	A_EVENT_CLOSE_BINDERY			= 11,
	A_EVENT_CREATE_FILE				= 12,
	A_EVENT_CREATE_USER				= 13,
	A_EVENT_DELETE_FILE				= 14,
	A_EVENT_DELETE_USER				= 15,
	A_EVENT_DIR_SPACE_RESTRICTIONS	= 16,
	A_EVENT_DISABLE_ACCOUNT			= 17,
	A_EVENT_DOWN_SERVER				= 18,
	A_EVENT_GRANT_TRUSTEE			= 19,
	A_EVENT_INTRUDER_LOCKOUT_CHANGE	= 20,
	A_EVENT_LOGIN_USER				= 21,
	A_EVENT_LOGIN_USER_FAILURE		= 22,
	A_EVENT_LOGOUT_USER				= 23,
	A_EVENT_NET_LOGIN				= 24,
	A_EVENT_MODIFY_ENTRY			= 25,
	A_EVENT_OPEN_BINDERY			= 26,
	A_EVENT_OPEN_FILE				= 27,
	A_EVENT_Q_ATTACH_SERVER			= 28,
	A_EVENT_Q_CREATE				= 29,
	A_EVENT_Q_CREATE_JOB			= 30,
	A_EVENT_Q_DESTROY				= 31,
	A_EVENT_Q_DETACH_SERVER			= 32,
	A_EVENT_Q_EDIT_JOB				= 33,
	A_EVENT_Q_JOB_FINISH			= 34,
	A_EVENT_Q_JOB_SERVICE			= 35,
	A_EVENT_Q_JOB_SERVICE_ABORT		= 36,
	A_EVENT_Q_REMOVE_JOB			= 37,
	A_EVENT_Q_SET_JOB_PRIORITY		= 38,
	A_EVENT_Q_SET_STATUS			= 39,
	A_EVENT_Q_START_JOB				= 40,
	A_EVENT_Q_SWAP_RIGHTS			= 41,
	A_EVENT_READ_FILE				= 42,
	A_EVENT_REMOVE_TRUSTEE			= 43,
	A_EVENT_RENAME_MOVE_FILE		= 44,
	A_EVENT_RENAME_USER				= 45,
	A_EVENT_SALVAGE_FILE			= 46,
	A_EVENT_STATION_RESTRICTIONS	= 47,
	A_EVENT_CHANGE_PASSWORD			= 48,
	A_EVENT_TERMINATE_CONNECTION	= 49,
	A_EVENT_UP_SERVER				= 50,
	A_EVENT_USER_CHANGE_PASSWORD	= 51,
	A_EVENT_USER_LOCKED				= 52,
	A_EVENT_USER_SPACE_RESTRICTIONS	= 53,
	A_EVENT_USER_UNLOCKED			= 54,
	A_EVENT_VOLUME_MOUNT			= 55,
	A_EVENT_VOLUME_DISMOUNT			= 56,
	A_EVENT_WRITE_FILE				= 57,
	AUDITING_ACTIVE_CONNECTION_RCD	= 58,
	AUDITING_ADD_AUDITOR_ACCESS		= 59,
	AUDITING_ADD_AUDIT_PROPERTY		= 60,
	AUDITING_CHANGE_AUDIT_PASSWORD	= 61,
	AUDITING_DELETE_AUDIT_PROPERTY	= 62,
	AUDITING_DISABLE_VOLUME_AUDIT	= 63,
	AUDITING_OPEN_FILE_HANDLE_RCD	= 64,
	AUDITING_ENABLE_VOLUME_AUDITING	= 65,
	AUDITING_REMOVE_AUDITOR_ACCESS	= 66,
	AUDITING_RESET_AUDIT_FILE		= 67,
	AUDITING_RESET_AUDIT_FILE2		= 68,
	AUDITING_RESET_CONFIG_FILE		= 69,
	AUDITING_WRITE_AUDIT_BIT_MAP	= 70,
	AUDITING_WRITE_AUDIT_CONFIG_HDR	= 71,
	AUDITING_NLM_ADD_RECORD			= 72,
	AUDITING_ADD_NLM_ID_RECORD		= 73,
	AUDITING_CHANGE_AUDIT_PASSWORD2	= 74,
	A_EVENT_CREATE_DIRECTORY		= 75,
	A_EVENT_DELETE_DIRECTORY		= 76,
	A_EVENT_LAST_PLUS_ONE
};

#if 0
enum auditedDSDEventIDs
{
	/*  */
	AUDITING_DISABLE_CNT_AUDIT		= 91,
	AUDITING_ENABLE_CNT_AUDITING	= 92,
	AUDITING_RESET_HISTORY_FILE		= 33,
	/*  */
	ADS_ADD_ENTRY					= 101,
	ADS_REMOVE_ENTRY				= 102,
	ADS_RENAME_OBJECT				= 103,
	ADS_MOVE_ENTRY					= 104,
	ADS_ADD_SECURITY_EQUIVALENCE	= 105,
	ADS_REMOVE_SECURITY_EQUIVALENCE	= 106,
	ADS_ADD_ACL						= 107,
	ADS_REMOVE_ACL					= 108,
	/*  */
	ADS_LAST_PLUS_ONE
};

#define ADS_EVENT_NOT_AUDITED		-1
#endif

enum auditBitMapIDs
{
	/* first 32 bit numbers reserved for dir service */
#if 0
	ADS_BIT_ADD_ENTRY = 					1, /*  first one is 1 */
	ADS_BIT_REMOVE_ENTRY = 					2,
	ADS_BIT_RENAME_OBJECT = 				3,
	ADS_BIT_MOVE_ENTRY = 					4,
	ADS_BIT_ADD_SECURITY_EQUIV =			5,
	ADS_BIT_REMOVE_SECURITY_EQUIV = 		6,
	ADS_BIT_ADD_ACL = 						7,
	ADS_BIT_REMOVE_ACL =					8,
#endif
	/*  */
	A_BIT_BIND_CHG_OBJ_SECURITY    = 32,
	A_BIT_BIND_CHG_PROP_SECURITY,
	A_BIT_BIND_CREATE_OBJ,
	A_BIT_BIND_CREATE_PROPERTY,
	A_BIT_BIND_DELETE_OBJ,
	A_BIT_BIND_DELETE_PROPERTY,
	A_BIT_CHANGE_DATE_TIME,
	A_BIT_CHANGE_EQUIVALENCE,
	A_BIT_CHANGE_SECURITY_GROUP,
	A_BIT_UCLOSE_FILE,
	A_BIT_CLOSE_BINDERY,
	A_BIT_UCREATE_FILE,
	A_BIT_CREATE_USER,
	A_BIT_UDELETE_FILE,
	A_BIT_DELETE_USER,
	A_BIT_DIR_SPACE_RESTRICTIONS,
	A_BIT_DISABLE_ACCOUNT,
	A_BIT_DOWN_SERVER,
	A_BIT_GRANT_TRUSTEE,
	A_BIT_INTRUDER_LOCKOUT_CHANGE,
	A_BIT_LOGIN_USER,
	A_BIT_LOGIN_USER_FAILURE,
	A_BIT_LOGOUT_USER,
	A_BIT_NET_LOGIN,
	A_BIT_UMODIFY_ENTRY,
	A_BIT_OPEN_BINDERY,
	A_BIT_UOPEN_FILE,
	A_BIT_UREAD_FILE,
	A_BIT_REMOVE_TRUSTEE,
	A_BIT_URENAME_MOVE_FILE,
	A_BIT_RENAME_USER,
	A_BIT_USALVAGE_FILE,
	A_BIT_STATION_RESTRICTIONS,
	A_BIT_CHANGE_PASSWORD,
	A_BIT_TERMINATE_CONNECTION,
	A_BIT_UP_SERVER,
	A_BIT_USER_CHANGE_PASSWORD,
	A_BIT_USER_LOCKED,
	A_BIT_USER_SPACE_RESTRICTIONS,
	A_BIT_USER_UNLOCKED,
	A_BIT_VOLUME_MOUNT,
	A_BIT_VOLUME_DISMOUNT,
	A_BIT_UWRITE_FILE,
	A_BIT_GOPEN_FILE,
	A_BIT_GCLOSE_FILE,
	A_BIT_GCREATE_FILE,
	A_BIT_GDELETE_FILE,
	A_BIT_GREAD_FILE,
	A_BIT_GWRITE_FILE,
	A_BIT_GRENAME_MOVE_FILE,
	A_BIT_GMODIFY_ENTRY,
	A_BIT_IOPEN_FILE,
	A_BIT_ICLOSE_FILE,
	A_BIT_ICREATE_FILE,
	A_BIT_IDELETE_FILE,
	A_BIT_IREAD_FILE,
	A_BIT_IWRITE_FILE,
	A_BIT_IRENAME_MOVE_FILE,
	A_BIT_IMODIFY_ENTRY,
	A_BIT_Q_ATTACH_SERVER,
	A_BIT_Q_CREATE,
	A_BIT_Q_CREATE_JOB,
	A_BIT_Q_DESTROY,
	A_BIT_Q_DETACH_SERVER,
	A_BIT_Q_EDIT_JOB,
	A_BIT_Q_JOB_FINISH,
	A_BIT_Q_JOB_SERVICE,
	A_BIT_Q_JOB_SERVICE_ABORT,
	A_BIT_Q_REMOVE_JOB,
	A_BIT_Q_SET_JOB_PRIORITY,
	A_BIT_Q_SET_STATUS,
	A_BIT_Q_START_JOB,
	A_BIT_Q_SWAP_RIGHTS,
	A_BIT_NLM_ADD_RECORD,
	A_BIT_NLM_ADD_ID_RECORD,
	A_BIT_CLOSE_MODIFIED_FILE,
	A_BIT_GCREATE_DIRECTORY,
	A_BIT_ICREATE_DIRECTORY,
	A_BIT_UCREATE_DIRECTORY,
	A_BIT_GDELETE_DIRECTORY,
	A_BIT_IDELETE_DIRECTORY,
	A_BIT_UDELETE_DIRECTORY
};

/* auditing error codes */
#define ERR_AUDITING_NOT_ENABLED	ERR_NO_SPOOL_SPACE
#define ERR_AUDITING_ACTIVE			166 /* OS == ERR_ALREADY_IN_USE */
#define ERR_AUDITING_RECORD_SIZE	ERR_CREDIT_LIMIT_EXCEEDED
#define ERR_AUDITING_HARD_IO_ERROR	131
#define ERR_AUDITING_NO_RIGHTS		ERR_ACCESS_DENIED
#define ERR_AUDITING_VERSION		ERR_INVALID_RESOURCE_TAG
#define ERR_AUDITING_NOT_SUPV		132

/* this is the structure defined in the OS modify.h */

struct ModifyStructure
{
       BYTE *MModifyName;
       LONG MFileAttributes;
       LONG MFileAttributesMask;
       WORD MCreateDate;
       WORD MCreateTime;
       LONG MOwnerID;
       WORD MLastArchivedDate;
       WORD MLastArchivedTime;
       LONG MLastArchivedID;
       WORD MLastUpdatedDate;           /* also last modified date and time. */
       WORD MLastUpdatedTime;
       LONG MLastUpdatedID;
       WORD MLastAccessedDate;
       WORD MInheritanceGrantMask;
       WORD MInheritanceRevokeMask;
       long MMaximumSpace;        /* in OS version this is a 32 bit int */
	   LONG MLastUpdatedInSeconds;
};
#ifndef MModifyNameBit
#define MModifyNameBit            1
#define MFileAttributesBit 2
#define      MCreateDateBit             4
#define      MCreateTimeBit             8
#define      MOwnerIDBit                0x10
#define      MLastArchivedDateBit       0x20
#define      MLastArchivedTimeBit       0x40
#define      MLastArchivedIDBit         0x80
#define      MLastUpdatedDateBit        0x100
#define      MLastUpdatedTimeBit        0x200
#define      MLastUpdatedIDBit    0x400
#define      MLastAccessedDateBit       0x800
#define      MInheritanceRestrictionMaskBit    0x1000
#define      MMaximumSpaceBit           0x2000
#endif
union EventUnion
{
       struct eventChgDate
       {
             LONG	newDosDateTime;
       } EChgDate;

       struct eventCreateUser
       {
             LONG	userID;
             BYTE	name[1];
       } ECreateUser;

       struct eventBindChgSecurity
       {
             LONG	newSecurity;
             LONG	oldSecurity;
             BYTE	name[1];
       } EBindChgSecurity;

       struct eventBindChgSecGrp
       {
             LONG	addFlag;
             BYTE	objName[1];                             /* obj name */
             BYTE	name[1];                                /* member name */
       } EBindChgSecGrp;

       struct eventBindCreateObj
       {
             LONG	objectID;
             LONG	security;
             BYTE	name[1];
       } EBindCreateObj;

       struct eventBindCreateProp
       {
             LONG	security;
             BYTE	name[1];
       } EBindCreateProp;

       struct eventBindDeleteProp
       {
             BYTE	name[1];
       } EBindDeleteProp;

       struct eventIntruderLockoutChg
       {
             BYTE	hbaa;         /* BYTE exchanged allowed attempts */
             BYTE	lbaa;
             BYTE	hbrm;         /* reset minutes */
             BYTE	lbrm;
             BYTE	hblm;         /* lock minutes */
             BYTE	lblm;
       } EILockChg;

       struct eventLogin
       {
             LONG	userID;
             BYTE	networkAddressType;
             BYTE	networkAddressLength;
             BYTE	networkAddress[1]; 	/* variable length */
             BYTE	name[1];
       } ELogin;


       struct eventChgPasswd
       {
             BYTE	name[1];      /* object or user name */
       } EChgPasswd;

       struct eventChgSecurity
       {
             LONG	newSecurity;
             LONG	oldSecurity;
             BYTE	name[1];
       } EChgSecurity;

       struct eventFDelete
       {
             LONG	nameSpace;
             BYTE	fileName[1];
       } EFDelete;

       struct eventFOpen
       {
             LONG	handle;
             LONG	rights;
             LONG	nameSpace;
             BYTE	fileName[1];
       } EFOpen;

       struct eventFClose
       {
             LONG	handle;
             LONG	modified;
       } EFClose;

       struct eventFRead
       {
             LONG	handle;
             LONG	byteCount;
             LONG	offset;
       } EFRead;

       struct eventAuditProperty
       {
             BYTE	name[1];
       } EAuditProperty;

       struct eventModify                            /* modify dir entry */
       {
             LONG	modifyBits;
             LONG	nameSpace;
             BYTE	modifyStruct[ sizeof(struct ModifyStructure) ];
             BYTE	fileName[1];
             /* the following length preceeded strings are optional
                                        as defined by the modify bits */
             BYTE	oldDosName[1];
             BYTE	newOwner[1];
             BYTE	lastArchivedBy[1];
             BYTE	lastModifiedBy[1];
       } EModify;

       struct eventQAttach
       {
             BYTE	qname[1];
       } EQAttach;

       struct eventQCreate
       {
             LONG	qType;
             BYTE	fileName[1];
       } EQCreate;

       struct eventQJobService
       {
             LONG	tType;
             BYTE	qname[1];
       } EQJobService;

       struct eventQSetStatus
       {
             LONG	status;
             BYTE	qname[1];
       } EQSetStatus;

       struct eventStationRestrictions
       {
             BYTE	name[1];
             BYTE	netAddress[1];
       } EStnRestrictions;

       struct eventTrustee
       {
             LONG	trusteeID;
             LONG	rights;
             LONG	nameSpace;
             BYTE	trusteeName[1];
             BYTE	fileName[1];
       } ETrustee;

       struct eventTrusteeSpace
       {
             LONG	spaceValue;
             BYTE	trusteeName[1];
       } ETSpace;

       struct auditingNLMAddRecord
       {
             LONG	recordTypeID;
             LONG	dataLen;
             BYTE	userName[1];
             BYTE	data[1];
       } ENLMRecord;
};

#include <npackoff.h>

#endif
