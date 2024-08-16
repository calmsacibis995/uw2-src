/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_P_SYSI86_H	/* wrapper symbol for kernel use */
#define _SVC_P_SYSI86_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/p_sysi86.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * i386at-specific sysi86 system call subfunctions
 */

#define RTODC		60	/* Read time of day clock */
#define STIME 		54	/* Set internal time */
#define	SI86SLTIME	72	/* Set local time correction */

#define SI86RDID	92	/* ROM BIOS Machid ID */
#define SI86RDBOOT	93	/* Bootable Non-SCSI Hard Disk */
#define SI86BUSTYPE     94      /* Determine I/O Bus Type */
#define SI86SDIDEV      95      /* return SDI pass-thru device */

/* BIOS ID values for SI86RDID */

#define C2	'E'	/* AT&T "Cascade 2" */
#define C3	'F'	/* AT&T "Cascade 3" */
#define C4	'G'	/* AT&T "Cascade 4" */
#define C6	'K'	/* AT&T "Cascade 6" */
#define E8R1	'R'	/* AT&T "Enterprise E8R1" */

/*
 * SI86BUSTYPE return values
 */

#define ISA_BUS		0
#define EISA_BUS	1
#define MCA_BUS		2

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_P_SYSI86_H */
