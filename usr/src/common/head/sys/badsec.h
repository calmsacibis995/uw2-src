/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tmp.head.sys:sys/badsec.h	1.1"

#ifndef _IO_TARGET_BADSEC_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_BADSEC_H	/* subject to change without notice */

#define	BADSECFILE	"/etc/scsi/badsec"

#define	MAXBLENT	4
struct	badsec_lst {
	int	bl_cnt;
	struct	badsec_lst *bl_nxt;
	int	bl_sec[MAXBLENT];
};

#define BADSLSZ		sizeof(struct badsec_lst)

#endif /* _IO_TARGET_BADSEC_H */
