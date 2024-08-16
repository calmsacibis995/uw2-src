/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/dswire.h	1.2"
/***************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written consent of
 * Novell, Inc.
 *
 *  $release$
 *  $modname: dswire.h$
 *  $version: 1.39$
 *  $date: Fri, Jul 1, 1994$
 *  $nokeywords$
 *
 ***************************************************************************/

#ifndef	__DSWIRE_H
#define	__DSWIRE_H

/*---------------------------------------------------------------------------
 * Directory Services NCP verb and subverbs
 */

#define DS_NCP_VERB				104

/* subverbs */
#define DS_NCP_PING						1
#define DS_NCP_FRAGMENT					2
#define DS_NCP_FRAGMENT_CLOSE			3
#define DS_NCP_BINDERY_CONTEXT		4
#define DS_NCP_MONITOR_CONNECTION	5
#define DS_NCP_GET_DS_STATISTICS		6
#define DS_NCP_RESET_DS_COUNTERS		7
#define DS_NCP_RELOAD					8
#define DS_NCP_AUDITING			200		/* 200 - 255 reserved for auditing */

/*---------------------------------------------------------------------------
 * definitions for the DS Ping request
 */
/* ping output tags */
#define DS_PING_VERSION	9		/* obsolete name */
#define DSPING_DEFAULT	9
#define DSPING_FIELDS	10

/* ping input flags */
#define DSPING_SUPPORTED_FIELDS	0x00000001L
#define DSPING_DEPTH					0x00000002L
#define DSPING_BUILD_NUMBER		0x00000004L
#define DSPING_FLAGS					0x00000008L
#define DSPING_SAP_NAME				0x00010000L
#define DSPING_TREE_NAME			0x00020000L

/* ping output flags */
#define DSPONG_ROOT_MOST_MASTER	0x0001

/*---------------------------------------------------------------------------
 */
/* NDS protocol flags */
#define DS_IS_CLIENT				0x80000000L

#define MAX_MESSAGE			   0x00010000L

#define NO_MORE_ITERATIONS		0xffffffffL

/*---------------------------------------------------------------------------
 * Directory Services Large Packet Verb Numbers
 */

#define DSV_SERVER_INTERNAL				0		/* 0x00 */
#define DSV_RESOLVE_NAME					1		/* 0x01 */
#define DSV_READ_ENTRY_INFO				2		/* 0x02 */
#define DSV_READ								3		/* 0x03 */
#define DSV_COMPARE							4		/* 0x04 */
#define DSV_LIST								5		/* 0x05 */
#define DSV_SEARCH							6		/* 0x06 */
#define DSV_ADD_ENTRY						7		/* 0x07 */
#define DSV_REMOVE_ENTRY	 				8		/* 0x08 */
#define DSV_MODIFY_ENTRY					9		/* 0x09 */
#define DSV_MODIFY_RDN						10		/* 0x0A */
#define DSV_DEFINE_ATTR						11		/* 0x0B */
#define DSV_READ_ATTR_DEF					12		/* 0x0C */
#define DSV_REMOVE_ATTR_DEF				13		/* 0x0D */
#define DSV_DEFINE_CLASS					14		/* 0x0E */
#define DSV_READ_CLASS_DEF					15		/* 0x0F */
#define DSV_MODIFY_CLASS_DEF				16		/* 0x10 */
#define DSV_REMOVE_CLASS_DEF				17		/* 0x11 */
#define DSV_LIST_CONTAINABLE_CLASSES	18		/* 0x12 */
#define DSV_GET_EFFECTIVE_RIGHTS			19		/* 0x13 */
#define DSV_OBSOLETE_ADD_PARTITION		20		/* 0x14 */
#define DSV_OBSOLETE_REMOVE_PARTITION	21		/* 0x15 */
#define DSV_LIST_PARTITIONS				22		/* 0x16 */
#define DSV_SPLIT_PARTITION				23		/* 0x17 */
#define DSV_JOIN_PARTITIONS				24		/* 0x18 */
#define DSV_ADD_REPLICA						25		/* 0x19 */
#define DSV_REMOVE_REPLICA					26		/* 0x1A */
#define DSV_OPEN_STREAM						27		/* 0x1B */
#define DSV_SEARCH_FILTER					28		/* 0x1C */
#define DSV_CREATE_SUBORDINATE_REF		29		/* 0x1D */
#define DSV_LINK_REPLICA					30		/* 0x1E */
#define DSV_CHANGE_REPLICA_TYPE			31		/* 0x1F */
#define DSV_START_UPDATE_SCHEMA 			32		/* 0x20 */
#define DSV_END_UPDATE_SCHEMA 			33		/* 0x21 */
#define DSV_UPDATE_SCHEMA					34		/* 0x22 */
#define DSV_START_UPDATE_REPLICA			35		/* 0x23 */
#define DSV_END_UPDATE_REPLICA			36		/* 0x24 */
#define DSV_UPDATE_REPLICA					37		/* 0x25 */
#define DSV_SYNC_PARTITION					38		/* 0x26 */
#define DSV_SYNC_SCHEMA						39		/* 0x27 */
#define DSV_READ_SYNTAXES					40		/* 0x28 */
#define DSV_GET_REPLICA_ROOT_ID			41		/* 0x29 */
#define DSV_BEGIN_MOVE_ENTRY				42		/* 0x2A */
#define DSV_FINISH_MOVE_ENTRY				43		/* 0x2B */
#define DSV_RELEASE_MOVED_ENTRY			44		/* 0x2C */
#define DSV_BACKUP_ENTRY					45		/* 0x2D */
#define DSV_RESTORE_ENTRY					46		/* 0x2E */
#define DSV_SAVE_DIB							47		/* 0x2F */
#define DSV_CONTROL							48		/* 0x30 */
#define DSV_UNUSED_3							49		/* 0x31 */
#define DSV_CLOSE_ITERATION				50		/* 0x32 */
#define DSV_UNUSED_4							51		/* 0x33 */
#define DSV_AUDIT_SKULKING					52		/* 0x34 */
#define DSV_GET_SERVER_ADDRESS			53		/* 0x35 */
#define DSV_SET_KEYS							54		/* 0x36 */
#define DSV_CHANGE_PASSWORD				55		/* 0x37 */
#define DSV_VERIFY_PASSWORD 				56		/* 0x38 */
#define DSV_BEGIN_LOGIN						57		/* 0x39 */
#define DSV_FINISH_LOGIN					58		/* 0x3A */
#define DSV_BEGIN_AUTHENTICATION			59		/* 0x3B */
#define DSV_FINISH_AUTHENTICATION		60		/* 0x3C */
#define DSV_LOGOUT							61		/* 0x3D */
#define DSV_OBSOLETE_REPAIR_RING			62		/* 0x3E */
#define DSV_REPAIR_TIMESTAMPS				63		/* 0x3F */
#define DSV_CREATE_BACKLINK				64		/* 0x40 */
#define DSV_DELETE_EXTERNAL_REFERENCE	65		/* 0x41 */
#define DSV_RENAME_EXTERNAL_REFERENCE	66		/* 0x42 */
#define DSV_CREATE_ENTRY_DIR				67		/* 0x43 */
#define DSV_REMOVE_ENTRY_DIR				68		/* 0x44 */
#define DSV_OBSOLETE_NEW_MASTER			69		/* 0x45 */
#define DSV_CHANGE_TREE_NAME				70		/* 0x46 */
#define DSV_PARTITION_ENTRY_COUNT		71		/* 0x47 */
#define DSV_CHECK_LOGIN_RESTRICTIONS	72		/* 0x48 */
#define DSV_START_JOIN						73		/* 0x49 */
#define DSV_LOW_LEVEL_SPLIT				74		/* 0x4A */
#define DSV_LOW_LEVEL_JOIN					75		/* 0x4B */
#define DSV_ABORT_PARTITION_OPERATION	76		/* 0x4C */
#define DSV_GET_ALL_SERVERS				77		/* 0x4D */
#define DSV_PARTITION_FUNCTION			78		/* 0x4E */
#define DSV_READ_REFERENCES				79		/* 0x4F */
#define DSV_INSPECT_ENTRY					80		/* 0x50 */
#define DSV_GET_REMOTE_ENTRY_ID			81		/* 0x51 */
#define DSV_CHANGE_SECURITY				82		/* 0x52 */
#define DSV_CHECK_CONSOLE_OPERATOR		83		/* 0x53 */
#define DSV_SET_MOVE_STATE					84		/* 0x54 */
#define DSV_LOW_LEVEL_MOVETREE			85		/* 0x55 */
#define DSV_MOVE_EXTERNAL_REFERENCE		86		/* 0x56 */
#define DSV_LOW_LEVEL_ABORT_JOIN			87		/* 0x57 */
#define DSV_CHECK_SEV						88		/* 0x58 */
#define DSV_MERGE_TREE						89		/* 0x59 */

/*---------------------------------------------------------------------------
 * entry flag definitions used by DSV_LIST, DSV_READ_ENTRY_INFO, DSV_SEARCH.
 */
#define DS_ALIAS_ENTRY				0x0001
#define DS_PARTITION_ROOT			0x0002
#define DS_CONTAINER_ENTRY			0x0004
#define DS_CONTAINER_ALIAS			0x0008
#define DS_MATCHES_LIST_FILTER	0x0010	/* only returned by DSV_LIST */

/*---------------------------------------------------------------------------
 * definitions used by DSV_LIST
 * LIST is very similar to one level SEARCH with a predefined filter. For
 * an object to be returned it must satisfy the following filter expression:
 * ClassFilter && RDNFilter && ContainerFilter && TimeFilter || AllContainerFilter
 */
#define DS_LIST_TYPELESS_OUTPUT			0x0001
#define DS_LIST_CONTAINERS					0x0002	/* only containers */
#define DS_LIST_SLASH_OUTPUT				0x0004
#define DS_LIST_FULL_DOT_OUTPUT			0x0008
#define DS_LIST_USE_DEREF_ALIAS_CLASS	0x0010
#define DS_LIST_ALL_CONTAINERS			0x0020	/* include all containers */
#define DS_LIST_OBSOLETE_DEREF_ALIASES	0x0040	/* only used versions 0, 1 */

/*---------------------------------------------------------------------------
 * definitions used by DSV_MODIFY_ENTRY
 */
/* union tags */
#define DS_ADD_ATTRIBUTE		0x00	/* add first value of attribute, error if it already exists */
#define DS_REMOVE_ATTRIBUTE	0x01	/* remove all values, error if attribute does not exist */
#define DS_ADD_VALUE				0x02	/* add first or additional value, error if duplicate */
#define DS_REMOVE_VALUE			0x03	/* remove a value, error if it does not exist */
#define DS_ADDITIONAL_VALUE	0x04	/* add additional value, error if duplicate or first */
#define DS_OVERWRITE_VALUE		0x05	/* add first or additional value, overwrite if duplicate */
#define DS_CLEAR_ATTRIBUTE		0x06	/* remove all values, no error if attribute does not exists */
#define DS_CLEAR_VALUE			0x07	/* remove value, no error if value does not exists */

/*---------------------------------------------------------------------------
 * definitions used by DSV_READ, DSV_SEARCH
 */
/* info types and union tags */
#define DS_ATTRIBUTE_NAMES			0x00
#define DS_ATTRIBUTE_VALUES		0x01
#define DS_EFFECTIVE_PRIVILEGES	0x02
#define DS_VALUE_INFO				0x03
#define DS_ABBREVIATED_VALUE		0x04

/*---------------------------------------------------------------------------
 * definitions used by DSV_READ for value flags
 */
#define DS_NAMING				0x0001
#define DS_BASECLASS			0x0002
#define DS_PRESENT			0x0004

/*---------------------------------------------------------------------------
 * definitions used by DSV_READ_ATTR_DEF
 */
#define DS_SINGLE_VALUED_ATTR	0x0001	/* also used by DSV_DEFINE_ATTR */
#define DS_SIZED_ATTR			0x0002	/* also used by DSV_DEFINE_ATTR */
#define DS_NONREMOVABLE_ATTR	0x0004
#define DS_READ_ONLY_ATTR		0x0008
#define DS_HIDDEN_ATTR			0x0010
#define DS_STRING_ATTR			0x0020
#define DS_SYNC_IMMEDIATE		0x0040	/* also used by DSV_DEFINE_ATTR */
#define DS_PUBLIC_READ			0x0080	/* also used by DSV_DEFINE_ATTR */
#define DS_SERVER_READ			0x0100
#define DS_WRITE_MANAGED		0x0200	/* also used by DSV_DEFINE_ATTR */
#define DS_PER_REPLICA			0x0400	/* also used by DSV_DEFINE_ATTR */

/* info types and union tags */
#define DS_ATTR_DEF_NAMES		0
#define DS_ATTR_DEFS				1

/*---------------------------------------------------------------------------
 * definitions used by DSV_DEFINE_CLASS and DSV_READ_CLASS_DEF
 */
#define DS_CONTAINER_CLASS			0x01
#define DS_EFFECTIVE_CLASS			0x02
#define DS_NONREMOVABLE_CLASS		0x04
#define DS_AMBIGUOUS_NAMING		0x08
#define DS_AMBIGUOUS_CONTAINMENT	0x10

/* info types and union tags */
#define DS_CLASS_DEF_NAMES			0
#define DS_CLASS_DEFS				1
#define DS_EXPANDED_CLASS_DEFS	2
#define DS_INFO_CLASS_DEFS			3
#define DS_FULL_CLASS_DEFS			4

/*---------------------------------------------------------------------------
 * definitions used by DSV_SEARCH
 */
#define DS_SEARCH_ENTRY					0
#define DS_SEARCH_SUBORDINATES		1
#define DS_SEARCH_SUBTREE				2

#define DS_ALIAS_REFERRAL				0
#define DS_PARTITION_REFERRAL			1

#define DS_SEARCH_ITEM					0
#define DS_SEARCH_OR						1
#define DS_SEARCH_AND					2
#define DS_SEARCH_NOT					3

#define DS_SEARCH_EQUAL					7
#define DS_SEARCH_GREATER_OR_EQUAL	8
#define DS_SEARCH_LESS_OR_EQUAL		9
#define DS_SEARCH_APPROX				10
#define DS_SEARCH_PRESENT				15
#define DS_SEARCH_RDN					16
#define DS_SEARCH_BASE_CLASS			17
#define DS_SEARCH_MODIFICATION_GE 	18
#define DS_SEARCH_VALUE_TIME_GE		19
#define DS_SEARCH_REFERENCES			20

/*---------------------------------------------------------------------------
 * bits used by OpenStream
 */

#define DS_READ_STREAM		0x00000001L
#define DS_WRITE_STREAM		0x00000002L

/*---------------------------------------------------------------------------
 * definitions used by skulking
 */
#define DS_SKULK_NEW_ENTRY				0x0001
#define DS_SKULK_PARTITION_ROOT		0x0002
#define DS_SKULK_ALIAS_ENTRY			0x0004
#define DS_SKULK_AUDITED_ENTRY		0x0008
#define DS_SKULK_CLIENT_IS_MASTER	0x0010
#define DS_SKULK_ENTRY_ID_ONLY		0x0020
#define DS_SKULK_OBITUARIES			0x0040
#define DS_SKULK_MORE					0x0080
#define DS_SKULK_NEW_EPOCH				0x0100

/*---------------------------------------------------------------------------
 * flags used by MoveSubTree
 */
#define MT_END						0x0001
#define MT_NAMES					0x0002
#define MT_REAL_IDS				0x0004
#define MT_EXTREF_IDS			0x0008
#define MT_PARTITION_OVERLAP	0x0010		/* parent & dest are same. */

/*---------------------------------------------------------------------------
 * flags used by Move Entry.
*/
#define MF_CREATION_TIME	0x00000001		/* release includes creation time */

/*---------------------------------------------------------------------------
 * definitions used by DSA_RESOLVE_NAME
 */
/* flags
 * Some combinations of the following flags are contradictory, the number
 * in the comment indicates the precedence of the bits (lower number means
 * higher precedence). If DS_ENTRY_ID, DS_CREATE_ID, or DS_DELAYED_CREATE_ID
 * is set all other bits are ignored.
 */
#define DS_ENTRY_ID					0x0001	/* 3 - get entry ID for a name on current connection */
#define DS_READABLE					0x0002	/* 5 */
#define DS_WRITABLE					0x0004	/* 6 */
#define DS_MASTER						0x0008	/* 7 */
#define DS_CREATE_ID					0x0010	/* 2 - create entryID for a name on current connection, if it doesn't already exist, always dereferences aliases */
#define DS_WALK_TREE					0x0020	/* 4 */
#define DS_DEREFERENCE_ALIASES	0x0040
#define DS_DELAYED_CREATE_ID		0x0200	/* 1 - same as DS_CREATE_ID as of v4.1 */
#define DS_EXHAUST_REPLICAS		0x0400
#define DS_ACCEPT_NOT_PRESENT		0x0800	/* internal use only */
#define DS_PREFER_REFERRALS		0x2000
#define DS_PREFER_ONLY_REFERRALS	0x4000	/* this bit has precedence over DS_PREFER_REFERRALS */

/* union tags */
#define DS_RESOLVED_OFFSET			0
#define DS_LOCAL_ENTRY_FOUND		1
#define DS_REMOTE_ENTRY_FOUND		2
#define DS_ALIAS_DEREFERENCED		3
#define DS_REFERRAL					4
#define DS_ENTRY_AND_REFERRALS	6

/* referral depth special tags */
#define DS_RESOLVING_DOWN			0xffffffffL
#define DS_RESOLVING_ACROSS		0xfffffffeL

/* output flags from DS_ENTRY_AND_REFERRALS tag */
#define DS_REF_TRUNCATED						0x0001
#define DS_REF_FILTERED_BY_TYPE				0x0002
#define DS_REF_FILTERED_BY_AVAILABILITY	0x0004
#define DS_REF_FILTERED_BY_TRANSPORT		0x0008
#define DS_REF_FILTERED_BY_VERSION			0x0010
#define DS_REF_ALIAS_DEREFERENCED			0x0020

/*---------------------------------------------------------------------------
 * flags for various APIs
 */

/* Partition Control SubFunction verbs */
#define PCF_RECEIVE_ALL_UPDATES	1
#define PCF_SEND_ALL_UPDATES		2

/* search flags */
#define DS_SEARCH_TYPELESS_OUTPUT	0x00000001L
#define DS_SEARCH_SLASH_OUTPUT		0x00000002L
#define DS_SEARCH_FULL_DOT_OUTPUT	0x00000004L
#define DS_SEARCH_ALIASES				0x00010000L

/* finish login flags */
#define DS_CHANGING_PASSWORD 			0x0001

/* restore flags */
#define DS_RESTORE_MORE					0x0001
#define DS_RESTORE_MOVING				0x0002

/* read flags */
#define DS_READ_TYPELESS_OUTPUT		0x0001
#define DS_READ_SLASH_OUTPUT			0x0002
#define DS_READ_FULL_DOT_OUTPUT		0x0004

/* read entry info flags */
#define DS_INFO_TYPELESS_OUTPUT		0x0001
#define DS_INFO_SLASH_OUTPUT			0x0002
#define DS_INFO_FULL_DOT_OUTPUT		0x0004

/* get server address flags */
#define DS_SERVER_NAME_TYPELESS			0x0001
#define DS_SERVER_NAME_SLASH_OUTPUT		0x0002
#define DS_SERVER_NAME_FULL_DOT_OUTPUT	0x0004

/* list partition flags */
#define DS_PARTITION_NAMES_TYPELESS		0x0001
#define DS_LIST_ALL_PARTITIONS			0x0002
#define DS_PARTITION_SLASH_OUTPUT		0x0004
#define DS_PARTITION_FULL_DOT_OUTPUT	0x0008

/* finish move flags */
#define DS_REMOVE_OLD_NAME_VALUES	0x0001

/* read reference flags */
#define DS_READ_REPLICA_SYNC_FORMAT	0x0002

/*---------------------------------------------------------------------------
 * flags which specify entry info output of
 * DSV_LIST, DSV_READ_ENTRY_INFO, and DSV_SEARCH.
 */
#define DSI_OUTPUT_FIELDS				0x00000001L
#define DSI_ENTRY_ID						0x00000002L
#define DSI_ENTRY_FLAGS					0x00000004L
#define DSI_SUBORDINATE_COUNT			0x00000008L
#define DSI_MODIFICATION_TIME			0x00000010L
#define DSI_MODIFICATION_TIMESTAMP	0x00000020L
#define DSI_CREATION_TIMESTAMP		0x00000040L
#define DSI_PARTITION_ROOT_ID			0x00000080L
#define DSI_PARENT_ID					0x00000100L
#define DSI_REVISION_COUNT				0x00000200L
#define DSI_REPLICA_TYPE				0x00000400L
#define DSI_BASE_CLASS					0x00000800L
#define DSI_ENTRY_RDN					0x00001000L
#define DSI_ENTRY_DN						0x00002000L
#define DSI_PARTITION_ROOT_DN			0x00004000L
#define DSI_PARENT_DN					0x00008000L
#define DSI_PURGE_TIME					0x00010000L
#define DSI_DEREFERENCE_BASE_CLASS	0x00020000L

/*---------------------------------------------------------------------------
 * flags which specify partition info output of
 * DSV_LIST_PARTITIONS
 */
#define DSP_OUTPUT_FIELDS				0x00000001L
#define DSP_PARTITION_ID				0x00000002L
#define DSP_REPLICA_STATE				0x00000004L
#define DSP_MODIFICATION_TIMESTAMP	0x00000008L
#define DSP_PURGE_TIME					0x00000010L
#define DSP_LOCAL_PARTITION_ID  		0x00000020L
#define DSP_PARTITION_DN				0x00000040L
#define DSP_REPLICA_TYPE				0x00000080L
#define DSP_PARTITION_BUSY				0x00000100L

/*---------------------------------------------------------------------------
 * flags which specify partition info output of
 * DS_NCP_GET_DS_STATISTICS
 */
#define DSS_OUTPUT_FIELDS				0x00000001L
#define DSS_NO_SUCH_ENTRY 				0x00000002L
#define DSS_LOCAL_ENTRY					0x00000004L
#define DSS_TYPE_REFERRAL				0x00000008L
#define DSS_ALIAS_REFERRAL 			0x00000010L
#define DSS_REQUEST_COUNT				0x00000020L
#define DSS_REQUEST_DATA_SIZE			0x00000040L
#define DSS_REPLY_DATA_SIZE			0x00000080L
#define DSS_RESET_TIME					0x00000100L
#define DSS_TRANSPORT_REFERRAL		0x00000200L
#define DSS_UP_REFERRAL					0x00000400L
#define DSS_DOWN_REFERRAL				0x00000800L

/*===========================================================================*/
#endif
