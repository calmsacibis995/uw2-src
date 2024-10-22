/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:std.h	1.1.8.2"
#ident  "$Header: std.h 1.3 91/06/21 $"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *	@(#) std.h 1.1 88/03/30 inccmd:std.h
 */
#define	SYSBSIZE	BSIZE		/* system block size */
#define	SYSBSHIFT	BSHIFT

#define	EFFBSIZE	SYSBSIZE	/* efficient block size */
#define	EFFBSHIFT	SYSBSHIFT

#define	MULWSIZE	2		/* multiplier 'word' */
#define	MULWSHIFT	1
#define	MULLSIZE	4		/* multiplier 'long' */
#define	MULLSHIFT	2
#define	MULBSIZE	512		/* multiplier 'block' */
#define	MULBSHIFT	9
#define	MULKSIZE	1024		/* multiplier 'k' */
#define	MULKSHIFT	10

#define	SYSTOMUL(sysblk)	((sysblk) * (SYSBSIZE / MULBSIZE))
