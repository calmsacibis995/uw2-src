/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/portable.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/portable.h 1.5 (Novell) 7/30/91
 */

/*
**	NetWare/SRC 
**
**		Header file to define those variables to ease portability of 
**		this product to various environments.
*/

/* Has this file already been defined? */
#ifndef	PORTABLE_HEADER
#define	PORTABLE_HEADER		/* nothing */

/*	Define machine type definitions. */
#ifdef	i386
#define	LO_HI_MACH_TYPE
#endif

#ifdef	m68k
#define	HI_LO_MACH_TYPE
#endif

#ifdef	m88k
#define	HI_LO_MACH_TYPE
#define	STRICT_ALIGNMENT
#endif

#ifdef	sparc
#define	HI_LO_MACH_TYPE
#define	STRICT_ALIGNMENT
#endif

#ifdef	r3000
#define	HI_LO_MACH_TYPE
#define	STRICT_ALIGNMENT
#endif

#ifdef	ibmr2
#define	HI_LO_MACH_TYPE
#define	STRICT_ALIGNMENT
#endif


/* Number of elements in array */
#define	DIM(a)	(sizeof(a)/sizeof(*(a)))

/* Scope control keywords */
#define	public				/* public is default in C */
#define	private	static		/* static really means private */


#define	FREE		0
#define	IN_USE		1

/* Byte swap macros */
#ifdef LO_HI_MACH_TYPE	 
#define GETINT16(x)		(\
						((uint16)(x & 0x00FF) << 8) | \
						((uint16)(x & 0xFF00) >> 8) \
						)
#define PUTINT16(x)		GETINT16(x)

#define GETINT32(x)		(\
						((uint32)(x & 0x000000FF) << 24) | \
						((uint32)(x & 0x0000FF00) <<  8) | \
						((uint32)(x & 0x00FF0000) >>  8) | \
						((uint32)(x & 0xFF000000) >> 24) \
						) 
#define PUTINT32(x)		GETINT32(x)
#define REVGETINT16(x)		(x)
#define REVGETINT32(x)		(x)
#endif   /*  end of #ifdef LO_HI */

#ifdef HI_LO_MACH_TYPE   /* MOTOROLA 680X0 processor family */
#define GETINT16(x)			(x)
#define GETINT32(x)			(x)
#define PUTINT16(x)			(x)
#define PUTINT32(x)			(x)
#define	REVGETINT16(x)		(\
							((uint16)(x & 0x00FF) << 8) | \
							((uint16)(x & 0xFF00) >> 8) \
							)
#define REVGETINT32(x)		(\
							((uint32)(x & 0x000000FF) << 24) | \
							((uint32)(x & 0x0000FF00) <<  8) | \
							((uint32)(x & 0x00FF0000) >>  8) | \
							((uint32)(x & 0xFF000000) >> 24) \
							) 
#endif   /*  end of #ifdef HI_LO_MACH_TYPE */

/* Alignment macros */
#ifdef STRICT_ALIGNMENT
#define GETALIGN32(src, dest)	{ \
								register uint8 *sptr = (uint8 *) src; \
								register uint8 *dptr = (uint8 *) dest; \
								dptr[0] = sptr[0]; \
								dptr[1] = sptr[1]; \
								dptr[2] = sptr[2]; \
								dptr[3] = sptr[3]; \
								}
#define GETALIGN16(src, dest)	{ \
								register uint8 *sptr = (uint8 *) src; \
								register uint8 *dptr = (uint8 *) dest; \
								dptr[0] = sptr[0]; \
								dptr[1] = sptr[1]; \
								}
#else
#define GETALIGN32(src, dest) (*((uint32 *)dest) = *((uint32 *)src))
#define GETALIGN16(src, dest) (*((uint16 *)dest) = *((uint16 *)src))
#endif		/* end of STRICT_ALIGN */

#endif		/* end of #ifndef PORTABLE_HEADER */
