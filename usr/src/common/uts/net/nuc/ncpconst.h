/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ncpconst.h	1.14"
#ifndef _NET_NUC_NCP_NCPCONST_H
#define _NET_NUC_NCP_NCPCONST_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ncpconst.h,v 2.52.2.4 1995/02/12 22:32:21 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpconst.h
 *	ABSTRACT: Manifest constants used in NCP land.
 */

#define NCP_MAJOR_VERSION					00
#define NCP_MINOR_VERSION					02

/*
 *	Field length's in NCPs
 */
#define	NCP_VOLUME_NAME_LENGTH				32 
#define TRANSPORT_ADDRESS_LENGTH			64
#define NCP_FHANDLE_LENGTH					6
#define NCP_SEARCH_COOKIE_LENGTH			9
#define NCP_MAX_TASKS						256
#define NCP_MAX_PATH_LENGTH					256
#define NCP_MAX_DOS_FILENAME_LENGTH			14
#define NCP_MAX_UNIX_FILENAME_LENGTH		255

#define NCP_MAX_NW_PATH_LENGTH				255
#define SET_USER_PREFERENCES_BITS			0x0003
#define DO_HEADER_SIGNATURE_BITS			0x0003
#define TRY_FOR_HEADER_SIGNATURES			0x0002
#define WILL_DO_HEADER_SIGNATURES			0x0001
#define SECURITY_MIS_MATCH					0x80000000

#define DO_IPX_CHECKSUM_BITS				0x000c
#define TRY_FOR_IPX_CHECKSUMS				0x0008
#define WILL_DO_IPX_CHECKSUMS				0x0004

#define DO_COMPLETE_SIGNATURE_BITS			0x0030
#define DO_ENCRYPTION_BITS					0x00c0

#define CHECKSUMMING_REQUESTED_BIT			0x01
#define SIGNATURE_REQUESTED_BIT				0x02
#define COMPLETE_SIGNATURE_REQUESTED_BIT	0x04
#define ENCRYPTION_REQUESTED_BIT			0x08
#define LARGE_INTERNET_PACKET_BIT			0x80

#define	LOCK_LOG_ONLY						0x00
#define	LOCK_EXCLUSIVE						0x01
#define	LOCK_NONEXCLUSIVE					0x03

/*
 *	Size of the constants and and additional buffer for ncp stuff
 *	that is not part of the header or constant
 */
#define NCP_HEADER_SIZE				(int)512
#define NCP_HEADER_DATA_SIZE		(int)(NCP_HEADER_SIZE - 6)
#define MAX_NEGOTIATED_BUFFER_SIZE	(uint16)4036

/*
 *	Size in bytes of parameters that are always passed
 *	in the NCP header
 */
#define NCP_HEADER_CONST_SIZE	(NCP_HEADER_SIZE - NCP_HEADER_DATA_SIZE)

/* 
 *	Possible Request Types 
 */
#define	CREATE_A_SERVICE_CONNECTION			0x1111
#define	FILE_SERVICE_REQUEST				0x2222
#define	FILE_SERVICE_RESPONSE				0x3333
#define	DESTROY_A_SERVICE_CONNECTION		0x5555
#define BIG_FILE_SERVICE_REQUEST            0x7777
#define	SERVER_BUSY							0x9999

/*
 *	Connection status bits
 */
#define CS_BAD_CONNECTION				0x01
#define	CS_NO_CONNECTIONS_AVAILABLE		0x04
#define CS_SERVER_DOWN					0x10
#define CS_BROADCAST_MESSAGE_WAITING	0x40

#define CS_PROBLEM_WITH_CONNECTION	(CS_BAD_CONNECTION |\
									CS_NO_CONNECTIONS_AVAILABLE |\
									CS_SERVER_DOWN)

/*
 *	Functions
 */
#define FNCREATE_CONNECTION		(uint8)0
#define FNDESTROY_CONNECTION	(uint8)0
#define FNLOG_FILE				(uint8)3
#define FNLOCK_FILE				(uint8)4
#define FNGET_VINFO_WITH_NUM	(uint8)18
#define FNGET_SERVER_DATE_TIME	(uint8)20
#define FNMESSAGE_SERVICES		(uint8)21
#define FNDIRECTORY_SERVICES	(uint8)22
#define FNGENERAL_SERVICES 		(uint8)23
#define	FNEND_OF_JOB			(uint8)24 
#define FNLOGOUT				(uint8)25
#define FNLOG_PHYS_RECORD		(uint8)26
#define FNLOCK_PHYS_RECORD		(uint8)27
#define FNCLEAR_PHYSICAL_RECORD	(uint8)30
#define FNCOMMIT_FILE			(uint8)59
#define	FNFILE_SEARCH_INIT		(uint8)62
#define	FNFILE_SEARCH_CONTINUE	(uint8)63
#define FNCLOSE_FILE			(uint8)66
#define FNCREATE_FILE			(uint8)67
#define FNDELETE_FILE			(uint8)68
#define FNRENAME_FILE			(uint8)69
#define FNOSET_FILE_ATTRIBUTE	(uint8)70
#define FNGET_FILE_SIZE			(uint8)71
#define FNREAD_FILE				(uint8)72
#define FNWRITE_FILE			(uint8)73
#define FNSET_FILE_TIME_DATE	(uint8)74
#define FNOPEN_FILE				(uint8)76
#define FNOSETEXT_FILE_ATTRIBUTE (uint8)79
#define FNBURST_CONNECTION_REQUEST (uint8)101
#define FNBURST_READ_FILE		(uint8)1
#define FNBURST_WRITE_FILE		(uint8)2

#define	FNNEGOTIATE_BUFFER_SIZE (uint8)33
#define FNMAX_PACKET_SIZE	(uint8)97

/*
 *	Functions available only on NetWare 3.11 and up
 */
#define FNGENERIC_ENHANCED		(uint8)87
/*
 *	Subfunctions of FNGENERIC_ENHANCED (NetWare 3.11 and up)
 */
#define SFENHANCED_OPEN_CREATE						(uint8)1
#define SFENHANCED_INITIALIZE_SEARCH				(uint8)2
#define SFENHANCED_RENAME							(uint8)4
#define SFENHANCED_OBTAIN_FILE_OR_SUBDIR_INFO		(uint8)6
#define SFENHANCED_MODIFY_FILE_OR_SUBDIR_INFO		(uint8)7
#define SFENHANCED_DELETE							(uint8)8
#define SFENHANCED_ADD_TRUSTEE_TO_FILE_OR_SUBDIR	(uint8)10
#define SFENHANCED_ALLOC_SHORT_DIRECTORY_HANDLE		(uint8)12
#define SFENHANCED_GET_NS_INFORMATION				(uint8)19
#define SFENHANCED_SEARCH_FOR_FILE_OR_SUBDIR_SET	(uint8)20
#define SFENHANCED_GET_PATH_STRING_FROM_HANDLE		(uint8)21
#define SFENHANCED_GENERATE_DIRECTORY_BASE			(uint8)22
#define SFENHANCED_GET_NAME_SPACES_LOADED_FROM_VOL	(uint8)24
#define SFENHANCED_SET_NS_INFORMATION				(uint8)25
#define SFENHANCED_SET_HUGE_NS_INFORMATION			(uint8)27
#define SFENHANCED_GET_FULL_PATH_STRING				(uint8)28
#define SFENHANCED_GET_EFFECTIVE_DIRECTORY_RIGHTS	(uint8)29

/*
 *  Modify bits used by GetNSInformation NCP 87:19
 */
#define UNIXModifyNameBit           0x0001
#define UNIXModifyFModeBit          0x0002
#define UNIXModifyNFSGroupIDBit     0x0004
#define UNIXModifyRDevBit           0x0008
#define UNIXModifyNumberOfLinksBit  0x0010
#define UNIXModifyLinkedFlagBit     0x0020
#define UNIXModifyFirstCreatedBit   0x0040
/*
#define UNIXModifyLinkNextDirNoBit  0x0080
#define UNIXModifyLinkEndDirNoBit   0x0100
#define UNIXModifyLinkPrevDirNoBit  0x0200
*/
#define UNIXModifyACSFlagsBit       0x0080
#define UNIXModifyNFSUserIDBit      0x0100
#define UNIXModifyMyFlagsBit        0x0200
/* 0x0400 is used by GetHardLinkInfoBit */
#define UNIXModifyHardLinkBit       0x0400

/*
 *	Modify bits used by ModifyFileOrSubdirectoryDOSInformation NCP 87/07
 */
#define MODIFY_NAME_BIT						0x000001
#define MODIFY_ATTRIBUTES_BIT				0x000002
#define MODIFY_CREATION_DATE_BIT			0x000004
#define MODIFY_CREATION_TIME_BIT			0x000008
#define MODIFY_CREATORS_ID_BIT				0x000010
#define MODIFY_ARCHIVED_DATE_BIT			0x000020
#define MODIFY_ARCHIVED_TIME_BIT			0x000040
#define MODIFY_ARCHIVERS_ID_BIT				0x000080
#define MODIFY_MODIFIED_DATE_BIT			0x000100
#define MODIFY_MODIFIED_TIME_BIT			0x000200
#define MODIFY_MODIFIERS_ID_BIT				0x000400
#define MODIFY_LAST_ACCESS_DATE_BIT			0x000800
#define MODIFY_INHERITANCE_BIT				0x001000
#define MODIFY_MAXIMUM_SPACE_BIT			0x002000

/*
 *	Functions available only on NetWare 3.11 and up and running the NUC
 *	support NLM or its equivalent
 */
#define FNNETWARE_UNIX_CLIENT_FUNCTION		(uint8)95
/*
 *	Subfunctions of FNNETWARE_UNIX_CLIENT_FUNCTION
 */
#define	SFNUCOpenFile						(uint8)1
#define SFNUCCreateFileOrSubdirectory		(uint8)2
#define SFNUCRenameFileOrSubdirectory		(uint8)3
#define SFNUCDeleteFileOrSubdirectory		(uint8)4
#define SFNUCLinkFile						(uint8)5
#define SFNUCGetAttributes					(uint8)6
#define SFNUCSetAttributes					(uint8)7
#define SFNUCCheckAccess					(uint8)8
#define SFNUCGetDirectoryEntries			(uint8)9
#define SFNUCReadFile						(uint8)21
#define SFNUCWriteFile						(uint8)22

#define SFUNCIsUNIXNameSpaceSupported		(uint8)0x10
#define	SFNUCOpenFile2						(uint8)0x11
#define SFNUCCreateFileOrSubdirectory2		(uint8)0x12
#define SFNUCRenameFileOrSubdirectory2		(uint8)0x13
#define SFNUCDeleteFileOrSubdirectory2		(uint8)0x14
#define SFNUCLinkFile2						(uint8)0x15
#define SFNUCGetAttributes2					(uint8)0x16
#define SFNUCSetAttributes2					(uint8)0x17
#define SFNUCCheckAccess2					(uint8)0x18
#define SFNUCGetDirectoryEntries2			(uint8)0x19

/*
 *	Message Subfunctions:
 *	FNMESSAGE_SERVICES (21)
 */
#define SFGET_BROADCAST_MESSAGE	(uint8)1
#define SFDISABLE_BROADCASTS	(uint8)2
#define SFENABLE_BROADCASTS		(uint8)3
#define SFGET_PERSONAL_MESSAGE	(uint8)5

/*
 *	Directory Services Subfunctions
 *	FNDIRECTORY_SERVICES (22)
 */
#define SFSET_DIRECTORY_HANDLE	(uint8)0
#define SFGET_DIR_HANDLE_PATH	(uint8)1
#define SFSCAN_DIR_INFO			(uint8)2
#define SFOGET_EFFECTIVE_RIGHTS	(uint8)3
#define SFGET_VOLUME_NUMBER		(uint8)5
#define SFGET_VOLUME_NAME		(uint8)6
#define SFCREATE_DIRECTORY		(uint8)10
#define SFDELETE_DIRECTORY		(uint8)11
#define SFOSCAN_DIR_TRUSTEES	(uint8)12
#define SFOADD_DIR_TRUSTEE		(uint8)13
#define SFODEL_DIR_TRUSTEE		(uint8)14
#define SFRENAME_DIRECTORY		(uint8)15
#define SFALLOC_PERM_DIR_HANDLE	(uint8)18
#define SFALLOC_TEMP_DIR_HANDLE	(uint8)19
#define SFDELETE_DIR_HANDLE		(uint8)20
#define SFSET_DIR_INFO			(uint8)25
#define SFGET_RIGHTS_FOR_ENTRY	(uint8)42
#define SFGET_VOL_AND_PURGE_INFO	(uint8)44
#define SFRENAME_OR_MOVE		(uint8)46
#define SFGET_NAMESPACE_INFO	(uint8)47

/*
 *	General Subfunctions:
 *	FNGENERAL_SERVICES (23)
 */
#define SFOSCAN_FILE_INFO		(uint8)15
#define SFOSET_FILE_INFO		(uint8)16
#define SFGET_SERVER_INFO		(uint8)17
#define SFLOGIN_OBJECT			(uint8)20
#define SFGET_LOG_KEY			(uint8)23
#define SFKEYED_LOGIN			(uint8)24
#define SFLICENSE_CONNECTION		(uint8)29
#define SFMAP_NAME_TO_ID		(uint8)53
#define SFMAP_ID_TO_NAME		(uint8)54
#define SFGET_FILE_PHYS_LOCKS	(uint8)222
#define SFGET_VOLUME_INFO		(uint8)233

/*
 *	Bindery constants / Function numbers
 */

/*
 *	Bindery object types
 */
#define NCP_MAX_OBJECT_NAME_LENGTH  48

#define BOBJ_T_USER			0x0001
#define BOBJ_T_GROUP 		0x0002
#define BOBJ_T_FILE_SERVER	0x0004

/*
 * 	NCP	File attributes
 *
 *	Consists of 4 bytes:
 *	byte 0: Standard attributes used in 2.x/3.x
 *	byte 1: Standard extended attributes used in 2.x/3.x
 *	byte 2: Extended-extended attributes used in 3.x
 * 	byte 3: Unused currently
 */
#define ATTR_NORMAL			0
#define ATTR_READ_ONLY		(1<<0)	
#define ATTR_HIDDEN			(1<<1)	
#define ATTR_SYSTEM			(1<<2)
#define ATTR_EXE_ONLY		(1<<3)
#define ATTR_DIRECTORY		(1<<4)
#define ATTR_ARCHIVE		(1<<5)
#define ATTR_UNUSED_6		(1<<6)
#define ATTR_SHARABLE		(1<<7)
#define ATTR_SMODE_1		(1<<8)
#define ATTR_SMODE_2		(1<<9)
#define ATTR_SMODE_3		(1<<10)
#define ATTR_UNUSED_11		(1<<11)
#define ATTR_TRANSACTION	(1<<12)
#define ATTR_INDEXED		(1<<13)
#define ATTR_READ_AUDIT		(1<<14)
#define ATTR_WRITE_AUDIT	(1<<15)

/*
 *	Attributes added in V3.X
 *	to make the attribute field 4 bytes
 */
#define ATTR_IMMED_PURGE	(1<<16)
#define ATTR_RENAME_INHIBIT	(1<<17)
#define ATTR_DELETE_INHIBIT	(1<<18)
#define ATTR_COPY_INHIBIT	(1<<19)

/*
 *	Search attributes used in 3.x
 */
#define SEARCH_ATTR_HIDDEN					0x0002
#define SEARCH_ATTR_SYSTEM					0x0004
#define SEARCH_ATTR_SUBDIRS_ONLY			0x0010
#define SEARCH_ATTR_ALL_FILES_AND_SUBDIRS	0x8000

/*
 *	HandleFlag definitions used in NWHandlePathStruct field HandleFlag
 */
#define SHORT_DIRECTORY_HANDLE				0x00
#define DIRECTORY_BASE						0x01
#define NO_HANDLE_PRESENT					0xff

/*
 *	Handle Type definitions used in the AllocateMode field of NCP 87:12
 */
#define PERMANENT_HANDLE					0x00
#define TEMPORARY_HANDLE					0x01
#define	SPECIAL_TEMPORARY_HANDLE			0x02

/*
 *	Open Modes used by OpenCreateFileOrDirectory NCP 87:01
 */
#define MODE_OPEN							0x01
#define MODE_REPLACE						0x02
#define MODE_CREATE							0x08

/*
 *	Return Information Mask used in 3.x
 */
#define RETURN_FILENAME						0x00000001
#define RETURN_DATA_STREAM_SPACE_ALLOCATED	0x00000002
#define RETURN_ATTRIBUTES					0x00000004
#define RETURN_DATA_STREAM_SIZE				0x00000008
#define RETURN_TOTAL_SPACE_ALLOCATED		0x00000010
#define RETURN_EXTENDED_ATTRIBUTES			0x00000020
#define RETURN_ARCHIVE_INFO					0x00000040
#define RETURN_MODIFY_INFO					0x00000080
#define RETURN_CREATION_INFO				0x00000100
#define RETURN_NAME_SPACE_INFO				0x00000200
#define RETURN_DIRECTORY_INFO				0x00000400
#define RETURN_RIGHTS_INFO					0x00000800

/*
 *	NCP Open flags
 *
 * NOTE:
 *   The following are not used with the 95 NCPs. 
 *   For other NCPs we need to use the following.
 */
#define AR_READ			(1<<0)
#define AR_WRITE		(1<<1)
#define AR_DENY_READ	(1<<2)
#define AR_DENY_WRITE	(1<<3)
#define AR_EXCLUSIVE	(1<<4)
#define AR_UNUSED_5		(1<<5)
#define AR_UNUSED_6		(1<<6)
#define AR_UNUSED_7		(1<<7)

/*
 *  rights bits
 */
#define RIGHTS_READ     0x1
#define RIGHTS_WRITE    0x2
#define RIGHTS_OPEN     0x4
#define RIGHTS_CREATE   0x8
#define RIGHTS_DELETE   0x10
#define RIGHTS_PARENTAL 0x20
#define RIGHTS_SEARCH   0x40
#define RIGHTS_MODIFY   0x80
#define RIGHTS_ALL   	0xFF


/*
 *	Error codes returned from the NCP's
 */
#define	E_SUCCESS				0x00
#define E_OUT_OF_DISK_SPACE		0x01
#define E_LOCK_FAIL				0x80
#define E_NO_FILE_HANDLES		0x81
#define E_NO_OPEN_PRIV			0x82
#define E_NO_CREATE_PRIV		0x84
#define E_NO_CREATE_DELETE		0x85
#define E_BAD_FILE_HANDLE		0x88
#define E_NO_SEARCH_PRIV		0x89
#define E_NO_DELETE_PRIV		0x8a
#define E_NO_RENAME_PRIV		0x8b
#define E_NO_MODIFY_PRIV		0x8c
#define E_FILE_INUSE			0x8d
#define E_ALL_FILES_INUSE		0x8e
#define E_ALL_READ_ONLY			0x90
#define E_SOME_NAMES_EXIST		0x91
#define E_ALL_NAMES_EXIST		0x92
#define E_NO_READ_PRIV			0x93
#define E_NO_WRITE_PRIV			0x94
#define E_SERVER_NO_MEMORY		0x96
#define E_DIRECTORY_FULL		0x99
#define E_BAD_DIR_HANDLE		0x9b
#define E_INVALID_PATH			0x9c
#define E_NO_DIR_HANDLES		0x9d
#define E_BAD_FILENAME			0x9e
#define E_DIRECTORY_NOT_EMPTY	0xa0
#define E_READ_LOCKED			0xa2
#define E_RENAME_DIR_INVALID	0xa4
#define E_UNKNOWN_REQUEST		0xfd
#define E_LOCK_COLLISION		0xfd
#define E_TIMEOUT				0xfe
#define E_FILE_EXISTS			0xfe
#define E_FAILURE				0xff
#define E_NO_FILES_FOUND		0xff
#define E_NODE_EXISTS			0xff

/*
 *	Name Space identifiers used by NetWare
 */
#define NCP_DOS_NAME_SPACE	0x00
#define NCP_MAC_NAME_SPACE	0x01
#define NCP_UNIX_NAME_SPACE	0x02
#define NCP_FTAM_NAME_SPACE	0x03
#define NCP_OS2_NAME_SPACE	0x04

#ifdef UNDEF
/*
 *	UNIX file mode (permission) bits defined by the NFS Name Space
 */
#define S_IFMT		0170000 /* type of file */
#define S_IFDIR		0040000 /* directory */
#define S_IFCHR		0020000 /* character special file */
#define S_IFBLK		0060000	/* block special file */
#define S_IFREG		0100000 /* regular */
#define S_IFLNK		0120000 /* symbolic link */
#define	S_IFSOCK	0140000	/* named socket */
#define	S_IFIFO		0010000	/* fifo */

#define S_ISDIR( m )    (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR( m )    (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK( m )    (((m) & S_IFMT) == S_IFBLK)
#define S_ISREG( m )    (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK( m )    (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK( m )   (((m) & S_IFMT) == S_IFSOCK)
#define S_ISFIFO( m )   (((m) & S_IFMT) == S_IFIFO)

/* owner permission */
#define S_IRWXU         0000700
#define S_IRUSR         0000400
#define S_IWUSR         0000200
#define S_IXUSR         0000100
#define S_IREAD         0000400
#define S_IWRITE        0000200
#define S_IEXEC         0000100

/* group permission.  same as owner's on DOS */
#define S_IRWXG         0000070
#define S_IRGRP         0000040
#define S_IWGRP         0000020
#define S_IXGRP         0000010

/* other permission.  same as owner's on DOS */
#define S_IRWXO         0000007
#define S_IROTH         0000004
#define S_IWOTH         0000002
#define S_IXOTH         0000001

/* setuid, setgid, and sticky.  always false on DOS */
#define S_ISUID         0004000
#define S_ISGID         0002000
#define S_ISVTX         0001000

#endif UNDEF

/* error bits returned by NCPspcUNIXtoDOSFileName */
#define NCP_NAME_TRUNCATED			0x01
#define NCP_INVALID_CHARACTER		0x02


#endif /* _NET_NUC_NCP_NCPCONST_H */
