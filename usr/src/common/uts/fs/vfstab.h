/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_VFSTAB_H	/* wrapper symbol for kernel use */
#define _FS_VFSTAB_H	/* subject to change without notice */

#ident	"@(#)kern:fs/vfstab.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	VFSTAB	"/etc/vfstab"
#define	VFS_LINE_MAX	1024

#define	VFS_TOOLONG	1	/* entry exceeds VFS_LINE_MAX */
#define	VFS_TOOMANY	2	/* too many fields in line */
#define	VFS_TOOFEW	3	/* too few fields in line */

#define	vfsnull(vp)	((vp)->vfs_special = (vp)->vfs_fsckdev =\
			 (vp)->vfs_mountp = (vp)->vfs_fstype =\
			 (vp)->vfs_fsckpass = (vp)->vfs_automnt =\
			 (vp)->vfs_mntopts = (vp)->vfs_macceiling = NULL)

#define	putvfsent(fd, vp)\
	fprintf((fd), "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",\
		(vp)->vfs_special ? (vp)->vfs_special : "-",\
		(vp)->vfs_fsckdev ? (vp)->vfs_fsckdev : "-",\
		(vp)->vfs_mountp ? (vp)->vfs_mountp : "-",\
		(vp)->vfs_fstype ? (vp)->vfs_fstype : "-",\
		(vp)->vfs_fsckpass ? (vp)->vfs_fsckpass : "-",\
		(vp)->vfs_automnt ? (vp)->vfs_automnt : "-",\
		(vp)->vfs_mntopts ? (vp)->vfs_mntopts : "-", \
		(vp)->vfs_macceiling ? (vp)->vfs_macceiling : "-")

struct vfstab {
	char	*vfs_special;
	char	*vfs_fsckdev;
	char	*vfs_mountp;
	char	*vfs_fstype;
	char	*vfs_fsckpass;
	char	*vfs_automnt;
	char	*vfs_mntopts;
	char	*vfs_macceiling; 	/* field to contain mac ceiling */
					/* of file system */
};

#ifdef __STDC__
extern int	getvfsent(FILE *, struct vfstab *);
extern int	getvfsspec(FILE *, struct vfstab *, const char *);
extern int	getvfsfile(FILE *, struct vfstab *, const char *);
extern int	getvfsany(FILE *, struct vfstab *, const struct vfstab *);
#else
extern int	getvfsent();
extern int	getvfsspec();
extern int	getvfsfile();
extern int	getvfsany();
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_VFSTAB_H */
