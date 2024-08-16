/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsdefns.h	1.2"

#if !defined(_SMSDEFNS_H_INCLUDED)
#define _SMSDEFNS_H_INCLUDED

#if defined(DEBUG_CODE)
#define STATIC
#else
#define STATIC static
#endif

#if !defined(TRUE)
#define TRUE		1
#define FALSE		0
#endif

#if !defined(loop)
#define loop	for (;;)
#define is		==
#define isnt	!=
#define and		&&
#define or		||
#define AND		&
#define OR		|
#endif

#if !defined(NULL)
#define NULL 0L
#endif

#define UINT64_ZERO	{ 0, 0, 0, 0 }
#define UINT64_MAX	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }

// Defines for NWSM_DATA_STREAM_TYPE fid
#define NWSM_CLEAR_TEXT_DATA_STREAM			0x0
#define NWSM_SPARSE_DATA_STREAM				0x1
#define NWSM_COMPRESSED_DATA_STREAM			0x2

// Defines for NWSM_DATA_STREAM_NUMBER fid
#define NWSM_PRIMARY_DATA_STREAM_NUM		0x0
#define NWSM_MAC_RESOURCE_FORK_NUM			0x1
#define NWSM_FTAM_DATA_STREAM_NUM			0x2

// Defines for NWSM_MEDIA_MARK_TYPE fid
#define	NWSM_MEDIA_MARK_HARD				0x0	// Both file and set marks
													// are done by the hardware
#define	NWSM_MEDIA_MARK_SOFT				0x1	// Both file and set marks
													// are simulated with a
													// linked list of soft
													// media mark fids.
#define	NWSM_MEDIA_MARK_SIM_SET				0x2	// File marks are done by
													// the hardware, set marks
													// are multiple consecutive
													// file marks.

// Defines for address types of GetTargetServiceAddress
#define SPX		1
#define TCPIP	2
#define ADSP	4

#define SPX_INTERNET_ADDRESS_LENGTH      	10


// Defines for versions of compression
#define NWSM_NOVELL_COMPRESSION_V1			0x1

// Defines for NWSMTSGetUnsupportedOptions
#define NWSM_BACK_ACCESS_DATE_TIME			0x01
#define NWSM_BACK_CREATE_DATE_TIME			0x02
#define NWSM_BACK_MODIFIED_DATE_TIME		0x04
#define NWSM_BACK_ARCHIVE_DATE_TIME			0x08
#define NWSM_BACK_SKIPPED_DATA_SETS		   	0x10

#define NWSM_RESTORE_NEW_DATA_SET_NAME		0x01
#define NWSM_RESTORE_CHILD_UPDATE_MODE		0x02
#define NWSM_RESTORE_PARENT_UPDATE_MODE		0x04
#define NWSM_RESTORE_PARENT_HANDLE          0x08


/*	Generic selectionType defines */
#define NWSM_TSA_DEFINED_RESOURCE_EXC		0x02
#define NWSM_TSA_DEFINED_RESOURCE_INC		0x03
#define NWSM_PARENT_TO_BE_EXCLUDED			0x04
#define NWSM_PARENT_TO_BE_INCLUDED			0x05
#define NWSM_CHILD_TO_BE_EXCLUDED			0x08
#define NWSM_CHILD_TO_BE_INCLUDED			0x09
#define NWSM_EXCLUDE_CHILD_BY_FULL_NAME		0x10
#define NWSM_INCLUDE_CHILD_BY_FULL_NAME		0x11

/*	Generic scanType defines */
#define NWSM_DO_NOT_TRAVERSE				0x0001
#define NWSM_EXCLUDE_ARCHIVED_CHILDREN		0x0002
#define NWSM_EXCLUDE_HIDDEN_CHILDREN		0x0004
#define NWSM_EXCLUDE_HIDDEN_PARENTS			0x0008
#define NWSM_EXCLUDE_SYSTEM_CHILDREN		0x0010
#define NWSM_EXCLUDE_SYSTEM_PARENTS			0x0020

#define NWSM_EXCLUDE_CHILD_TRUSTEES			0x0040
#define NWSM_EXCLUDE_PARENT_TRUSTEES		0x0080
#define NWSM_EXCLUDE_ACCESS_DATABASE		0x0100
#define NWSM_EXCLUDE_VOLUME_RESTS			0x0200
#define NWSM_EXCLUDE_DISK_SPACE_RESTS		0x0400
#define NWSM_EXCLUDE_EXTENDED_ATTRIBUTS		0x0800
#define NWSM_EXCLUDE_DATA_STREAMS			0x1000
#define NWSM_EXCLUDE_MIGRATED_CHILD			0x2000
#define NWSM_EXPAND_COMPRESSED_DATA			0x4000

/*	Mask for numeric modes, backup and restore */
#define NWSM_OPEN_MODE_MASK				0x000F

/*	NWSMOpenDataSetForBackup modes */
#define NWSM_USE_LOCK_MODE_IF_DW_FAILS		0x0001
#define NWSM_NO_LOCK_NO_PROTECTION			0x0002
#define NWSM_OPEN_READ_ONLY					0x0003
#define NWSM_NO_DATA_STREAMS				0x0100 /* Also used for Restore */

/*	NWSMOpenDataSetForRestore modes */
#define NWSM_OVERWRITE_DATA_SET				0x0001
#define NWSM_DO_NOT_OVERWRITE_DATA_SET		0x0002
#define NWSM_CREATE_PARENT_HANDLE			0x0003
#define NWSM_UPDATE_DATA_SET				0x0004

#define NWSM_CLEAR_MODIFY_FLAG_RESTORE		0x0040
#define NWSM_RESTORE_MODIFY_FLAG			0x0080
/*	#define NWSM_NO_DATA_STREAMS				0x0100 */
#define NWSM_NO_EXTENDED_ATTRIBUTES			0x0200 
#define NWSM_NO_PARENT_TRUSTEES				0x0400
#define NWSM_NO_CHILD_TRUSTEES				0x0800
#define NWSM_NO_VOLUME_RESTRICTIONS			0x1000
#define NWSM_NO_DISK_SPACE_RESTRICTIONS		0x2000 

/*	NWSMTSSetArchiveStatus setFlag defines */
#define NWSM_CLEAR_MODIFY_FLAG				1
#define NWSM_SET_ARCHIVE_DATE_AND_TIME		2
#define NWSM_SET_ARCHIVER_ID				4

/*	Buffer Lengths */
#define NWSM_MAX_DESCRIPTION_LEN			80
#define NWSM_MAX_RESOURCE_LEN				30
#define NWSM_MAX_STRING_LEN					60
#define NWSM_MAX_TARGET_SRVC_NAME_LEN		48
#define NWSM_MAX_TARGET_SRVC_TYPE_LEN		40
#define NWSM_MAX_TARGET_SRVC_VER_LEN		10
#define NWSM_MAX_SOFTWARE_NAME_LEN			80
#define NWSM_MAX_SOFTWARE_TYPE_LEN			40
#define NWSM_MAX_SOFTWARE_VER_LEN			10
#define NWSM_MAX_TARGET_USER_NAME_LEN		256		// MAX_DN_ in nwdsdefs.h
#define NWSM_MAX_ERROR_STRING_LEN			255
#define NWSM_MAX_MM_MODULE_LABEL_LEN		64
#define NWSM_MAX_DEVICE_LABEL_LEN			64
#define NWSM_MAX_MEDIA_LABEL_LEN			64

#define EndChar(p, b)		strchr((PSTRING)(b), *LastChar(p))
#if defined(NETWARE_V320)
//		#define LastChar(p)			(NWPrevChar(p, &(p)[strlen((PSTRING)p) - 1]))
#define LastChar(p)			(NWPrevChar(p, &(p)[strlen((PSTRING)p)]))
#else
#define LastChar(p)			(&(p)[strlen((PSTRING)p) - 1])
#endif
#define StrEnd(p)			(&(p)[strlen(p)])
#define StrEqu				!strcmp
//	#define StrIEqu				!stricmp
#define StrNEqu				!strncmp
//	#define StrNIEqu			!strnicmp
#define _min(a, b)			((a) < (b)) ? (a) : (b)

#endif

