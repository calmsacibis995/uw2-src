/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:CIELuvGcL.c	1.2"

/* $XConsortium: CIELuvGcL.c,v 1.2 91/07/25 01:07:33 rws Exp $ */

/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 * 
 * This file is a component of an X Window System-specific implementation
 * of XCMS based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * software and its documentation for any purpose and without fee, provided
 * that this copyright, permission, and disclaimer notice is reproduced in
 * all copies of this software and in supporting documentation.  TekColor
 * is a trademark of Tektronix, Inc.
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
 *		CIELuvGcL.c
 *
 *	DESCRIPTION
 *		Source for XcmsCIELuvClipL() gamut compression routine.
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *	EXTERNS
 */
extern Status _XcmsCIELuvQueryMaxLCRGB();


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuvClipL - Return the closest L*
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIELuvClipL (ccc, pColors_in_out, nColors, i, pCompressed)
    XcmsCCC ccc;
    XcmsColor *pColors_in_out;
    unsigned int nColors;
    unsigned int i;
    Bool *pCompressed;
/*
 *	DESCRIPTION
 *		Return the closest L* for a specific hue and chroma.
 *		This routine takes any color as input and outputs 
 *		a CIE XYZ color.
 *
 *		Since this routine works with the L* within
 *		pColor_in_out intermediate results may be returned
 *		even though it may be invalid.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *              XcmsSuccess - Succeeded
 *
 */
{
    XcmsCCCRec	myCCC;
    XcmsColor	*pColor;
    XcmsColor   Luv_max;
    XcmsFloat	hue, chroma, maxChroma;
    Status retval;

    /* Use my own CCC */
    bcopy ((char *)ccc, (char *)&myCCC, sizeof(XcmsCCCRec));
    myCCC.clientWhitePt.format = XcmsUndefinedFormat;/* Inherit Screen WP */
    myCCC.gamutCompProc = (XcmsCompressionProc)NULL;/* no gamut compression */

    /*
     * Color specification passed as input can be assumed to:
     *	1. Be in XcmsCIEXYZFormat
     *	2. Already be white point adjusted for the Screen White Point.
     *	    This means that the white point now associated with this
     *	    color spec is the Screen White Point (even if the
     *	    ccc->clientWhitePt differs).
     */

    pColor = pColors_in_out + i;
    
    if (ccc->visual->class < StaticColor) {
	/*
	 * GRAY !
	 */
	return(XcmsFailure);
    } else {
	/* Convert from CIEXYZ to CIE L*u*v* format */
	if (_XcmsDIConvertColors(&myCCC, pColor,
		ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIELuvFormat)
		== XcmsFailure) {
	    return(XcmsFailure);
	}

	hue = XCMS_CIELUV_PMETRIC_HUE(pColor->spec.CIELuv.u_star,
				      pColor->spec.CIELuv.v_star);
	chroma = XCMS_CIELUV_PMETRIC_CHROMA(pColor->spec.CIELuv.u_star,
					    pColor->spec.CIELuv.v_star);
	/* Step 1: compute the maximum L* and chroma for this hue. */
	/*         This copy may be overkill but it preserves the pixel etc. */
	bcopy((char *)pColor, (char *)&Luv_max, sizeof(XcmsColor));
	if (_XcmsCIELuvQueryMaxLCRGB (&myCCC, hue, &Luv_max,
		(XcmsRGBi *)NULL) == XcmsFailure) {
	    return (XcmsFailure);
	}
	maxChroma = XCMS_CIELUV_PMETRIC_CHROMA(Luv_max.spec.CIELuv.u_star,
					       Luv_max.spec.CIELuv.v_star);

	/* Now check and return the appropriate L* */
	if (chroma == maxChroma) {
	    /* When the chroma input is equal to the maximum chroma */
	    /* merely return the L* for that chroma. */
	    bcopy((char *)&Luv_max, (char *)pColor, sizeof(XcmsColor));
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		    ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIEXYZFormat);
	} else if (chroma > maxChroma) {
	    /* When the chroma input is greater than the maximum chroma */
	    /* merely return the L* and chroma for the given hue. */
	    bcopy((char *)&Luv_max, (char *)pColor, sizeof(XcmsColor));
	    return (XcmsFailure);
	} else if (pColor->spec.CIELuv.L_star < Luv_max.spec.CIELuv.L_star) {
	    /* Find the minimum lightness for the given chroma. */  
	    if (pColor->format != XcmsCIELuvFormat) {
		if (_XcmsDIConvertColors(ccc, pColor,
			ScreenWhitePointOfCCC(ccc), 1, XcmsCIELuvFormat)
			== XcmsFailure) {
		    return(XcmsFailure);
		}
	    }
	    if (XcmsCIELuvQueryMinL(&myCCC, degrees(hue), chroma, pColor)
		== XcmsFailure) {
		    return (XcmsFailure);
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		           ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIEXYZFormat);
	} else {
	    /* Find the maximum lightness for the given chroma. */
	    if (pColor->format != XcmsCIELuvFormat) {
		if (_XcmsDIConvertColors(ccc, pColor,
			      ScreenWhitePointOfCCC(ccc), 1, XcmsCIELuvFormat)
			== XcmsFailure) {
		    return(XcmsFailure);
		}
	    }
	    if (XcmsCIELuvQueryMaxL(&myCCC, degrees(hue), chroma, pColor)
		== XcmsFailure) {
		    return (XcmsFailure);
	    }
	    retval = _XcmsDIConvertColors(&myCCC, pColor,
		           ScreenWhitePointOfCCC(&myCCC), 1, XcmsCIEXYZFormat);
	}
	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
	return(retval);
    }
}
