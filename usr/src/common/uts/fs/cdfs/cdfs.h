/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifndef _FS_CDFS_CDFS_H			/* wrapper symbol for kernel use */
#define _FS_CDFS_CDFS_H			/* subject to change without notice */

#ident	"@(#)kern:fs/cdfs/cdfs.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/cdfs/cdfs_fs.h>	/* REQUIRED */
#include <fs/cdfs/cdfs_inode.h>	/* REQUIRED */
#include <fs/cdfs/cdrom.h>	/* REQUIRED */
#include <fs/cdfs/iso9660.h>	/* REQUIRED */
#include <fs/buf.h>		/* REQUIRED */
#include <fs/fbuf.h>		/* REQUIRED */
#include <mem/kmem.h>		/* REQUIRED */
#include <mem/seg.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fs/cdfs_fs.h>	/* REQUIRED */
#include <sys/fs/cdfs_inode.h>	/* REQUIRED */
#include <sys/cdrom.h>		/* REQUIRED */
#include <sys/fs/iso9660.h>	/* REQUIRED */
#include <sys/buf.h>		/* REQUIRED */
#include <sys/fbuf.h>		/* REQUIRED */
#include <sys/kmem.h>		/* REQUIRED */
#include <vm/seg.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#else

#include <sys/fs/cdfs_fs.h>	/* SVR4.2 COMPAT */
#include <sys/fs/cdfs_inode.h>	/* SVR4.2 COMPAT */
#include <sys/cdrom.h>		/* SVR4.2 COMPAT */
#include <sys/debug.h>		/* SVR4.2 COMPAT */
#include <sys/fs/iso9660.h>	/* SVR4.2 COMPAT */
#include <sys/buf.h>		/* SVR4.2 COMPAT */
#include <sys/fbuf.h>		/* SVR4.2 COMPAT */
#include <sys/kmem.h>		/* SVR4.2 COMPAT */
#include <vm/seg.h>		/* SVR4.2 COMPAT */

#endif /* _KERNEL_HEADERS */



/*
 * CDFS Global Kernel Constants
 */
extern const uchar_t			CDFS_ISO_STD_ID[];
extern const uchar_t			CDFS_HS_STD_ID[];
extern const uchar_t			CDFS_DOT[];
extern const uchar_t			CDFS_DOTDOT[];
extern const uchar_t			CDFS_POSIX_DOT[];
extern const uchar_t			CDFS_POSIX_DOTDOT[];
extern const struct cdfs_fid		CDFS_NULLFID;



/*
 * CDFS Global Kernel Variables.
 *
 * Note that 'rootvp' appears only to be used by the VM code to handle
 * the case where UNIX is running off the mini-root.  This should
 * implemented differently.
 */
extern struct vnode		*rootvp;

extern struct vfsops		cdfs_vfsops;
extern struct vnodeops		cdfs_vnodeops;

extern caddr_t			cdfs_Mem;
extern ulong_t			cdfs_MemSz;
extern uint_t			cdfs_MemCnt;

extern caddr_t			cdfs_TmpBufPool;
extern uint_t			cdfs_TmpBufSz;

extern uint_t			cdfs_InodeCnt;
extern struct cdfs_inode	*cdfs_InodeCache;
extern struct cdfs_inode	*cdfs_InodeFree;

extern uint_t			cdfs_IhashCnt;
extern struct cdfs_inode	**cdfs_InodeHash;	/* Addr of Inode pntr array	*/

extern uint_t			cdfs_DrecCnt;
extern struct cdfs_drec		*cdfs_DrecCache;
extern struct cdfs_drec		*cdfs_DrecFree;

extern uid_t			cdfs_InitialUID;
extern gid_t			cdfs_InitialGID;
extern mode_t			cdfs_InitialFPerm;
extern mode_t			cdfs_InitialDPerm;
extern int			cdfs_InitialDirSearch;
extern uint_t			cdfs_InitialNmConv;


/*
 * Return code values for internal CDFS routines.
 * Note: Return codes > 0 are defined in errno.h
 */
#define RET_OK			 0		/* Success */
#define RET_ERR			-1		/* Internal error detected */
#define RET_EOF			-2		/* End-of-file was reached */
#define RET_TRUE		-3		/* Condition is TRUE */
#define RET_FALSE		-4		/* Condition is FALSE */
#define RET_NOT_FOUND		-5		/* Item not found */
#define RET_SLEEP		-6		/* Process may have slept */



#ifdef _KERNEL

/*
 * "Buffer descriptors" that describe the information pertaining
 * to a temporary buffer.  These data structures, which are passed
 * between related routines, allow these routines to share and modify
 * the contents of a single buffer.
 *
 * These type of buffer interaction are necessary because a CDFS
 * file system stores the "Inode" information directly within the
 * directory itself.  In other file systems (e.g. s5 and ufs) the
 * directory contains an Inode ID # which is used to located the
 * Inode data in a separate area of the media.
 *
 * With a CDFS file system, all of the Inode information (except for
 * the XAR) are stored within the directory itself.  Therefore, once
 * an directory entry is located, the buffer that contains the contents
 * of a directory is used to collect the Inode information for that entry.
 */
enum cdfs_iotype {
	CDFS_FBUFIO,				/* 'struct fbuf' type I/O */
	CDFS_BUFIO				/* 'struct buf' type I/O */
};

typedef struct cdfs_iobuf {
	enum cdfs_iotype	sb_type;	/* I/O Buffer type */
	daddr_t			sb_sect;	/* Sector # of buffer contents*/
	ulong_t			sb_sectoff;	/* File offset of sector */
	uchar_t			*sb_start;	/* Kernel addr of buf contents*/
	uchar_t			*sb_end;	/* Ending kernel addr */
	uchar_t			*sb_ptr;	/* Buffer addr of roving pointer*/
	ulong_t			sb_offset;	/* File/Sect offset of roving pntr*/
	ulong_t			sb_reclen;	/* Length of current "object" */

	/*
	 * Fields used for implementing the temporary buffer scheme
	 * to transparently handle records that cross sector boundries.
	 */
	uchar_t			*sb_tmpbuf;	/* Temp buffer */
	uchar_t			*sb_split;	/* Temp buf addr of sect boundry*/
	daddr_t			sb_nextsect;	/* Sector # of next sector */
	ulong_t			sb_nextsectoff;	/* File offset of next sector */
	uchar_t			*sb_nextstart;	/* Addr of next sector contents	*/
	uchar_t			*sb_nextend;	/* Ending addr of sect contents */
	uchar_t			*sb_nextptr;	/* Sect buf pntr of new data */
	ulong_t			sb_nextoffset;	/* File/Sect offset of roving 
							pntr */

	/*
	 * CDFS_FBUFIO Type: Used for 'fbread()' type I/O.
	 */
	struct vnode		*sb_vp;		/* Vnode of file/dir	*/
	struct fbuf		*sb_fbp;	/* I/O structure	*/

	/*
	 * CDFS_BUFIO Type: Used for 'bread()' type I/O.
	 */
	dev_t			sb_dev;		/* Device # of CDFS media */
	struct buf		*sb_bp;		/* Buffer I/O structure	*/
}cdfs_iobuf_t;

/*						
 * Perhaps a 'struct clear' call would be faster.
 */
#define	CDFS_SETUP_IOBUF(tbuf, type) \
	(tbuf)->sb_type = (type);	\
	(tbuf)->sb_sect = 0;		\
	(tbuf)->sb_sectoff = 0;		\
	(tbuf)->sb_start = NULL;	\
	(tbuf)->sb_end = NULL;		\
	(tbuf)->sb_ptr = NULL;		\
	(tbuf)->sb_offset = 0;		\
	(tbuf)->sb_reclen = 0;		\
					\
	(tbuf)->sb_tmpbuf = NULL;	\
	(tbuf)->sb_split = NULL;	\
	(tbuf)->sb_nextsect = 0;	\
	(tbuf)->sb_nextsectoff = 0;	\
	(tbuf)->sb_nextstart = NULL;	\
	(tbuf)->sb_nextend = NULL;	\
	(tbuf)->sb_nextptr = NULL;	\
	(tbuf)->sb_nextoffset = 0;	\
					\
	(tbuf)->sb_vp = NULL;		\
	(tbuf)->sb_fbp = NULL;		\
	(tbuf)->sb_dev = NODEV;		\
	(tbuf)->sb_bp = NULL
	
#define CDFS_RELEASE_IOBUF(tbuf)			\
	if ((tbuf)->sb_bp != NULL) {			\
		brelse((tbuf)->sb_bp);			\
	}						\
	if ((tbuf)->sb_fbp != NULL) {			\
		fbrelse((tbuf)->sb_fbp, 0);	\
	}						\
	if ((tbuf)->sb_tmpbuf != NULL) {		\
		kmem_free((caddr_t)(tbuf)->sb_tmpbuf, cdfs_TmpBufSz); \
	}
	


/*
 * Setup the debug support structure.
 */
#ifndef CDFS_DEBUG

#define DB_CODE(x,y)

#else

#define	DB_CODE(x,y)	if (((x) & cdfs_dbflags) != 0) y

#define DB_NONE				0x00000000
#define DB_ENTER			0x00000001
#define DB_EXIT				0x00000002
#define DB_ALL				0xFFFFFFFF

extern STATIC u_int	cdfs_dbflags;	/* Flags to select desired DEBUG*/

extern void	(*cdfs_DebugPtr)();	/* CDFS Entry Point Debug Routine*/
extern void	cdfs_entry();		/* CDFS Entry Point debug stub	*/
#endif


/*
 * Prototypes for kernel functions that may not be externally referenced.
 */
int	cdfs_GetInode (struct vfs *, struct cdfs_fid *,
		struct cdfs_iobuf *, struct cdfs_inode **);
void	cdfs_CleanInode (struct cdfs_inode *, boolean_t);
int	cdfs_InitInode (struct cdfs_inode *, boolean_t);
int	cdfs_GetParent (struct vfs *, struct cdfs_inode *,
		struct cdfs_inode **, struct cred *);

int	cdfs_MergeXar (struct cdfs_inode *, struct cdfs_xar *);
int	cdfs_MergeRrip (struct cdfs_inode *, struct cdfs_rrip *);

void	cdfs_iinactive (struct cdfs_inode *);
int	cdfs_FlushInodes (struct vfs *);
int	cdfs_iaccess (struct vfs *, struct cdfs_inode *,
		mode_t, struct cred *);

void	cdfs_IputFree(struct cdfs_inode *);
void	cdfs_IrmFree(struct cdfs_inode *);
void	cdfs_IputHash(struct cdfs_inode *);
void	cdfs_IrmHash(struct cdfs_inode *);
void	cdfs_DrecPut(struct cdfs_drec **, struct cdfs_drec *);
void	cdfs_DrecRm(struct cdfs_drec **, struct cdfs_drec *);

/*
 * CDFS Directory routines.
 */
int	cdfs_DirLookup (struct vfs *, struct cdfs_inode *, uchar_t *,
		struct cdfs_inode **, struct cred *);
int	cdfs_ReadDrec (struct vfs *, struct cdfs_iobuf *);
int	cdfs_CmpDrecName (struct vfs *, struct cdfs_iobuf *,
		uchar_t *, struct pathname *);
int	cdfs_HiddenDrec (struct vfs *, struct cdfs_iobuf *);
int	cdfs_SectNum (struct vfs *, struct cdfs_inode *, ulong_t,
		daddr_t *, uint_t *, uint_t *);
int	cdfs_GetIsoName (struct vfs *, struct cdfs_iobuf *,
		struct pathname *);

int	cdfs_bmappage(struct vnode *, off_t , size_t , struct page **, 
		daddr_t	*, int);
int	cdfs_bmap_sect(struct vfs *, struct cdfs_inode *, ulong_t, daddr_t *,
		 uint_t *, uint_t *);

/*
 * CDFS Support Routines:
 */

int	cdfs_ConvertPvd (struct cdfs *, union media_pvd *, enum cdfs_type);
int	cdfs_ConvertDrec (struct cdfs_drec *, union media_drec *,
		enum cdfs_type);
int	cdfs_ConvertXar (struct cdfs_xar *, union media_xar *,
		enum cdfs_type);
int	cdfs_ConvertAdt (union media_adt *, timestruc_t *, enum cdfs_type);
int	cdfs_ConvertHdt (union media_hdt *, timestruc_t *, enum cdfs_type);
time_t	cdfs_ConvertDate (uint_t, uint_t, uint_t, uint_t, uint_t,
		uint_t, int);
struct vnode *cdfs_FindVnode (struct vfs *, struct cdfs_fid *);
int	cdfs_FlushVnodes (struct vfs *);
int	cdfs_atoi (uchar_t *, uint_t);
void	cdfs_pn_clear(struct pathname *);
int	cdfs_pn_set(struct pathname *, uchar_t *, uint_t);
int	cdfs_pn_append(struct pathname *, uchar_t *, uint_t);
int	cdfs_XcdrName (struct vfs *, uchar_t *, uint_t, struct pathname *);
dev_t	cdfs_GetDevNum(struct vfs *, struct cdfs_inode *);
uid_t	cdfs_GetUid(struct vfs *, struct cdfs_inode *);
gid_t	cdfs_GetGid(struct vfs *, struct cdfs_inode *);
mode_t	cdfs_GetPerms(struct vfs *, struct cdfs_inode *);
int	cdfs_FillBuf (struct vfs *, struct cdfs_iobuf *);
int	cdfs_ReadSect (struct vfs *, struct cdfs_iobuf *);


/*
 * RRIP procedures.
 */
int	cdfs_GetRripName (struct vfs *, struct cdfs_iobuf *, struct pathname *);
int	cdfs_LocSusp (struct vfs *, struct cdfs_iobuf *,
		struct susp_suf **, uint_t *);
int	cdfs_ReadSUF (struct vfs *, struct cdfs_iobuf *, uint_t, uint_t,
		struct susp_ce *);
int	cdfs_HiddenRrip (struct vfs *, struct cdfs_iobuf *);
int	cdfs_GetRrip (struct vfs *, struct cdfs_iobuf *, struct cdfs_rrip *);
int	cdfs_AppendRripSL (struct vfs *, struct pathname *,
		struct rrip_sl *, boolean_t *);
int	cdfs_AppendRripNM (struct pathname *, struct rrip_nm *, boolean_t *);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_CDFS_CDFS_H */
