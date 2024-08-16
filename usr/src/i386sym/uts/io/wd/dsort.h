/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_WD_DSORT_H	/* wrapper symbol for kernel use */
#define	_IO_WD_DSORT_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/wd/dsort.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL
void disksort(buf_t *, buf_t *);
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_WD_DSORT_H_ */
