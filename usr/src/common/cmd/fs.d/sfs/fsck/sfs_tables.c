/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */
#ident  "@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/sfs_tables.c	1.1.2.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/sfs/fsck/sfs_tables.c,v 1.1 91/02/28 17:27:12 ccs Exp $"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fs/sfs_tables.h>

/*
 * Bit patterns for identifying fragments in the block map
 * used as ((map & around) == inside)
 */
int sfs_around[] = SFS_AROUND_INIT;

int sfs_inside[] = SFS_INSIDE_INIT;

/*
 * Given a block map bit pattern, the frag tables tell whether a
 * particular size fragment is available. 
 *
 * used as:
 * if ((1 << (size - 1)) & fragtbl[fs->fs_frag][map] {
 *	at least one fragment of the indicated size is available
 * }
 *
 * These tables are used by the scanc instruction on the VAX to
 * quickly find an appropriate fragment.
 */
u_char sfs_fragtbl124[256] = SFS_TBL124_INIT;

u_char sfs_fragtbl8[256] = SFS_TBL8_INIT;

/*
 * The actual fragtbl array.
 */
u_char *sfs_fragtbl[MAXFRAG + 1] = SFS_FRAGTBL_INIT;
