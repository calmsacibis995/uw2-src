/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:clients/snftobdf/bdf2snf_font.h	1.1"

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
#ifndef BDF2SNF_FONT_H

#define LeftToRight 0
#define RightToLeft 1
/*
 * for linear char sets
 */
#define n1dChars(pfi) ((pfi)->lastCol - (pfi)->firstCol + 1)
#define chFirst firstCol	/* usage:  pfi->chFirst */
#define chLast lastCol		/* usage:  pfi->chLast */

/*
 * for 2D char sets
 */
#if 0
#define n2dChars(pfi)	(((pfi)->lastCol - (pfi)->firstCol + 1) * \
			 ((pfi)->lastRow - (pfi)->firstRow + 1))

#define ADDRXTHISCHARINFO( pf, ch ) \
        ((CharInfoRec *) &((pf)->pCI[(ch) - (pf)->pFI->chFirst]))

#define	GLWIDTHPIXELS(pci) \
	((pci)->metrics.rightSideBearing - (pci)->metrics.leftSideBearing)
#define	GLHEIGHTPIXELS(pci) \
 	((pci)->metrics.ascent + (pci)->metrics.descent)


#define	GLYPHWIDTHBYTES(pci)	(((GLYPHWIDTHPIXELS(pci))+7) >> 3)
#define	GLYPHHEIGHTPIXELS(pci)	(pci->metrics.ascent + pci->metrics.descent)
#define	GLYPHWIDTHPIXELS(pci)	(pci->metrics.rightSideBearing \
				    - pci->metrics.leftSideBearing)
#define GLWIDTHPADDED( bc)	((bc+7) & ~0x7)

#if GLYPHPADBYTES == 0 || GLYPHPADBYTES == 1
#define	GLYPHWIDTHBYTESPADDED(pci)	(GLYPHWIDTHBYTES(pci))
#define	PADGLYPHWIDTHBYTES(w)		(((w)+7)>>3)
#endif

#if GLYPHPADBYTES == 2
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+1) & ~0x1)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+1) & ~0x1)
#endif

#if GLYPHPADBYTES == 4
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+3) & ~0x3)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+3) & ~0x3)
#endif

#if GLYPHPADBYTES == 8 /* for a cray? */
#define	GLYPHWIDTHBYTESPADDED(pci)	((GLYPHWIDTHBYTES(pci)+7) & ~0x7)
#define	PADGLYPHWIDTHBYTES(w)		(((((w)+7)>>3)+7) & ~0x7)
#endif

typedef struct _FontProp *FontPropPtr;
typedef struct _CharInfo *CharInfoPtr;
typedef struct _FontInfo *FontInfoPtr;
typedef unsigned int DrawDirection;
typedef struct _ExtentInfo *ExtentInfoPtr;

#endif
#endif /* FONT_H */
