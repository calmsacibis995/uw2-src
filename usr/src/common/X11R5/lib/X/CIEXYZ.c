/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:CIEXYZ.c	1.2"

/* $XConsortium: CIEXYZ.c,v 1.6 91/07/25 01:07:49 rws Exp $" */

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
 *
 *	NAME
 *		CIEXYZ.c
 *
 *	DESCRIPTION
 *		CIE XYZ Color Space
 *
 *
 */

#include <X11/Xos.h>
#include "Xlibint.h"
#include "Xcmsint.h"


/*
 *      EXTERNS
 *              External declarations required locally to this package
 *              that are not already declared in any of the included header
 *		files (external includes or internal includes).
 */
extern char XcmsCIEXYZ_prefix[];

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
 *      FORWARD DECLARATIONS
 */
static int CIEXYZ_ParseString();
Status XcmsCIEXYZ_ValidSpec();


/*
 *      LOCALS VARIABLES
 */

static XcmsConversionProc Fl_CIEXYZ_to_CIEXYZ[] = {
    NULL
};



/*
 *      GLOBALS
 *              Variables declared in this package that are allowed
 *		to be used globally.
 */
    /*
     * CIE XYZ Color Space
     */
XcmsColorSpace	XcmsCIEXYZColorSpace =
    {
	XcmsCIEXYZ_prefix,		/* prefix */
	XcmsCIEXYZFormat,		/* id */
	CIEXYZ_ParseString,	/* parseString */
	Fl_CIEXYZ_to_CIEXYZ,	/* to_CIEXYZ */
	Fl_CIEXYZ_to_CIEXYZ,	/* from_CIEXYZ */
	1
    };


/************************************************************************
 *									*
 *			PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		CIEXYZ_ParseString
 *
 *	SYNOPSIS
 */
static int
CIEXYZ_ParseString(spec, pColor)
    register char *spec;
    XcmsColor *pColor;
/*
 *	DESCRIPTION
 *		This routines takes a string and attempts to convert
 *		it into a XcmsColor structure with XcmsCIEXYZFormat.
 *		The assumed CIEXYZ string syntax is:
 *		    CIEXYZ:<X>/<Y>/<Z>
 *		Where X, Y, and Z are in string input format for floats
 *		consisting of:
 *		    a. an optional sign
 *		    b. a string of numbers possibly containing a decimal point,
 *		    c. an optional exponent field containing an 'E' or 'e'
 *			followed by a possibly signed integer string.
 *
 *	RETURNS
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
    if (strncmp(spec, XcmsCIEXYZ_prefix, n) != 0) {
	return(XcmsFailure);
    }

    /*
     * Attempt to parse the value portion.
     */
    if (sscanf(spec + n + 1, "%lf/%lf/%lf",
	    &pColor->spec.CIEXYZ.X,
	    &pColor->spec.CIEXYZ.Y,
	    &pColor->spec.CIEXYZ.Z) != 3) {
	return(XcmsFailure);
    }
    pColor->format = XcmsCIEXYZFormat;
    pColor->pixel = 0;
    return(XcmsCIEXYZ_ValidSpec(pColor));
}


/************************************************************************
 *									*
 *			PUBLIC ROUTINES 				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELab_ValidSpec
 *
 *	SYNOPSIS
 */
Status
XcmsCIEXYZ_ValidSpec(pColor)
    XcmsColor *pColor;
/*
 *	DESCRIPTION
 *		Checks if color specification valid for CIE XYZ
 *
 *	RETURNS
 *		XcmsFailure if invalid,
 *		XcmsSuccess if valid.
 *
 */
{
    if (pColor->format != XcmsCIEXYZFormat
	    ||
	    (pColor->spec.CIEXYZ.Y < 0.0 - XMY_DBL_EPSILON)
	    ||
	    (pColor->spec.CIEXYZ.Y > 1.0 + XMY_DBL_EPSILON)) {
	return(XcmsFailure);
    }
    return(XcmsSuccess);
}
