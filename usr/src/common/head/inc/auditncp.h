/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/auditncp.h	1.1"
#ifndef AUDITNCP_H
#define AUDITNCP_H

#include <npackon.h>

 /*****************************************************************************
 *
 *	(C) Copyright 1986-1993 Novell, Inc.
 *	All Rights Reserved.
 *
 *	This program is an unpublished copyrighted work which is proprietary
 *	to Novell, Inc. and contains confidential information that is not
 *	to be reproduced or disclosed to any other person or entity without
 *	prior written consent from Novell, Inc. in each and every instance.
 *
 *	WARNING:  Unauthorized reproduction of this program as well as
 *	unauthorized preparation of derivative works based upon the
 *	program or distribution of copies by sale, rental, lease or
 *	lending are violations of federal copyright laws and state trade
 *	secret laws, punishable by civil and criminal penalties.
 *
 *  $release$
 *  $modname: auditncp.h$
 *  $version: 1.22$
 *  $date: Wed, Jan 6, 1993$
 *  $nokeywords$
 *
 ****************************************************************************/

/*	FILE AUDITNCP.H
	System Auditing NCP Packet Definitions
*/

enum audit_ncp_subfunction_codes
{
	/* Handled off of OS NCP 88 */
	A_NCP_RETURN_VOL_AUDIT_STATUS = 1,	  /*  must be 1st one and = 1 */
	A_NCP_ADD_AUDIT_PROPERTY,
	A_NCP_ADD_AUDITOR_ACCESS,
	A_NCP_CHANGE_AUDIT_PASSWORD,
	A_NCP_CHECK_AUDITOR_ACCESS,
	A_NCP_DELETE_AUDIT_PROPERTY,
	A_NCP_DISABLE_VOLUME_AUDITING,
	A_NCP_ENABLE_VOLUME_AUDITING,
	A_NCP_IS_USER_AUDITED,
	A_NCP_READ_AUDIT_BIT_MAP,
	A_NCP_READ_AUDIT_CONFIG_HDR,
	A_NCP_READ_AUDITING_FILE,
	A_NCP_REMOVE_AUDITOR_ACCESS,
	A_NCP_RESET_AUDIT_FILE,
	A_NCP_RESET_HISTORY_FILE,
	A_NCP_WRITE_AUDIT_BIT_MAP,
	A_NCP_WRITE_AUDIT_CONFIG_HDR,
	A_NCP_CHANGE_AUDIT_PASSWORD2,
	A_NCP_RETURN_AUDIT_FLAGS,
	A_NCP_CLOSE_OLD_AUDIT_FILE,
	A_NCP_DELETE_OLD_AUDIT_FILE,
	A_NCP_CHECK_LEVEL_TWO_ACCESS
};

typedef struct
{
	WORD	auditingVersionDate;	/* structure must not change */
	WORD	auditFileVersionDate;	/* in these 1st 2 items */
	LONG	auditingEnabledFlag;
	LONG	volumeAuditFileSize;
	LONG	volumeAuditHistoryFileSize;
	LONG	volumeAuditFileMaxSize;
	LONG	volumeAuditFileSizeThreshold;
	LONG	auditRecordCount;
	LONG	historyRecordCount;
} VOL_AUDIT_STATUS;

struct a_pkt_hdr
{
	BYTE	subFuncReq;
	LONG	volumeNumber;
};
#define A_PKT_HDR struct a_pkt_hdr

union A_PKT_UNION
{
	struct a_pkt_pass_key
	{
		WORD	auditingVersionDate;
		BYTE	passwordKey[8];
	} A_PKT_PASS_KEY;

	struct A_PKT_ADD_AUDIT_PROPERTY
	{
		LONG	userID;
	} A_PKT_AAP;

	struct A_PKT_CHANGE_AUDIT_PASSWORD
	{
		BYTE	newPassword[16]; /* encrypted & encoded with old encrypted */
	} A_PKT_CAP;				 /* password */

	struct A_PKT_CHG_VOL_SIZE_CONFIG_INFO
	{
		LONG	auditFileMaxSize;
		LONG	auditFileSizeThreshold;
	} A_PKT_CVSCI;

	struct A_PKT_DELETE_AUDIT_PROPERTY
	{
		LONG	userID;
	} A_PKT_DAP;

 	struct A_PKT_ENABLE_VOLUME_AUDITING
	{
		WORD	auditingVersionDate;
		BYTE	cryptPassword[16];	/* encrypted */
	} A_PKT_EVA;

	struct A_PKT_IS_USER_AUDITED
	{
		LONG	userID;
	} A_PKT_IUA;

	struct A_PKT_READ_AUDITING_FILE
	{
		WORD fileCode;
		WORD byteCount;
		LONG offset;
	} A_PKT_RDAF;

	struct A_PKT_WRITE_AUDIT_BIT_MAP
	{
		AUDIT_EVENT_BIT_MAP bitMap;
	} A_PKT_WABM;

	struct A_PKT_WRITE_AUDIT_CONFIG_HDR
	{
		AUDIT_CONFIG_HDR configHdr;
	} A_PKT_WACH;

};

struct A_REPLY_READ_AUDITING_FILE
{
	LONG	byteCount;
	BYTE	bfr[1];
};
#define A_REP_RDAF struct A_REPLY_READ_AUDITING_FILE

/****************************************************************************/
/****************************************************************************/

#include <npackoff.h>

#endif
