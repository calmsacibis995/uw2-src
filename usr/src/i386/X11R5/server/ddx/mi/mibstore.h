/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/mi/mibstore.h	1.1"

/*-
 * mibstore.h --
 *	Header file for users of the MI backing-store scheme.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$XConsortium: mibstore.h,v 5.1 91/05/10 17:39:33 keith Exp $ SPRITE (Berkeley)"
 */

#ifndef _MIBSTORE_H
#define _MIBSTORE_H

typedef struct {
    void	    (*SaveAreas)(/* pPixmap, pObscured, x, y, pWin */);
    void	    (*RestoreAreas)(/* pPixmap, pExposed, x, y, pWin */);
    void	    (*SetClipmaskRgn)(/* pGC, pRegion */);
    PixmapPtr	    (*GetImagePixmap)(); /* unused */
    PixmapPtr	    (*GetSpansPixmap)(); /* unused */
} miBSFuncRec, *miBSFuncPtr;

extern void miInitBackingStore();

#endif /* _MIBSTORE_H */
