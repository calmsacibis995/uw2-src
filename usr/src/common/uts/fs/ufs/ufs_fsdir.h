/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_UFS_UFS_FSDIR_H	/* wrapper symbol for kernel use */
#define _FS_UFS_UFS_FSDIR_H	/* subject to change without notice */

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

#ident	"@(#)kern:fs/ufs/ufs_fsdir.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * The UFS header files are kept for command
 * source compatibility.  The kernel source
 * references SFS header files.
 */
#ifdef _KERNEL_HEADERS

#ifndef _FS_SFS_SFS_FSDIR_H
#include <fs/sfs/sfs_fsdir.h>
#endif

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fs/sfs_fsdir.h>

#else

#include <sys/fs/sfs_fsdir.h>

#endif /* _KERNEL_HEADERS */

/*
 * UFS specific definitions
 */

#ifdef	MAXNAMLEN
#undef	MAXNAMLEN
#endif
#define	MAXNAMLEN	SFS_MAXNAMLEN

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_UFS_UFS_FSDIR_H */
