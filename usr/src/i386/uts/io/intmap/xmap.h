/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_INTMAP_XMAP_H	/* wrapper symbol for kernel use */
#define _IO_INTMAP_XMAP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/intmap/xmap.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

							/* BEGIN SCO_INTL */
/*
 *	xmap structure - provides a per-tty structure for recording
 *	state information by emap and nmap routines.  Also used by
 *	select to avoid cluttering up tty structure.
 */

struct xmap {
	/*
	 *	select fields
	 */
	struct proc *	xm_selrd;	/* Process waiting on selwait (read) */
	struct proc *	xm_selwr;	/* Process waiting on selwait (write)*/
	/*
 	 *	emap fields (used to be in tty structure)
	 */
	struct emap *	xm_emp;		/* emapping table */
	unsigned char	xm_emchar;	/* saved emapping char */
	unsigned char	xm_emonmap;	/* True if should call nmmapout */
	/*
	 *	nmap fields
	 */
	struct nxmap *	xm_nmp;		/* nmapping table */
	char *		xm_nmiseqp;	/* Current pos in lead-in (input) */
	char *		xm_nmoseqp;	/* Current pos in lead-in (output) */
	unsigned short	xm_nmincnt;	/* # chars left in trailer (input) */
	unsigned short	xm_nmoncnt;	/* # chars left in trailer (output) */
	unsigned char	xm_nmiseqn;	/* Index of current lead-in (input) */
	unsigned char	xm_nmoseqn;	/* Index of current lead-in (output) */
};
							/* END SCO_INTL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_INTMAP_XMAP_H */
