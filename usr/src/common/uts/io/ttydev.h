/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TTYDEV_H	/* wrapper symbol for kernel use */
#define _IO_TTYDEV_H	/* subject to change without notice */

#ident	"@(#)kern:io/ttydev.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Terminal definitions related to underlying hardware.
 */

/*
 * Speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14
#define B38400	15
#define EXTA	14
#define EXTB	15

#if 0
/*
 * Hardware bits.
 * SHOULD NOT BE HERE.
 */
#define	DONE	0200
#define	IENABLE	0100

/*
 * Modem control commands.
 */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3

#endif /* end if 0 comment */

#if defined(__cplusplus)
        }
#endif
#endif /*_IO_TTYDEV_H */
