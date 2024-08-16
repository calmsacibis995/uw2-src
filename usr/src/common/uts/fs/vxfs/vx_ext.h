/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_ext.h	2.10 28 Mar 1994 01:37:33 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_ext.h	1.7"

/*
 * Copyright (c) 1994 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *	       RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *	       VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SANTA CLARA, CA 95054
 */

#ifndef	_FS_VXFS_VX_EXT_H
#define	_FS_VXFS_VX_EXT_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  structures and flags used for extent allocation
 */

struct vx_extalloc {
	struct vx_inode	*ext_ip;	/* inode that will contain allocation */
	int		ext_iaun;	/* starting au to search */
	daddr_t		ext_space;	/* total amount of space needed */
	daddr_t		ext_required;	/* minimum amount of space required */
	int		ext_flags;	/* flags for allocation */
	daddr_t		ext_indir;	/* addr of indir if indir data ext */
	daddr_t		ext_prevstart;	/* start of previous extent */
	long		ext_prevsize;	/* size of previous extent */
	int		ext_alloctype;	/* type of allocated extent */
	daddr_t		ext_start;	/* start of allocated extent */
	long		ext_size;	/* size of allocated extent */
};

/*
 * ext_flags for extent allocation
 */

#define	EXT_MASK	0x001f	/* mask for calls to vx_extentalloc */
#define	EXT_FIXED	0x0001	/* fixed size required */
#define	EXT_ALIGN	0x0002	/* aligned extent required */
#define	EXT_PREALLOC	0x0004	/* this is preallocation */
#define	EXT_EXTEND	0x0008	/* extension is allowed */
#define	EXT_SMALL	0x0010	/* searching for fragments is allowed */

#define	EXT_FINDFIX	0x1000	/* don't search for larger extent to breakup */
#define	EXT_PREV	0x2000	/* find nearest to previous */
#define	EXT_FORWARD	0x4000	/* search forward through au */
#define	EXT_BACKWARD	0x8000	/* search backward through au */

/*
 * paramaters governing reorganization requests
 */

#define	VX_MAX_REALLOC	5	/* max realloc rules for ext_alloc */
#define	VX_MAX_NEXTENTS	10	/* max nextents for extreorg */

/*
 * structure describing extent reorganization request for a file
 */

struct vx_extreorg {
	int		ext_op;		/* type of reorg operation */
	long		ext_fsetidx;	/* file set index */
	ino_t		ext_inum;	/* inode to reallocate */
	long		ext_igen;	/* generation count for file */
	off_t		ext_isize;	/* size in bytes of file */
	long		ext_iblocks;	/* blocks held by file */
	struct vx_realloc *ext_realloc;	/* array of rules for realloc */
	int		ext_nalloc;	/* number of valid entries in array */
	struct vx_dinode *ext_inode;	/* return area for new inode */
};

/*
 * type of reorganization operation for the ext_op field
 */

#define	VX_EXTREALLOC	1	/* do reallocation */
#define	VX_EXTSWAP	2	/* swap a data extent */
#define	VX_EXTINDSWAP1	3	/* swap an indirect address extent */
#define	VX_EXTINDSWAP2	4	/* swap a double indirect address extent */
#define	VX_EXTMAP_REORG	5	/* reorganize extent map and extents */

/*
 * A reallocation request at a particular offset
 */

struct vx_realloc {
	int	ext_cmd;	/* type of reallocation */
	int	ext_flag;	/* search direction */
	off_t	ext_offb;	/* offset to realloc from (in blocks) */
	long	ext_szb;	/* number of blocks to realloc */
	daddr_t	ext_start;	/* start of search */
	daddr_t	ext_end;	/* end of search */
	daddr_t	ext_nextents;	/* max number of extents to allocate */
};

/*
 * Reorganization command for ext_cmd field of vx_realloc
 */

#define	REORG_SEARCH	1	/* search range for extents */
#define	REORG_PREALLOC	2	/* use normal extent search */

/*
 * Paramaters governing extent allocation and extent maps
 */

#define	VX_MAXDEFEXT	2048	/* max extent allocated by default policy */
#define	VX_BACKSIZE	128	/* size to search backwards for */

#define	VX_MAPSZ	512	/* bytes per map sector */
#define	VX_MAPSHFT	9	/* log2(VX_MAPSZ) */
#define	VX_MAPBLKS	2048	/* blocks mapped by one map sector (bits / 2) */
#define	VX_MAPBLKMASK	0x7ff	/* mask for VX_MAPBLKS */
#define	VX_MAPBLKSHFT	11	/* shift for VX_MAPBLKS */
#define	VX_MAPBYTOBL	2	/* shift for bytes per sector to blocks */


/*
 * Is x a power of 2
 */

#define	VX_ISPOWER2(x)	(!((x) & ((x) - 1)))

/*
 * Offset from high order bit of the leftmost bit in a byte.
 */

#define	VX_LEFTBIT(x)	(vx_firstbit[(x)] >> 4)

/*
 * Offset from low order bit of the rightmost bit in a byte.
 */

#define	VX_RIGHTBIT(x)	(vx_firstbit[(x)] & 0xf)

/*
 * Macro that computes the log base 2 of x.
 */

#define	VX_LOG2(x)							  \
	(((unsigned)(x) < 256) ?  (7 - VX_LEFTBIT((unsigned)(x))) :	  \
       ((unsigned)(x) < 65536) ? (15 - VX_LEFTBIT((unsigned)(x) >> 8)) :  \
    ((unsigned)(x) < 16777216) ? (23 - VX_LEFTBIT((unsigned)(x) >> 16)) : \
				 (31 - VX_LEFTBIT((unsigned)(x) >> 24)))

/*
 * These flags are used as parameters to the vx_multi_alloc() routine.
 */

#define	VXMA_CONTIG	0x01	/* request contiguous allocation */
#define	VXMA_ALIGN	0x02	/* request aligned allocation */
#define	VXMA_LASTWRITE	0x04	/* use last write allocation semantics */
#define	VXMA_ONLYWRITE	0x08	/* use only write allocation semantics */
#define	VXMA_EXACT	0x10	/* allocated space must exactly fit request */
#define	VXMA_NOCLEAR	0x20	/* don't clear space inside file */
#define	VXMA_ONETRAN	0x40	/* operation must not span transactions */

#ifdef	_KERNEL

extern char	vx_firstbit[];
extern char	vx_countbits[];

#endif

#if defined(__cplusplus)
	}
#endif

#endif	/*_FS_VXFS_VX_EXT_H*/
