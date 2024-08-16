/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386at/ktool/idtools/mdep.h	1.11"
#ident	"$Header:"

/*
 * Machine-specific definitions for ID/TP.
 */

/* System file parameter ranges */
#define CLK_IVN	0		/* clock IVN */
#define CLK_IPL	8		/* clock IPL */
#define	SIVN	0		/* start of IVN range */
#define EIVN	31		/* end of IVN range (inclusive) */
#define SIPL	1		/* start of IPL range */
#define EIPL	9		/* end of IPL range (inclusive) */
#define SITYP	0		/* start of ITYP range */
#define EITYP	4		/* end of ITYP range (inclusive) */
#define	SIOA	0x0L		/* start of I/O address range */
#define EIOA	0xFFFFL		/* end of I/O address range (inclusive) */
#define	SCMA	0x10000L	/* start of controller memory address range */
#define DMASIZ	15		/* highest dma channel number permitted */


/* Current legal Master file flags */
#define ALL_MFLAGS	"bcdefhklmouCDFKLMOS-"
/* Obsolete Master file flags */
#define OLD_MFLAGS	"ainprstGHMNR"

/* Specific machine-specific Master file flags */
#define IOOVLOK	'O'		/* IOA regions can overlap		*/
#define	DMASHR	'D'		/* can share DMA channel		*/
#define	MEMSHR	'M'		/* can share DMA channel		*/

/* Macro for idcheck to determine if I/O overlap is allowed */
#define IO_OVERLAP_OK(mdev)	(INSTRING((mdev)->mflags, IOOVLOK))
