/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XcmsQRed.c	1.2"

/* $XConsortium: XcmsQRed.c,v 1.2 91/06/07 09:56:49 rws Exp $ */

/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 * 
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
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
 *		XcmsQRed.c - Query Red
 *
 *	DESCRIPTION
 *		Routine to obtain a color specification for full
 *		red intensity and zero  green and blue intensities.
 *
 *
 */

#include "Xlibint.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsQueryRed
 *
 *	SYNOPSIS
 */

Status
XcmsQueryRed(ccc, target_format, pColor_ret)
    XcmsCCC ccc;
    XcmsColorFormat target_format;
    XcmsColor *pColor_ret;
/*
 *	DESCRIPTION
 *		Returns the color specification in the target format for
 *		full intensity red and zero intensity green and blue.
 *
 *	RETURNS
 *		Returns XcmsSuccess, if failed; otherwise XcmsFailure
 *
 */
{
    XcmsColor tmp;

    tmp.format = XcmsRGBiFormat;
    tmp.pixel = 0;
    tmp.spec.RGBi.red = 1.0;
    tmp.spec.RGBi.green = 0.0;
    tmp.spec.RGBi.blue = 0.0;
    if (XcmsConvertColors(ccc, &tmp, 1, target_format, NULL) != XcmsSuccess) {
	return(XcmsFailure);
    }
    bcopy((char *)&tmp, (char *)pColor_ret, sizeof(XcmsColor));
    return(XcmsSuccess);
}
