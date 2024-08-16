/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODFS_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODFS_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/modfs.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/vfs.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

extern const char *mod_fsname(unsigned int);
extern rwlock_t mod_vfssw_lock;
extern int mod_fs_mount(struct vfs *, struct vnode *, struct mounta *,
			cred_t *);

struct mod_fs_data {
	char * mfd_name;
	struct vfsops *mfd_vfsops;
	unsigned long *mfd_fsflags;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MODFS_H */
