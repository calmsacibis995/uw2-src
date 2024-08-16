/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_bitmaps.h	2.6 28 Mar 1994 01:36:33 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_bitmaps.h	1.7"

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

#ifndef	_FS_VXFS_VX_BITMAPS_H
#define	_FS_VXFS_VX_BITMAPS_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _UTIL_KSYNCH_H
#include <util/ksynch.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * defines and definitions for inode, inode extended operation, and
 * extent bitmaps.
 */

/*
 * map types
 */

#define	VX_EMAP		1		/* free extent map */
#define	VX_IMAP		2		/* free inode map */
#define	VX_IEMAP	3		/* extended inode operations map */

/*
 * map clone structure
 */
struct vx_mclone {
	struct vx_mlinkhd	fmc_mlink;	/* transaction dependency q */
	struct buf		*fmc_bp;	/* cloned copy buffer */
	int			fmc_done;	/* completion status */
	int			fmc_badwrite;	/* write failed */
#ifdef	_KERNEL
	event_t			*fmc_done_evp;	/* map write done */
#endif	/* _KERNEL */
};
	
/*
 * map control structure
 * There is one of these for each map on a file system
 */

struct vx_map {
	struct vx_mlinkhd	fm_mlink;	/* transaction dependency q */
	struct vx_mlinkhd	fm_holdlink;	/* holding subfunction q */
	struct vx_mclone	*fm_clonep;	/* clone structure */
	int			fm_holdcnt;	/* num of trans holding map */
	int			fm_chgcnt;	/* number of changes to map */
	int			fm_delaycnt;	/* trans with delayed frees */
	int			fm_badwrite;	/* set for map write failure */
	int			fm_delaydone;	/* delayed mlinks are logged */
	struct vx_fsstruct	*fm_fsstruct;	/* file system pointer */
	struct vx_fset		*fm_fset;	/* file set pointer */
	struct vx_ausum		*fm_ausum;	/* au summary */
	struct buf		*fm_bp;		/* map buffer */
	char			fm_flag;	/* flags */
	char			fm_type;	/* type of map */
	int			fm_aun;		/* au number */
	int			fm_mapsz;	/* size of map */
	daddr_t			fm_fblkno;	/* first block of map */
#ifdef	_KERNEL
	sleep_t			*fm_slkp;	/* sleep lock for map */
#endif	/* _KERNEL */
};

#define	fm_fs		fm_fsstruct->fss_fsp

/*
 * values for fm_flag
 */

#define	VX_MAPBAD	0x01		/* map is bad */
#define	VX_MAPBADMSG	0x02		/* map bad message issued */

/*
 * Macros for manipulting the map lock
 */

#define	VX_MAP_LOCK(map)	SLEEP_LOCK(map->fm_slkp, PRINOD)
#define	VX_MAP_TRYLOCK(map)	(SLEEP_TRYLOCK(map->fm_slkp) == B_TRUE)
#define	VX_MAP_UNLOCK(map)	SLEEP_UNLOCK(map->fm_slkp)
  
#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_BITMAPS_H */
