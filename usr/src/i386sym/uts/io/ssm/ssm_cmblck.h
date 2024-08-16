/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_SSM_SSM_CMBLCK_H	/* wrapper symbol for kernel use */
#define	_IO_SSM_SSM_CMBLCK_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm_cmblck.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ssm_cmblck.h
 *	Definitions for the device driver command block interface.
 */


/*
 * The  descriptor  cb_desc  contains   four   fields
 * described  below.  After the cb_desc is filled out
 * it should contain all the information needed about
 * a drivers command blocks.
 */
struct cb_desc {
	char *cb_cbs;			/* pointer to block of cbs */
	short cb_state_offset;		/* offset of state field */
	short cb_count;			/* number of cbs */
	short cb_size;			/* size in bytes of a command block */
};

/*
 * macro to compute state field offset
 */
#define FLD_OFFSET(struct, field) (int) &(((struct *) 0)->field)

/*
 * command block is in use if bit 31 is set
 */
#define CB_BUSY 0x80000000

/*
 * Functions available in cntrlblock.c.
 */
#if defined(_KERNEL)

extern struct cb_desc *alloc_cb_desc(caddr_t, short, short, short); 
extern caddr_t get_cb(struct cb_desc *);
extern void free_cb(caddr_t, struct cb_desc *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_CMBLCK_H_ */
