/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/bindery.h	1.1"
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
 *    include/api/bindery.h 1.3 (Novell) 7/30/91
 */

/*
**
** TDR structure used for the bindery calls
**
*/
typedef struct binderyInfo {
	char	objName[ NWMAX_OBJECT_NAME_LENGTH ];
	char	oldObjName[ NWMAX_OBJECT_NAME_LENGTH ];
	char	newObjName[ NWMAX_OBJECT_NAME_LENGTH ];
	char	propName[ NWMAX_PROPERTY_NAME_LENGTH ];
	char	memName[ NWMAX_MEMBER_NAME_LENGTH ];
	char	oldPasswd[ NWMAX_PASSWORD_LENGTH ];
	char	newPasswd[ NWMAX_PASSWORD_LENGTH ];
	char	propValue[ NWMAX_PROPERTY_VALUE_LENGTH ];
	char	dirPath[ NWMAX_DIR_PATH_LENGTH ];
	uint8	newSecurity;
	uint8	flags;
	uint8	moreFlags;
	uint8	propFlags;
	uint8	segmentNum;
	uint8	accessLevel;
	uint8	objHasProp;	
	uint8	valueAvailable;
	uint8	volNumber;
	uint8	objNameLength;
	uint8	oldObjNameLength;
	uint8	newObjNameLength;
	uint8	propNameLength;
	uint8	memNameLength;
	uint8	oldPasswdLength;
	uint8	newPasswdLength;
	uint16	accessRights;
	uint16	objType;
	uint16	memType;
	uint32	objID;
	int32	searchInstance;
	int32	sequenceNum;
	NWObjectInfo_t		*objStruct;
	NWPropertyInfo_t	*propStruct;
} binderyInfo_t;








