/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_MNTTAB_H	/* wrapper symbol for kernel use */
#define _FS_MNTTAB_H	/* subject to change without notice */

#ident	"@(#)kern:fs/mnttab.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	MNTTAB	"/etc/mnttab"
#define	MNT_LINE_MAX	1024

#define	MNT_TOOLONG	1	/* entry exceeds MNT_LINE_MAX */
#define	MNT_TOOMANY	2	/* too many fields in line */
#define	MNT_TOOFEW	3	/* too few fields in line */

#define	mntnull(mp)\
	((mp)->mnt_special = (mp)->mnt_mountp =\
	 (mp)->mnt_fstype = (mp)->mnt_mntopts =\
	 (mp)->mnt_time = NULL)

#define	putmntent(fd, mp)\
	fprintf((fd), "%s\t%s\t%s\t%s\t%s\n",\
		(mp)->mnt_special ? (mp)->mnt_special : "-",\
		(mp)->mnt_mountp ? (mp)->mnt_mountp : "-",\
		(mp)->mnt_fstype ? (mp)->mnt_fstype : "-",\
		(mp)->mnt_mntopts ? (mp)->mnt_mntopts : "-",\
		(mp)->mnt_time ? (mp)->mnt_time : "-")

struct mnttab {
	char	*mnt_special;
	char	*mnt_mountp;
	char	*mnt_fstype;
	char	*mnt_mntopts;
	char	*mnt_time;
};

#ifdef __STDC__
extern int	getmntent(FILE *, struct mnttab *);
extern int	getmntany(FILE *, struct mnttab *, struct mnttab *);
#else
extern int	getmntent();
extern int	getmntany();
#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_MNTTAB_H */
