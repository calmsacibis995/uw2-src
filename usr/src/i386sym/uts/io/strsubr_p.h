/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STRSUBR_P_H	/* wrapper symbol for kernel use */
#define _IO_STRSUBR_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/strsubr_p.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif


#ifdef _KERNEL

/*
 * MACRO 
 * STRIOCTL_P(struct vnode *vp, int *cmdp, int *argp, int flag, int copyflag,
 *	cred_t *crp, int *rvalp, void **iocstatep, mblk_t **mp, int *error)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any platform-specific ioctl processing. On an AT platform
 *	all the platform-specfic ioctl conversion and remapping are
 *	isolated in this interface. However, on the Sequent platforms
 *	this is a NULL macro.
 */

#define STRIOCTL_P(vp, cmdp, argp, flag, copyflag, crp, rvalp, iocstatep, mp, error)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STRSUBR_P_H */
