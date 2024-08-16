/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIX System Laboratories, Inc.	*/
/*	(a wholly-owned subsidiary of Novell, Inc.).     	*/
/*	All Rights Reserved.                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIX SYSTEM     	*/
/*	LABORATORIES, INC. (A WHOLLY-OWNED SUBSIDIARY OF NOVELL, INC.).	*/
/*	The copyright notice above does not evidence any actual or     	*/
/*	intended publication of such source code.                      	*/

#ifndef _MEMFS_MKROOT_H	/* wrapper symbol for kernel use */
#define _MEMFS_MKROOT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/memfs/memfs_mkroot.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#include <util/ksynch.h> /* REQUIRED */
#include <util/param.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */
#include <fs/vnode.h> /* REQUIRED */

#define	NAME_MAX	64

/* incore memfs mkroot image struct */
typedef struct memfs_image {
	size_t	mi_size;
	vtype_t	mi_type;
	mode_t	mi_mode;
	uid_t	mi_uid;
	gid_t	mi_gid;
	timestruc_t	mi_atime;
	timestruc_t	mi_mtime;
	timestruc_t	mi_ctime;
	daddr_t	mi_addr;
	off_t	mi_pnumber;
	void 	*mi_vnode;
	void	*mi_plist;
	char	mi_name[NAME_MAX];
	char	mi_tname[NAME_MAX];
} memfs_image_t;

#endif
