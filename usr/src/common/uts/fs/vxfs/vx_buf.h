/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_buf.h	2.14 18 Jul 1994 03:20:03 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_buf.h	1.8"

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

/*
 * Portions Copyright 1993, 1992, 1991 UNIX System Laboratories, Inc.
 * Portions Copyright 1990 - 1984 AT&T
 * All Rights Reserved
 */

#ifndef	_FS_VXFS_VX_BUF_H
#define	_FS_VXFS_VX_BUF_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  vx_buf.h--buffer management for vxfs file system.
 */

#define	VX_BUFWAIT	0x00	/* pseudo-flag; wait for buf if locked */
#define	VX_NOBUFWAIT	0x01	/* don't wait for locked buf */
#define	VX_INCORE	0x02	/* only get buffer if incore */
#define	VX_NONBLOCK	0x04	/* non blocking read for pageout */
#define	VX_READASYNC	0x08	/* disowning read for bitmaps */
#define	VX_NOREAD	0x10	/* get buffer and clear it */

/*
 * Macros for tracking buffer space
 */

#define	VX_BUFSPACE_ADD(space)	ATOMIC_INT_ADD(&vx_bufspace, (space))
#define	VX_BUFSPACE_SUB(space)	ATOMIC_INT_SUB(&vx_bufspace, (space))

/*
 * Keep track of buffer cache space used by VxFS for long term
 * holders.  Currently these are maps and the link count table.
 */

#define	VX_BUFHOLD(size)	{ 					\
	VX_BUFSPACE_ADD((size) >> 10);					\
	TED_ASSERT("f:bufhold:1a", 					\
	    ATOMIC_INT_READ(&vx_bufspace) >= 0 &&		/*TED_*/\
	    ATOMIC_INT_READ(&vx_bufspace) <= vx_maxbufspace &&	/*TED_*/\
	    (size) >= DEV_BSIZE && ((size) & DEV_BSIZE - 1) == 0); /*TED_*/ \
}

#define	VX_BUFRELE(size) { 						\
	VX_BUFSPACE_SUB((size) >> 10);					\
	TED_ASSERT("f:bufrele:1a", 					\
	    ATOMIC_INT_READ(&vx_bufspace) >= 0 &&		/*TED_*/\
	    ATOMIC_INT_READ(&vx_bufspace) <= vx_maxbufspace &&	/*TED_*/\
	    (size) >= DEV_BSIZE && ((size) & DEV_BSIZE - 1) == 0); /*TED_*/ \
}

#define	VX_PAGEPROC	(IS_PAGEOUT())

/*
 * Copy one buf header to another.  This is necessary for the snapshot
 * copy buffer, since simply doing a *cbp = *bp sort of thing will 
 * smash the MP locks in the buf header.
 */

#define CLONEBP(sbp, tbp) \
	bcopy(sbp, tbp, ((int)&((struct buf *)0)->b_iowait));

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_BUF_H */
