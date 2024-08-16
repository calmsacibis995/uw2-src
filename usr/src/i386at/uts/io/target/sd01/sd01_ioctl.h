/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TARGET_SD01_SD01_IOCTL_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SD01_SD01_IOCTL_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sd01/sd01_ioctl.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define SD_CHAR		('D' << 8)
#define	SD_ELEV		(SD_CHAR | 0x1)		/* Elevator Algorithm */
#define	SD_PDLOC	(SD_CHAR | 0x2)		/* Absolute PD sector */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SD01_SD01_IOCTL_H */
