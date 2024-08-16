/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_tables.c	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <util/debug.h>
#include <fs/sfs/sfs_tables.h>

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
STATIC u_char sfs_fragtbl124[] = SFS_TBL124_INIT;

STATIC u_char sfs_fragtbl8[] = SFS_TBL8_INIT;

/*
 * The actual fragtbl array.
 */
u_char *sfs_fragtbl[] = SFS_FRAGTBL_INIT;
