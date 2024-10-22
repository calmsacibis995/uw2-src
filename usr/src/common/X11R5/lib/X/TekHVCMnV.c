/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:TekHVCMnV.c	1.2"

/* $XConsortium: TekHVCMnV.c,v 1.6 91/07/25 01:08:09 rws Exp $" */

/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 * 
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  TekColor is a
 * trademark of Tektronix, Inc.  The term "TekHVC" designates a particular
 * color space that is the subject of U.S. Patent No. 4,985,853 (equivalent
 * foreign patents pending).  Permission is hereby granted to use, copy,
 * modify, sell, and otherwise distribute this software and its
 * documentation for any purpose and without fee, provided that:
 * 
 * 1. This copyright, permission, and disclaimer notice is reproduced in
 *    all copies of this software and any modification thereof and in
 *    supporting documentation; 
 * 2. Any color-handling application which displays TekHVC color
 *    cooordinates identifies these as TekHVC color coordinates in any
 *    interface that displays these coordinates and in any associated
 *    documentation;
 * 3. The term "TekHVC" is always used, and is only used, in association
 *    with the mathematical derivations of the TekHVC Color Space,
 *    including those provided in this file and any equivalent pathways and
 *    mathematical derivations, regardless of digital (e.g., floating point
 *    or integer) representation.
 * 
 * Tektronix makes no representation about the suitability of this software
 * for any purpose.  It is provided "as is" and with all faults.
 * 
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR THE PERFORMANCE OF THIS SOFTWARE.
 *
 *	NAME
 *		TekHVCMnV.c
 *
 *	DESCRIPTION
 *		Source for XcmsTekHVCQueryMinV gamut boundary querying routine.
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *	DEFINES
 */
#define EPS	    0.001

/*
 *	EXTERNS
 */
extern Status _XcmsTekHVCQueryMaxVCRGB();
extern Status _XcmsTekHVC_CheckModify();
extern XcmsColorSpace XcmsTekHVCColorSpace;


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsTekHVCQueryMinV - Compute minimum value for hue and chroma
 *
 *	SYNOPSIS
 */
Status
XcmsTekHVCQueryMinV (ccc, hue, chroma, pColor_return)
    XcmsCCC ccc;
    XcmsFloat hue;
    XcmsFloat chroma;
    XcmsColor *pColor_return;

/*
 *	DESCRIPTION
 *		Return the minimum value for a specific hue, and the
 *		corresponding chroma.  The input color specification
 *		may be in any format, however output is in XcmsTekHVCFormat.
 *
 *		Since this routine works with the value within
 *		pColor_return intermediate results may be returned
 *		even though it may be invalid.
 *
 *	ASSUMPTIONS
 *		This routine assumes that the white point associated with
 *		the color specification is the Screen White Point.  The
 *		Screen White Point will also be associated with the
 *		returned color specification.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *		XcmsSuccess - Succeeded with no modifications
 *
 */ 
{
    XcmsCCCRec myCCC;
    XcmsColor tmp;
    XcmsColor max_vc;

    /*
     * Check Arguments
     */
    if (ccc == NULL || pColor_return == NULL) {
	return(XcmsFailure);
    }

    /*
     * Insure TekHVC installed
     */
    if (XcmsAddColorSpace(&XcmsTekHVCColorSpace) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* Use my own CCC */
    bcopy ((char *)ccc, (char *)&myCCC, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;/* inherit screen white pt */
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;/* no gamut comp func */

    tmp.spec.TekHVC.H = hue;
    tmp.spec.TekHVC.V = 100.0;
    tmp.spec.TekHVC.C = chroma;
    tmp.pixel = pColor_return->pixel;
    tmp.format = XcmsTekHVCFormat;


    /* Check for a valid HVC */
    if (!_XcmsTekHVC_CheckModify (&tmp)) {
	return(XcmsFailure);
    }

    /* Step 1: compute the maximum value and chroma for this hue. */
    /*         This copy may be overkill but it preserves the pixel etc. */
    bcopy((char *)&tmp, (char *)&max_vc, sizeof(XcmsColor));
    if (_XcmsTekHVCQueryMaxVCRGB (&myCCC, max_vc.spec.TekHVC.H, &max_vc,
	    (XcmsRGBi *)NULL) == XcmsFailure) {
	return(XcmsFailure);
    }

    /* Step 2: find the intersection with the maximum hvc and chroma line. */
    if (tmp.spec.TekHVC.C > max_vc.spec.TekHVC.C + EPS) {
	/* If the chroma is to large then return maximum hvc. */
	tmp.spec.TekHVC.C = max_vc.spec.TekHVC.C;
	tmp.spec.TekHVC.V = max_vc.spec.TekHVC.V;
    } else {
	tmp.spec.TekHVC.V = tmp.spec.TekHVC.C *
		max_vc.spec.TekHVC.V / max_vc.spec.TekHVC.C;
	if (tmp.spec.TekHVC.V > max_vc.spec.TekHVC.V) {
	    tmp.spec.TekHVC.V = max_vc.spec.TekHVC.V;
	} else if (tmp.spec.TekHVC.V < 0.0) {
	    tmp.spec.TekHVC.V = tmp.spec.TekHVC.C = 0.0;
	}
    }
    if (_XcmsTekHVC_CheckModify (&tmp)) {
	bcopy ((char *) &tmp, (char *) pColor_return, sizeof (XcmsColor));
	return(XcmsSuccess);
    } else {
	return(XcmsFailure);
    }
}
