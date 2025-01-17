/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:CIEuvY.c	1.2"

/* $XConsortium: CIEuvY.c,v 1.7 91/07/25 01:07:51 rws Exp $" */

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
 *
 *	NAME
 *		CIEuvy.c
 *
 *	DESCRIPTION
 *		This file contains routines that support the CIE u'v'Y
 *		color space to include conversions to and from the CIE
 *		XYZ space.
 *
 *	DOCUMENTATION
 *		"TekColor Color Management System, System Implementor's Manual"
 */

#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xcmsint.h"


/*
 *	EXTERNS
 */
extern char	XcmsCIEuvY_prefix[];


/*
 *	FORWARD DECLARATIONS
 */

static int CIEuvY_ParseString();
Status XcmsCIEuvY_ValidSpec();
/*
 *	DEFINES
 *		Internal definitions that need NOT be exported to any package
 *		or program using this package.
 */
#ifdef DBL_EPSILON
#  define XMY_DBL_EPSILON DBL_EPSILON
#else
#  define XMY_DBL_EPSILON 0.00001
#endif


/*
 *	LOCAL VARIABLES
 */

    /*
     * NULL terminated list of functions applied to get from CIEuvY to CIEXYZ
     */
static XcmsConversionProc Fl_CIEuvY_to_CIEXYZ[] = {
    XcmsCIEuvYToCIEXYZ,
    NULL
};

    /*
     * NULL terminated list of functions applied to get from CIEXYZ to CIEuvY
     */
static XcmsConversionProc Fl_CIEXYZ_to_CIEuvY[] = {
    XcmsCIEXYZToCIEuvY,
    NULL
};


/*
 *	GLOBALS
 */

    /*
     * CIE uvY Color Space
     */
XcmsColorSpace	XcmsCIEuvYColorSpace =
    {
	XcmsCIEuvY_prefix,		/* prefix */
	XcmsCIEuvYFormat,		/* id */
	CIEuvY_ParseString,	/* parseString */
	Fl_CIEuvY_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_CIEuvY,	/* from_CIEXYZ */
	1
    };



/************************************************************************
 *									*
 *			 PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CIEuvY_ParseString
 *
 *	SYNOPSIS
 */
static int
CIEuvY_ParseString(spec, pColor)
    register char *spec;
    XcmsColor *pColor;
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsCIEuvYFormat.
 *		The assumed CIEuvY string syntax is:
 *		    CIEuvY:<u>/<v>/<Y>
 *		Where u, v, and Y are in string input format for floats
 *		consisting of:
 *		    a. an optional sign
 *		    b. a string of numbers possibly containing a decimal point,
 *		    c. an optional exponent field containing an 'E' or 'e'
 *			followed by a possibly signed integer string.
 *
 *	RETURNS
 *		0 if failed, non-zero otherwise.
 */
{
    int n;
    char *pchar;

    if ((pchar = strchr(spec, ':')) == NULL) {
	return(XcmsFailure);
    }
    n = (int)(pchar - spec);

    /*
     * Check for proper prefix.
     */
    if (strncmp(spec, XcmsCIEuvY_prefix, n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.CIEuvY.u_prime,
	    &pColor->spec.CIEuvY.v_prime,
	    &pColor->spec.CIEuvY.Y) != 3) {
	return(XcmsFailure);
    }
    pColor->format = XcmsCIEuvYFormat;
    pColor->pixel = 0;
    return(XcmsCIEuvY_ValidSpec(pColor));
}


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIEuvY_ValidSpec
 *
 *	SYNOPSIS
 */
Status
XcmsCIEuvY_ValidSpec(pColor)
    XcmsColor *pColor;
/*
 *	DESCRIPTION
 *		Checks if color specification valid for CIE u'v'Y.
 *
 *	RETURNS
 *		XcmsFailure if invalid,
 *		XcmsSuccess if valid.
 *
 */
{
    if (pColor->format != XcmsCIEuvYFormat
	    ||
	    (pColor->spec.CIEuvY.Y < 0.0 - XMY_DBL_EPSILON)
	    ||
	    (pColor->spec.CIEuvY.Y > 1.0 + XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIEuvYToCIEXYZ - convert CIEuvY to CIEXYZ
 *
 *	SYNOPSIS
 */
Status
XcmsCIEuvYToCIEXYZ(ccc, puvY_WhitePt, pColors_in_out, nColors)
    XcmsCCC ccc;
    XcmsColor *puvY_WhitePt;
    XcmsColor *pColors_in_out;
    unsigned int nColors;
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIEuvY format to CIEXYZ format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsCIEXYZ XYZ_return;
    XcmsColor whitePt;
    int i;
    XcmsColor *pColor = pColors_in_out;
    XcmsFloat div, x, y, z, Y;

    /*
     * Check arguments
     *	Postpone checking puvY_WhitePt until it is actually needed
     *	otherwise converting between XYZ and uvY will fail.
     */
    if (pColors_in_out == NULL) {
	return(XcmsFailure);
    }


    /*
     * Now convert each XcmsColor structure to CIEXYZ form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is CIEuvY */
	if (!XcmsCIEuvY_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/*
	 * Convert to CIEXYZ
	 */

	Y = pColor->spec.CIEuvY.Y;

	/* Convert color u'v' to xyz space */
	div = (6.0 * pColor->spec.CIEuvY.u_prime) - (16.0 * pColor->spec.CIEuvY.v_prime) + 12.0;
	if (div == 0.0) {
	    /* use white point since div == 0 */
	    if (puvY_WhitePt == NULL ) {
		return(XcmsFailure);
	    }
	    /*
	     * Make sure white point is in CIEuvY form
	     */
	    if (puvY_WhitePt->format != XcmsCIEuvYFormat) {
		/* Make copy of the white point because we're going to modify it */
		bcopy((char *)puvY_WhitePt, (char *)&whitePt, sizeof(XcmsColor));
		if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL, 1,
			XcmsCIEuvYFormat)) {
		    return(XcmsFailure);
		}
		puvY_WhitePt = &whitePt;
	    }
	    /* Make sure it is a white point, i.e., Y == 1.0 */
	    if (puvY_WhitePt->spec.CIEuvY.Y != 1.0) {
		return(XcmsFailure);
	    }
	    div = (6.0 * puvY_WhitePt->spec.CIEuvY.u_prime) -
		    (16.0 * puvY_WhitePt->spec.CIEuvY.v_prime) + 12.0;
	    if (div == 0) {
		/* internal error */
		return(XcmsFailure);
	    }
	    x = 9.0 * puvY_WhitePt->spec.CIEuvY.u_prime / div;
	    y = 4.0 * puvY_WhitePt->spec.CIEuvY.v_prime / div;
	} else {
	    x = 9.0 * pColor->spec.CIEuvY.u_prime / div;
	    y = 4.0 * pColor->spec.CIEuvY.v_prime / div;
	}
	z = 1.0 - x - y;

	/* Convert from xyz to XYZ */
	/* Conversion uses color normalized lightness based on Y */
	if (y != 0.0) {
	    XYZ_return.X = x * Y / y;
	} else {
	    XYZ_return.X = x;
	}
	XYZ_return.Y = Y;
	if (y != 0.0) {
	    XYZ_return.Z = z * Y / y;
	} else {
	    XYZ_return.Z = z;
	}

	bcopy((char *)&XYZ_return, (char *)&pColor->spec.CIEXYZ, sizeof(XcmsCIEXYZ));
	/* Identify that format is now CIEXYZ */
	pColor->format = XcmsCIEXYZFormat;
    }

    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsCIEXYZToCIEuvY - convert CIEXYZ to CIEuvY
 *
 *	SYNOPSIS
 */
Status
XcmsCIEXYZToCIEuvY(ccc, puvY_WhitePt, pColors_in_out, nColors)
    XcmsCCC ccc;
    XcmsColor *puvY_WhitePt;
    XcmsColor *pColors_in_out;
    unsigned int nColors;
/*
 *	DESCRIPTION
 *		Converts color specifications in an array of XcmsColor
 *		structures from CIEXYZ format to CIEuvY format.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded.
 *
 */
{
    XcmsCIEuvY uvY_return;
    XcmsColor whitePt;
    int i;
    XcmsColor *pColor = pColors_in_out;
    XcmsFloat div;

    /*
     * Check arguments
     *	Postpone checking puvY_WhitePt until it is actually needed
     *	otherwise converting between XYZ and uvY will fail.
     */
    if (pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Now convert each XcmsColor structure to CIEuvY form
     */
    for (i = 0; i < nColors; i++, pColor++) {

	/* Make sure original format is CIEXYZ */
	if (!XcmsCIEXYZ_ValidSpec(pColor)) {
	    return(XcmsFailure);
	}

	/* Convert to CIEuvY */
	div = pColor->spec.CIEXYZ.X + (15.0 * pColor->spec.CIEXYZ.Y) +
		(3.0 * pColor->spec.CIEXYZ.Z);
	if (div == 0.0) {
	    /* Use white point since div == 0.0 */
	    if (puvY_WhitePt == NULL ) {
		return(XcmsFailure);
	    }
	    /*
	     * Make sure white point is in CIEuvY form
	     */
	    if (puvY_WhitePt->format != XcmsCIEuvYFormat) {
		/* Make copy of the white point because we're going to modify it */
		bcopy((char *)puvY_WhitePt, (char *)&whitePt, sizeof(XcmsColor));
		if (!_XcmsDIConvertColors(ccc, &whitePt, (XcmsColor *)NULL, 1,
			XcmsCIEuvYFormat)) {
		    return(XcmsFailure);
		}
		puvY_WhitePt = &whitePt;
	    }
	    /* Make sure it is a white point, i.e., Y == 1.0 */
	    if (puvY_WhitePt->spec.CIEuvY.Y != 1.0) {
		return(XcmsFailure);
	    }
	    uvY_return.Y = pColor->spec.CIEXYZ.Y;
	    uvY_return.u_prime = puvY_WhitePt->spec.CIEuvY.u_prime;
	    uvY_return.v_prime = puvY_WhitePt->spec.CIEuvY.v_prime;
	} else {
	    uvY_return.u_prime = 4.0 * pColor->spec.CIEXYZ.X / div;
	    uvY_return.v_prime = 9.0 * pColor->spec.CIEXYZ.Y / div;
	    uvY_return.Y = pColor->spec.CIEXYZ.Y;
	}

	bcopy((char *)&uvY_return, (char *)&pColor->spec.CIEuvY, sizeof(XcmsCIEuvY));
	/* Identify that format is now CIEuvY */
	pColor->format = XcmsCIEuvYFormat;
    }

    return(XcmsSuccess);
}
