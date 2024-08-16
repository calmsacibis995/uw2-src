/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_POSTWAIT_H    /* wrapper symbol for kernel use */
#define _IO_POSTWAIT_H    /* subject to change without notice */

#ident	"@(#)kern-i386:io/postwait/postwait.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define PWIOC		('P'<<8)
#define PWIOC_POST	(PWIOC|1)
#define PWIOC_WAIT	(PWIOC|2)

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_POSTWAIT_H */
