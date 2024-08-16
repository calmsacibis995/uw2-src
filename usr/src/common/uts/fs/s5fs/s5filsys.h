/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5FILSYS_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5FILSYS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5filsys.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Structure of the super-block.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <fs/s5fs/s5param.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/fs/s5param.h> /* REQUIRED */
#else
#include <sys/fs/s5param.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct	filsys
{
	u_short	s_isize;	/* size in blocks of i-list */
	daddr_t	s_fsize;	/* size in blocks of entire volume */
	short	s_nfree;	/* number of addresses in s_free */
	daddr_t	s_free[NICFREE];/* free block list */
		/* S5 inode definition cannot change for EFT */
	short	s_ninode;	/* number of i-nodes in s_inode */
	o_ino_t	s_inode[NICINOD];/* free i-node list */
	char    s_flock;        /* lock during free list manipulation */
        char    s_ilock;        /* lock during i-list manipulation */
	char  	s_fmod; 	/* super block modified flag */
	char	s_ronly;	/* mounted read-only flag */
	time_t	s_time; 	/* last super block update */
	short	s_dinfo[4];	/* device information */
	daddr_t	s_tfree;	/* total free blocks*/
	o_ino_t	s_tinode;	/* total free inodes */
	char	s_fname[6];	/* file system name */
	char	s_fpack[6];	/* file system pack name */
	long	s_fill[12];	/* adjust to make sizeof filsys */
	long	s_state;	/* file system state */
	long	s_magic;	/* magic number to indicate new file system */
	u_long	s_type;		/* type of new file system */
} filsys_t ;
#define s_bshift  s_type	/* so far, type is just bsize shift factor */

#define FsMAGIC	0xfd187e20	/* s_magic */

#define FsMINBSHIFT	1
#define FsMINBSIZE	512

#define FsBSIZE(bshift)		(FsMINBSIZE << ((bshift) - FsMINBSHIFT))

/* Old symbols for specific s_type values. */
#define Fs1b	1	/* 512-byte blocks */
#define Fs2b	2	/* 1024-byte blocks */
#define Fs4b	3	/* 2048-byte blocks */

#define	FsOKAY		0x7c269d38	/* s_state: clean */
#define	FsACTIVE	0x5e72d81a	/* s_state: active */
#define	FsBAD		0xcb096f43	/* s_state: bad root */
#define FsBADBLK	0xbadbc14b	/* s_state: bad block corrupted it */

#define getfs(vfsp)		\
  ((struct filsys *)((s5_fs_t *)vfsp->vfs_data)->fs_bufp->b_addrp)
#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_S5FS_S5FILSYS_H */
