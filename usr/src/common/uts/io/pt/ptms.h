/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_PT_PTMS_H	/* wrapper symbol for kernel use */
#define _IO_PT_PTMS_H	/* subject to change without notice */

#ident	"@(#)kern:io/pt/ptms.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <io/stream.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structures and definitions supporting the pseudo terminal drivers.
 */
struct pt_ttys {
	uint_t	pt_state;	/* state of master/slave pair */
	queue_t	*ptm_wrq;	/* master's write queue pointer */
	queue_t	*pts_wrq;	/* slave's write queue pointer */
	mblk_t	*pt_bufp;	/* ptr. to zero byte msg. blk. */
	lock_t	*pt_lock;	/* to protect fields of this structure */
	int	ptm_active;	/* # active procedure on master queue */
	int	pts_active;	/* # active procedure on slave queue */
	sv_t	*ptm_sv;	/* to block master side close */
	sv_t	*pts_sv;	/* to block slave side close */
};

#endif /* _KERNEL || _KMEMUSER */

/*
 * pt_state values: 
 */
#define	PTLOCK		0001	/* master/slave pair is locked */
#define	PTMOPEN		0002	/* master side is open */
#define	PTSOPEN		0004	/* slave side is open */
#define PTMCLOSE	0010	/* master side wants to close */
#define PTSCLOSE	0020	/* slave side wants to close */
#define PTBAD		0040	/* unusable minor (lock or sv alloc. failed */
#define PTMOFF		0100	/* master side is about to call qprocsoff */

/*
 * ioctl commands
 */
#define	ISPTM	(('P'<<8)|1)	/* query for master */
#define	UNLKPT	(('P'<<8)|2)	/* unlock master/slave pair */

#ifdef _KERNEL

/*
 * tunable parameters defined in ptm.cf
 */
extern struct pt_ttys ptms_tty[];
extern int pt_cnt;
extern int pt_sco_cnt;

#else

#ifdef __STDC__
extern int grantpt(int);
extern char *ptsname(int);
extern int unlockpt(int);
#else
extern int grantpt();
extern char *ptsname();
extern int unlockpt();
#endif

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_PT_PTMS_H */
