/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_JIOCTL_H	/* wrapper symbol for kernel use */
#define _IO_JIOCTL_H	/* subject to change without notice */

#ident	"@(#)kern:io/jioctl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * jioctl.h
 *
 * This header file is for compatibility only.  The only thing we still
 * support is JWINSIZE since pseudo ttys will do the
 * conversion for those apps that use this interface.
 */

#define	JTYPE		('j'<<8)
#define	JWINSIZE	(JTYPE|5)  /* inquire window size */

/*
 * jwinsize structure used by JWINSIZE message.
 */

struct jwinsize
{
	char	bytesx, bytesy;	/* Window size in characters */
	short	bitsx, bitsy;	/* Window size in bits */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_JIOCTL_H */
