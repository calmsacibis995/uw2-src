/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xnamvfsops.c	1.1"

#include <fs/vfs.h>
#include <fs/fs_subr.h>

vfsops_t xnam_vfsops = {
	(int (*)())fs_nosys,	/* mount */
	(int (*)())fs_nosys,	/* unmount */
	(int (*)())fs_nosys,	/* root */
	(int (*)())fs_nosys,	/* statvfs */
	(int (*)())fs_nosys,	/* sync */
	(int (*)())fs_nosys,	/* vget */
	(int (*)())fs_nosys,	/* mountroot */
	(int (*)())fs_nosys,	/* not used */
	(int (*)())fs_nosys,	/* filler[8] */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};
