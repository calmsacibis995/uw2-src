/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_PT_PTEM_H	/* wrapper symbol for kernel use */
#define _IO_PT_PTEM_H	/* subject to change without notice */

#ident	"@(#)kern:io/pt/ptem.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#else

#include <sys/stream.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifndef _IO_TERMIOS_H
#ifndef _IO_TTOLD_H

/* Windowing structure to support TIOCSWINSZ/TIOCGWINSZ */
struct winsize {
	unsigned short ws_row;       /* rows, in characters*/
	unsigned short ws_col;       /* columns, in character */
	unsigned short ws_xpixel;    /* horizontal size, pixels */
	unsigned short ws_ypixel;    /* vertical size, pixels */
};

#endif /* end _IO_TTOLD_H */
#endif /* end _IO_TERMIOS_H */

#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * The ptem data structure used to define the global data
 * for the psuedo terminal emulation streams module
 */
struct ptem {
	unsigned short ptem_state;	/* state of ptem entry; see below */
	long ptem_cflags;	/* copy of c_cflags */
	mblk_t *ptem_dackp;	/* preallocated msgb used to ACK disconnect */
	lock_t *ptem_lock;	/* to protect fields of this structure */
	struct winsize ptem_wsz;	/* hold the windowing info. */
};

/*
 * ptem_state:
 */
#define	STRFLOW		0x1	/* streams flow control is on */
#define	OFLOW_CTL	0x2	/* Outflow control on */

/*
 * Constants used to distinguish between a common function invoked
 * from the read or write side put procedures
 */
#define	RDSIDE	1
#define	WRSIDE	2

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_PT_PTEM_H */
