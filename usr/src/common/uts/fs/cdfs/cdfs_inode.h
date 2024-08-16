/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _FS_CDFS_CDFS_INODE_H
#define _FS_CDFS_CDFS_INODE_H

#ident	"@(#)kern:fs/cdfs/cdfs_inode.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/cdfs/cdfs_susp.h>	/* REQUIRED */
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#include <fs/pathname.h>	/* REQUIRED */
#include <fs/statvfs.h>		/* REQUIRED */
#include <fs/vfs.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fs/cdfs_susp.h>	/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/pathname.h>	/* REQUIRED */
#include <sys/statvfs.h>	/* REQUIRED */
#include <sys/vfs.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.2 COMPAT */
#include <sys/pathname.h>	/* SVR4.2 COMPAT */
#include <sys/vfs.h>		/* SVR4.2 COMPAT */
#include <sys/vnode.h>		/* SVR4.2 COMPAT */
#include <sys/fs/cdfs_susp.h>	/* SVR4.2 COMPAT */
#include <sys/fs/iso9660.h>	/* SVR4.2 COMPAT */

#endif /* _KERNEL_HEADERS */


/*
 * The I-node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, each mapping,
 * and the root.  An inode is `named' by its dev/inumber pair.
 *
 * Inode Locking:
 *	There are 2 lock objects in the CDFS inode. They are
 *	referred to as the 'sleep lock', and 'spin lock'.
 *
 *	sleep lock 
 *	    It is a long term lock and may be held while blocking. A
 *	    file's global state is preserved by holding this lock.
 *	    
 *	    This lock should be acquired/released as:
 *		SLEEP_LOCK(&ip->i_splock, PRINOD)
 *		SLEEP_UNLOCK(&ip->i_splock)
 *
 *	fast spin lock (fast spin lock)
 *	    The inode fields which are updated and/or accessed frequently
 *	    are covered by this lock.
 *	    If the holder of the fast spin lock needs to block, for example,
 *	    to retrieve indirect block information from disk, the fast spin
 *	    lock is dropped. After the LWP resumes, it must re-acquire the
 *	    fast spin lock and re-verify the disk block information because
 *	    other LWPs may have run while the LWP was blocked and change
 *	    backing store information. In this case, the new backing
 *	    store information is used. This approach reduces lock
 *	    acquisition overhead and provides greater concurrency 
 *	    since getpage operations that fill holes but don't change
 *	    the file size can run in parallel.
 *
 *	    This lock should be acquired/released as:
 *		FSPIN_LOCK(&ip->i_mutex)
 *		FSPIN_UNLOCK(&ip->i_mutex)
 *
 *	    The following fields are protected by the fast spin lock:
 *		i_DirOffset	i_Flags		i_DirRec
 *		i_AccessDate 	i_DRcount	
 *
 *	    *NOTE that IFREE may only be set/cleared in i_Flags while holding
 *	     the inode table lock. If IFREE is set, than no other LWP has
 *	     a reference to the inode. To clear IFREE, the same condition
 *	     (no other references to inode) must hold true. Thus, the fast
 *	     spin lock is not necessary when setting or clearing IFREE.
 *
 *	Several of the inode members require no locking since they're
 *	invariant while an inode is referenced by at least 1 LWP. They
 *	are:
 *		i_DevNum	i_Vnode		i_number	
 *		i_vfs		i_ParentFid	i_Fid
 */

/*
 * CDFS File ID structure that uniquely identifies a specific
 * file/dir within a specific CDFS file-system.  This data
 * identifies the location of the first Directory Record
 * of the file/dir in question.
 *
 * WARNING:
 * The size of 'cdfs_fid' CAN NOT be greater than MAXFIDSZ (vfs.h)
 * and (if using NFS) NFS_FHMAXDATA (nfs.h).  Refer to
 * cdfs_fid() (cdfs_vnops.c) and makefh() (nfs_export.c) for details.
 */
typedef struct cdfs_fid {
	daddr_t		fid_SectNum;	/* Log. Sect of 1st Dir Rec	*/
	uint_t		fid_Offset;	/* Sect offset of 1st Dir Rec	*/
} cdfs_fid_t;


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Disk block address caching realted defines.
 */
#define DB_HOLE 0		/* Value used when no block allocated */



/*
 * Macro to compare two File ID structures.
 * Note: 'fid_Offset' is most likely to be different.
 */
#define CDFS_CMPFID(fid1, fid2) ( \
	((((fid1)->fid_Offset) == ((fid2)->fid_Offset)) && \
	(((fid1)->fid_SectNum) == ((fid2)->fid_SectNum))) \
	? B_TRUE : B_FALSE \
)



typedef struct cdfs_drec {
	struct cdfs_drec 	*drec_NextDR;	/* Pointer to next Dir Rec */
	struct cdfs_drec 	*drec_PrevDR;	/* Pointer to previous Dir Rec*/
	uint_t			drec_Loc;	/* Loc of media DREC (L-Sec #)*/
	uint_t			drec_Offset;	/* # bytes from L-sec start */
	uint_t			drec_Len;	/* Len of media Dir Rec (Bytes)	*/
	uint_t			drec_XarLen;	/* Len of media XAR (Log Blk) */
	daddr_t			drec_ExtLoc;	/* Location of Extent (L-Blk #)	*/
	uint_t			drec_DataLen;	/* Len of File Sec data	*/
	timestruc_t		drec_Date;	/* Recording date and time */
	uint_t			drec_Flags;	/* Flags - See below	*/
	uint_t			drec_UnitSz;	/* File Unit Size	*/
	uint_t			drec_Interleave;  /* Interleave Gap Size */
	uint_t			drec_VolSeqNum;	  /* Volume Sequence Number */
	uint_t			drec_FileIDLen;	  /* Len of File ID String */
	uint_t		 	drec_FileIDOff;	  /* Dir Rec offset of File ID*/
	uint_t		 	drec_SysUseOff;   /* Dir Rec offset of Sys Use
							 Area*/
	uint_t		 	drec_SysUseSz;	  /* Size of Sys Use Area */
} cdfs_drec_t;

/*
 * Bit definition of 'drec_Flags' field.
 */
#define	CDFS_DREC_EXIST		0x01	/* Hide the file's existence */
#define CDFS_DREC_DIR		0x02	/* Entry is a Directory	*/
#define CDFS_DREC_ASSOC		0x04	/* Entry is an Associated File	*/
#define CDFS_DREC_REC		0x08	/* File data has "Record" format*/
#define CDFS_DREC_PROTECT	0x10	/* File has valid permission data*/
#define CDFS_DREC_RESERVE	0x60	/* Reserved			*/
#define CDFS_DREC_MULTI		0x80	/* Additional Dir Records follow*/

/*
 * Define the various Record Formats for the data in a file.
 */
enum cdfs_recfmt {
	CDFS_RECFMT_NONE	= 0,		/* No record format	*/
	CDFS_RECFMT_FIXED	= 1,		/* Fixed length records	*/
	CDFS_RECFMT_VAR1	= 2,		/* Variable length records: 
							Type 1*/
	CDFS_RECFMT_VAR2	= 3		/* Variable length records:
							 Type 2*/
};

/*
 * Define the various Record Attributes for the Records of a file.
 * A record attribute defines the control characters that preceed
 * the actual record data.
 */
enum cdfs_recattr {
	CDFS_RECATTR_CRLF	= 0,	/* Begins with a CR and LF chars*/
	CDFS_RECATTR_1539	= 1,	/* 1st char conforms to ISO-1539*/
	CDFS_RECATTR_NONE	= 2	/* No leading control characters*/
};



/*
 * Common XAR structure.
 */
typedef struct cdfs_xar {
	uid_t			xar_UserID;		/* User ID	*/
	gid_t			xar_GroupID;		/* Group ID	*/
	uint_t			xar_Perms;		/* Mode/Perm flags - See below*/
	timestruc_t		xar_CreateDate;		/* File creation date/time		*/
	timestruc_t		xar_ModDate;		/* File modification date/time	*/
	timestruc_t		xar_ExpireDate;		/* File expiration date/time	*/
	timestruc_t		xar_EffectDate;		/* File Effective date/time		*/
	enum cdfs_recfmt	xar_RecFmt;		/* File record format */
	enum cdfs_recattr	xar_RecAttr;		/* File record attribute		*/
	uint_t			xar_RecLen;		/* File record length (Bytes)	*/
	uchar_t			*xar_SysID;		/* System ID string */
	uchar_t			*xar_SysUse;		/* System Use Area */
	uint_t			xar_EscSeqLen;		/* Len of Escape Sequences		*/ 
	uint_t			xar_ApplUseLen;		/* Len of Application Use Area	*/	
	uchar_t			*xar_EscSeq;		/* Escape Sequences */
	uchar_t			*xar_ApplUse;		/* Application Use Area			*/
} cdfs_xar_t;

/*
 * Bit definitions of Inode Permission field.
 * Note: All reserved bits are set to one.
 */
#define	CDFS_XAR_SYSUSER	0x0001		/* System-group Read bit */
#define CDFS_XAR_SYSGROUP	0x0004		/* System-group Execute bit */
#define CDFS_XAR_OWNREAD	0x0010		/* File-owner Read bit	*/
#define CDFS_XAR_OWNEXEC	0x0040		/* File-owner Execute bit */
#define CDFS_XAR_GROUPREAD	0x0100		/* File-group Read bit	*/
#define CDFS_XAR_GROUPEXEC	0x0400		/* File-group Execute bit */
#define CDFS_XAR_OTHERREAD	0x1000		/* Other Read bit	*/
#define CDFS_XAR_OTHEREXEC	0x4000		/* Other Execute bit	*/
#define CDFS_XAR_NONESET	0xAAAA		/* All other bits set to 1 */		



/*
 * CDFS Inode structure contains the relevent information
 * of an individual file or directory.
 */
typedef struct cdfs_inode {
	struct cdfs_inode	*i_FreeFwd;	/* Free list forward link */
	struct cdfs_inode	*i_FreeBack;	/* Free list backward link */
	struct cdfs_inode	*i_HashFwd;	/* Hash list forward link */
	struct cdfs_inode	*i_HashBack;	/* Hash list backward link */
	fspin_t  		i_mutex; 	/* spin lock - see above */
	sleep_t 		i_splock;     	/* sleep lock - see above */
	uint_t			i_Flags;	/* Inode flags - See CDFS struct*/
	cdfs_fid_t		i_Fid;		/* File ID info	*/
	cdfs_fid_t		i_ParentFid;	/* Parent's File ID info */
	uid_t			i_UserID;	/* User ID	*/
	gid_t			i_GroupID;	/* Group ID	*/
	uint_t			i_Mode;		/* File type, Mode, and Perms */
	uint_t			i_Size;		/* Total # of bytes in file */
	uint_t			i_LinkCnt;	/* # of links to file */
	dev_t			i_DevNum;	/* Device # of BLK/CHR */
	uint_t			i_DRcount;	/* # of Directory Records */
	vfs_t			*i_vfs;		/* Associated File sys */
	daddr_t			i_NextByte;	/* Next read-ahead offset (Byte)*/
	int			i_mapsz;	/* kmem_alloc'ed size	*/
	long			i_mapcnt;	/* mappings to file pages */
	struct cdfs_drec	*i_DirRec;	/* 1st link-list Dir Rec of file*/
	cdfs_xar_t		*i_Xar;		/* XAR info from last Dir Rec */
	struct cdfs_rrip	*i_Rrip;	/* RRIP info from last Dir Rec*/
	vnode_t			*i_Vnode;	/* Vnode associated with Inode*/
	timestruc_t		i_AccessDate;	/* File Access date/time */
	timestruc_t		i_ModDate;	/* File Modification date/time*/
	timestruc_t		i_CreateDate;	/* File Creation date/time */
	timestruc_t		i_ExpireDate;	/* File Expiration date/time */
	timestruc_t		i_EffectDate;	/* File Effective date/time */
	timestruc_t		i_AttrDate;	/* File Attribute Change date/time*/
	timestruc_t		i_BackupDate;	/* File Backup date/time */
	struct pathname		i_SymLink;	/* Symbolic Link pathname */
	off_t			i_DirOffset;	/* Dir offset of last ref'd entry*/
	ulong_t			i_VerCode;	/* Version code attribute */
	daddr_t			i_ReadAhead;	/* File offset of read-ahead I/O*/
	/*
	 * The following fields cause storage to be allocated for the
	 * corresponding data structures.  Since each inode will usually
	 * need each of these structures, this is a simple mechanism for
	 * getting the needed storage.  Reference to these structures should
	 * be done via the corresponding pointers allocated above.  Thus,
	 * if the storage is to be dynamically allocated, very little
	 * code needs to change.
	 */
	cdfs_drec_t	i_DirRecStorage;	/* Static storage for i_DirRec*/
	cdfs_xar_t	i_XarStorage;		/* Static storage for i_Xar */
	struct cdfs_rrip	i_RripStorage;	/* Static storage for i_Rrip */
	vnode_t		i_VnodeStorage;		/* Static storage for i_Vnode */
} cdfs_inode_t;

#endif	/* _KERNEL | _KMEMUSER*/

#if defined(_KERNEL)

/*
 * Macros to define locking/unlocking/checking/releasing
 *       of the inode's read/write sleep lock (i_rwlock).
 */
#define CDFS_SLEEP_LOCK(ip)   	SLEEP_LOCK(&(ip)->i_splock, PRINOD)
#define CDFS_SLEEP_UNLOCK(ip)   SLEEP_UNLOCK(&(ip)->i_splock)

#define CDFS_SLEEPLOCK_BLKD(ip)     SLEEP_LOCKBLKD(&(ip)->i_splock)
#define CDFS_ITRYSLEEP_LOCK(ip)      SLEEP_TRYLOCK(&(ip)->i_splock)

#define CDFS_SLEEPLOCK_RELLOCK(ip)  		 \
	ASSERT(u.u_procp != NULL); 	 	 \
        SLEEP_LOCK_RELLOCK(&(ip)->i_splock, PRINOD, &cdfs_inode_table_mutex)

/*
 * Macros to define locking/unlocking for inode's
 *      spin lock (i_mutex).
 */
#define CDFS_ILOCK(ip)          FSPIN_LOCK(&(ip)->i_mutex)
#define CDFS_IUNLOCK(ip)      	FSPIN_UNLOCK(&(ip)->i_mutex)


/*
 * Initialize an inode for first time use
 * Set all Inode fields to zero/NULL, except
 * for the fields that require special values.
 */

#define CDFS_INITINODE_LOCKS(ip) {	                    	\
	FSPIN_INIT(&(ip)->i_mutex);				\
        SLEEP_INIT(&(ip)->i_splock, (uchar_t) 0,                \
                     &cdfs_ino_splock_lkinfo, KM_SLEEP);        \
}


/*
 * Define the Inode Hashing Function.  If the size of the
 * Inode Table is an even power-of-two, a fast hashing function
 * function can be used.   Otherwise, we have to use a slow one.
 */
#define	CDFS_INOHASH(fid) ( \
	((cdfs_IhashCnt & (cdfs_IhashCnt-1)) == 0) \
	? \
		(((uint_t)((fid)->fid_SectNum) + \
		(uint_t)((fid)->fid_Offset)) & (cdfs_IhashCnt-1)) \
	: \
		(((uint_t)((fid)->fid_SectNum) + \
		(uint_t)((fid)->fid_Offset)) % (cdfs_IhashCnt-1)) \
)

#define ITOV(ip)	((ip)->i_Vnode)
#define VTOI(vp)	((struct cdfs_inode *)(vp)->v_data)

/*
 * The current definition of an Inode number "wastes" many
 * potential values of an ino_t type.  Instead of using a
 * sector/offset of the Dir Rec, it may be better (i.e. a more
 * efficient use of the value range of an ino_t) to use the
 * sector # of the first Dir Rec in the directory (i.e. the DOT
 * entry) and an occurence # of the desired Dir Rec.  However,
 * keeping track of the "Dir Rec count" will require additional code.
 */
#define	CDFS_INUM(vfs, sect, offset)	\
	((ino_t)(((sect) << CDFS_SECTSHFT(vfs)) + (offset)))


#define NODEISUNLOCKED  0
#define NODEISLOCKED    1

#endif /* _KERNEL */

/*
 * Bit definitions of Inode Flag field.
 */
#define	IUPD		0x00000001	/* File has been modified */
#define	IACC		0x00000002	/* Inode access time needs update*/
#define	IMOD		0x00000004	/* Inode has been modified */
#define IFREE           0x00000008      /* inode on the free list */
#define	ISYNC		0x00000020	/* Do allocations synchronously	*/
#define	ICHG		0x00000040	/* Inode has been changed */
#define	INOACC		0x00000080	/* No access time update:getpage()*/
#define	IMODTIME	0x00000100	/* Mod time already set	*/
#define	IINACTIVE	0x00000200	/* Inode iinactive in progress*/

#define CDFS_INODE_HIDDEN	0x00010000	/* Hide file from user	*/
#define CDFS_INODE_ASSOC	0x00020000	/* Associated file type	*/

#define CDFS_INODE_PERM_OK	0x00100000	/* Inode's permission is valid*/
#define CDFS_INODE_UID_OK	0x00200000	/* Inode's User ID is valid */
#define CDFS_INODE_GID_OK	0x00400000	/* Inode's Group ID is valid */
#define CDFS_INODE_DEV_OK	0x00800000	/* Inode's Device Num is valid*/

#define CDFS_INODE_RRIP_REL	0x10000000	/* Inode data relocated via RRIP*/

/*
 * Bit definitions of Inode Mode field.
 * - File type definitions.
 * - Special mode flags.
 * - Permission flags: Can be used for each perms set (User, Group, Other)
 *   by right-shifting the appropriate amount:
 *   (User = Bits 6-8), (Group = Bits 3-5), (Other = Bits 0-2)
 */
#define	IFMT		0170000			/* File type mask	*/
#define	IFIFO		0010000			/* Named pipe type file	*/
#define	IFCHR		0020000			/* Character special 	*/
#define	IFDIR		0040000			/* Directory		*/
#define	IFBLK		0060000			/* Block special	*/
#define	IFREG		0100000			/* Regular		*/
#define	IFLNK		0120000			/* Symbolic link	*/
#define	IFSOCK		0140000			/* Socket		*/

/*
 * File modes
 */
#define	ISUID		0004000			/* Set User ID on execution */
#define	ISGID		0002000			/* set Group ID on execution */
#define	ISVTX		0001000			/* Save swapped text after use*/

/*
 * Permissions
 */
#define	IREAD		0000400		 	/* Read, Write, Execute perms */
#define	IWRITE		0000200
#define	IEXEC		0000100

#define IUSER_SHIFT	0
#define IGROUP_SHIFT	3
#define IOTHER_SHIFT	6

#define IREAD_USER	(IREAD >> IUSER_SHIFT)
#define IWRITE_USER	(IWRITE >> IUSER_SHIFT)
#define IEXEC_USER		(IEXEC >> IUSER_SHIFT)
#define IREAD_GROUP	(IREAD >> IGROUP_SHIFT)
#define IWRITE_GROUP	(IWRITE >> IGROUP_SHIFT)
#define IEXEC_GROUP		(IEXEC >> IGROUP_SHIFT)
#define IREAD_OTHER	(IREAD >> IOTHER_SHIFT)
#define IWRITE_OTHER	(IWRITE >> IOTHER_SHIFT)
#define IEXEC_OTHER		(IEXEC >> IOTHER_SHIFT)

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_CDFS_CDFS_INODE_H */
