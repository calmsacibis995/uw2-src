/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:i386/head/stand.h	1.6.4.2"

#ifndef _STAND_H
#define _STAND_H

/*
 * Header file for standalone package
 */

#if defined(__STDC__)

#if #machine(m68k) || #machine(m88k)
#define	FsTYPE	2
#else
#define	FsTYPE	1
#endif

#else
#if m68k || m88k
#define	FsTYPE	2
#else
#define	FsTYPE	1
#endif

#endif 	/* __STDC__ */

#ifndef _ERRNO_H
#include "errno.h"
#endif

#ifndef _SYS_PARAM_H
#include "sys/param.h"
#endif
#ifndef _SYS_TYPES_H
#include "sys/types.h"
#endif
#ifndef _SYS_INODE_H
#include "sys/inode.h"
#endif

/*
 * I/O block flags
 */

#define F_READ	01
#define F_WRITE	02
#define F_ALLOC	04
#define F_FILE	010

/*
 * Request codes -- must be
 * the same as an F_XXX above
 */

#define	READ	1
#define	WRITE	2

/*
 * Buffer sizes
 */

#if FsTYPE == 2
#define BLKSIZ	1024
#else
#define BLKSIZ	512
#endif
#define NAMSIZ	60

/*
 * devsw table --
 * initialized in conf.c
 */

struct devsw {
	int	(*dv_strategy)();
	int	(*dv_open)();
	int	(*dv_close)();
};
extern struct devsw _devsw[];

/*
 * dtab table -- entries
 * are created by MKNOD
 */

#define NDEV	16

struct dtab {
	char		*dt_name;
	struct devsw	*dt_devp;
	int		dt_unit;
	daddr_t		dt_boff;
};
extern struct dtab _dtab[];

/*
 * mtab table -- entries
 * are created by mount
 */

#define NMOUNT	8

struct mtab {
	char		*mt_name;
	struct dtab	*mt_dp;
};
extern struct mtab _mtab[];

/*
 * I/O block: includes an inode,
 * cells for the use of seek, etc,
 * and a buffer.
 */

#define NFILES	6

struct iob {
	char		i_flgs;
	struct inode	i_ino;
	time_t		i_atime;
	time_t		i_mtime;
	time_t		i_ctime;
	struct dtab	*i_dp;
	off_t		i_offset;
	daddr_t		i_bn;		/* disk block number (physical) */
	char		*i_ma;
	int		i_cc;		/* character count */
	char		i_buf[BLKSIZ];
};
extern struct iob _iobuf[];

/*
 * Set to the error type of the routine that
 * last returned an error -- may be read by perror.
 */

#define	RAW	040
#define	LCASE	04
#define	XTABS	02
#define	ECHO	010
#define	CRMOD	020
#define B300	7
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	int	sg_flags;		/* mode flags */
};

#endif 	/* _STAND_H */
