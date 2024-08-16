/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef _STAND_SAIO_H
#define _STAND_SAIO_H

#ident	"@(#)stand:i386sym/standalone/sys/saio.h	1.1"

/*
 * Saio.h
 * 	Standard I/O interface defintions for standalone package.
 */

#ifndef _SYS_BFS_H
#include <sys/bfs.h>
#endif

/*
 * An in-core inode consists of information about the
 * device from which it originates and a copy of the
 * on-device inode from the BFS filesystem.
 */
struct inode {
	struct	inode *i_next;	/* ptr to next inode */
	dev_t	i_devunit;	/* device where inode resides */
	ino_t	i_number;	/* this inode's number, 1-to-1 filesystem */
	ushort	i_flag;		/* for tracking changes to the inode */
	struct bfs_info	*i_bfs; /* Addr of filesystem dependant data, i.e., 
				 * in-core superblock data and bookkeeping. */
	struct  bfs_dirent i_dir; /* on-disk portion of BFS inode */
};

/* Flags for the i_flag member of struct inode */
#define	IUPD		0x001		/* file has been modified */
#define	IACC		0x002		/* inode access time to be updated */
#define	ICHG		0x004		/* inode has been changed */

/*
 * I/O descriptor block: includes an
 * inode, cells for the use of seek, etc,
 * and a buffer.
 */
struct	iob {
	int	i_flgs;		/* see F_ below */
	struct	inode *i_ino;	/* inode, if file */
	int	i_unit;		/* pseudo device unit */
	int	i_partno;	/* device subunit# (partition, file, etc) */
	daddr_t	i_boff;		/* block offset on device */
	off_t	i_offset;	/* seek offset in file */
	daddr_t	i_bn;		/* 1st block # of next read */
	char	*i_ma;		/* memory address of i/o buffer */
	int	i_cc;		/* character count of transfer */
	int	i_error;	/* error # return */
	int	i_errcnt;	/* error count for driver retries */
	int	i_howto;	/* how to open */
	caddr_t	i_buf;		/* i/o buffer */
	struct bfs_info	*i_bfs; /* Addr of filesystem dependant data, i.e., 
				 * in-core superblock data and bookkeeping. */
	dev_t	i_dev;		/* device for open */
	ino_t	i_inodenum;	/* inode number */
	char	i_fname[MAXPATHLEN];
	char	*i_dptr;	/* private data pointer for drivers */
};

#define NULL 0
#define O_MODIFY        (O_CREAT|O_TRUNC|O_RDWR|O_WRONLY)	/* for mode_t */

#define F_READ		0x1	/* file opened for reading */
#define F_WRITE		0x2	/* file opened for writing */
#define F_ALLOC		0x4	/* buffer allocated */
#define F_FILE		0x8	/* file instead of device */

#define	F_TYPEMASK	0x0ff00 /* for separating out the type bits */
/* io types */
#define	F_RDDATA	0x0100	/* read data */
#define	F_WRDATA	0x0200	/* write data */
/* special types */
#define F_PACKET	0x10000	/* packet (should be outside F_TYPEMASK) */
#define F_TAPE		0x20000 /* tape (should be outside F_TYPEMASK) */


/*
 * generic SCSI drive data referenced through the iob element i_dptr
 */
struct drive_info {
	int	(*di_cmd) ();	/* SCSI adapter specific command function */
	struct	st *di_geom;	/* quick access for disk geometry */
	char	*di_prefix;	/* drive ascii prefix (ex. "wd") */
	unchar	di_flag;	/* local flag: set to 0 when leaving driver */
	unchar	di_pad[3];	/* unused */
	char	*di_ptr;	/* pointer to SCSI adapter specific data */
};

#define SAIOSCSI_INFO(__io)	((struct drive_info *)(void *)((__io)->i_dptr))
#define SAIOSCSI_CMD(_io,_cp)	(SAIOSCSI_INFO(_io)->di_cmd(_io,_cp))
#define SAIOSCSI_GEOM(_io)	(SAIOSCSI_INFO(_io)->di_geom)
#define SAIOSCSI_PREFIX(_io)	(SAIOSCSI_INFO(_io)->di_prefix)
#define SAIOSCSI_FLAG(_io)	(SAIOSCSI_INFO(_io)->di_flag)
#define SAIOSCSI_PTR(_io)	(SAIOSCSI_INFO(_io)->di_ptr)

/*
 * File System Block buffer
 */
struct buf {
	char	b_buf[MAXBSIZE];	/* data buffer */
	struct buf *b_next;		/* next pointer */
	struct buf *b_prev;		/* prev pointer */
	daddr_t	b_blkno;		/* phys blkno of data */
	dev_t	b_dev;			/* device of data */
	int	b_usecnt;		/* use count */
	int	b_bcount;		/* amount of data in buffer */
	int	b_flags;		/* for errors, etc */
};

#define	B_WRITE	0x01
#define B_READ	0x02
#define B_ERROR	0x04
#define B_MOD	0x08

/*
 * args to brelse/bwrite
 */
#define DONTFREE	0
#define MRU		1	/* put at tail of free list */
#define LRU		2	/* put at head of free list */

/*
 * 3rd arg to bwrite
 */
#define	W_DELAY		0	/* don't write immediately */
#define W_NODELAY	1	/* write immediately */

/*
 * action arg to getblk
 */
#define ZERO		0
#define DONOTHING	1
#define DOREAD		2

#define bread(dev, bn, size)	(getblk((dev), (bn), (size), DOREAD))

/*
 * Device switch.
 */
struct devsw {
	char	*dv_name;
	int	(*dv_strategy)();
	void	(*dv_open)();
	void	(*dv_close)();
	int	(*dv_ioctl)();
	off_t	(*dv_lseek)();	/* SCS: for the packet drivers */
	int	dv_flags;	/* SCS: flags to indicate it is packet */
};

#define	D_PACKET	0x01	/* SCS: driver is packet driver */
#define D_DISK		0x02	/* SCS: driver is a disk driver */
#define D_TAPE		0x04	/* SCS: driver is a tape driver */

/*
 * Drive description table.
 * Returned from SAIODEVDATA call.
 */
struct st {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/surfaces/heads */
	short	nspc;		/* # sectors/cylinder */
	short	ncyl;		/* # cylinders */
};

/*
 * Request codes. Must be the same a F_XXX above
 */
#define	READ	1
#define	WRITE	2

#define	NFILES	8
#define NBUFS	6	
#define NINODES	(NFILES+2)
#define RAWALIGN 16

/* error codes */
#define	EBADF	1	/* bad file descriptor */
#define	EOFFSET	2	/* relative seek not supported */
#define	EDEV	3	/* improper device specification on open */
#define	ENXIO	4	/* unknown device specified */
#define	EUNIT	5	/* improper unit specification */
#define	ESRCH	6	/* directory search for file failed */
#define	EIO	7	/* generic error */
#define	ECMD	10	/* undefined driver command */
#define	EBSE	11	/* bad sector error */
#define	EWCK	12	/* write check error */
#define	EECC	13	/* uncorrectable ecc error */
#define	EHER	14	/* hard error */
#define	EHDRECC	15	/* uncorrectable header ecc error */
#define	ESO	16	/* Sector overun error - indicates header problem */
#define	EID	17	/* ID field specified by cyl, head, sect not found */
#define	EDAM	18	/* data address mark not found within 16 bytes of ID */
#define EEXISTS 19	/* file exists */
#define ENOSPC	20	/* no space left on device */
#define EFBIG	21	/* file too large */
#define ENOTDIR	22	/* not a directory */
#define EACCES	23	/* Permission denied (write access not permitted) */
#define EINVAL	24	/* Invalid argument */
#define ENOENT	25	/* file or directory entry not found */

/* ioctl's -- for disks just now */
#define	SAIODEVDATA	(('d'<<8)|8)	/* get device data */
/* Sequent added IOCTL calls */
#define SAIOSCSICMD	(('d'<<8)|15)	/* SCSI command */
#define SAIOFIRSTSECT	(('d'<<8)|19)	/* Return first sector of usable space*/
#define SAIOSCSICMDOLD	(('d'<<8)|21)	/* SCSI command: old pass-thru format */

/* error code for talking to SLIC */
#define	SL_GOOD		(SL_PARITY|SL_EXISTS|SL_OK)
#define SL_NOT_OK	(-1)
#define	SL_BAD_INTR	(-2)
#define SL_BAD_PAR	(-3)
#define SL_BAD_DEST	(-4)
#define SL_PERROR	(-1)
#define SL_DERROR	(-2)
#define SL_NOTOK	(-3)

/*
 * External standard I/O interfaces.
 */
extern	int errno;		/* Just like unix */

/*
 * Definitions shared only between internal standard I/O modules.
 */
extern struct	iob iob[];	/* File descriptors */
extern struct inode inode[];	/* In-core inodes */
struct inode  *ino_list;	/* Linked list of allocated inodes. */
extern int modify_flag; 	/* Set if filesys use may modify it */ 
extern struct devsw devsw[];	/* Driver interface switch table */
extern int n_devsw;		/* # records in driver interface switch table */

extern struct buf *getblk(dev_t, daddr_t, int, int);
extern void brelse(struct buf *, int);
extern void bwrite(struct buf *, int, int);

extern int read(int, char *, int);
extern int write(int, char *, int);
extern off_t lseek(int, off_t, int);
extern int close(int);
extern int ioctl(int, int, char *);
extern char *gets(char *);

#endif /* _STAND_SAIO_H */
