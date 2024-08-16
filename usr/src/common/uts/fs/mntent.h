/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_MNTENT_H	/* wrapper symbol for kernel use */ 
#define _FS_MNTENT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/mntent.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _KERNEL

#include <string.h>

#define MNTTAB		"/etc/mnttab"
#define VFSTAB		"/etc/vfstab"

#define MNTTYPE_SFS	"sfs"
#define MNTTYPE_RUFS	"rufs"
#define MNTTYPE_UFS	"ufs"
#define MNTTYPE_SWAP	"swap"	/* swap file system */
#define MNTTYPE_IGNORE	"ignore"/* No type specified, ignore this entry */
#define MNTTYPE_LO	"lo"	/* Loop back File system */

#define MNTOPT_RO	"ro"	/* read only */
#define MNTMAXSTR	128
#define MNTOPT_RW	"rw"	/* read/write */
#define MNTOPT_RQ	"rq"	/* read/write with quotas */
#define MNTOPT_QUOTA	"quota"	/* quotas */
#define MNTOPT_NOQUOTA	"noquota"/* no quotas */
#define MNTOPT_SOFT	"soft"	/* soft mount */
#define MNTOPT_HARD	"hard"	/* hard mount */
#define MNTOPT_NOSUID	"nosuid"/* no set uid allowed */
#define MNTOPT_NOAUTO	"noauto"/* hide entry from mount -a */
#define MNTOPT_GRPID	"grpid"	/* SysV-compatible group-id on create */
#define MNTOPT_REMOUNT	"remount"/* change options on previous mount */
#define MNTOPT_NOSUB	"nosub"	/* disallow mounts beneath this one */
#define MNTOPT_MULTI	"multi"	/* Do multi-component lookup */

#define L_SET		0	/* for lseek */

#ifdef __STDC__
extern char * hasmntopt(struct mnttab *, char *);
#else 
extern char * hasmntopt();
#endif

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_MNTENT_H */
