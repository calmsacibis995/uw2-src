/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *                 European Language Supplement 4.2 
 *                    Open Look Input Method
 *
 * Copyright (c) 1992 UNIX System Laboratories, Inc.
 * All Rights Reserved 
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
 * UNIX System Laboratories, Inc.
 * The copyright notice above does not evidence any
 * actual or intended publication of such source code.
 *
 * 
 *
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlGetImV.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>


#define bcopy(src, dst, len)    OlMemMove(char, dst, src, len)


/*
 * OlGetImValues()   : Get attributes of the OLIM.
 *                 In: Pointer to the OL Input Method.
 *                     Pointer to the attributes list.
 *                Out: Attributes of OLIM in the attributes list.
 *             Return: Nothing.
 *             Errors: None.
 *          Algorithm: The info is in the OlIm structure,
 *                     just get it.
 */
void
OlGetImValues(ol_im_p, im_values_list)
    OlIm	*ol_im_p;
    OlImValues	*im_values_list;
{
    OlImStyles	*styles;
    OlImStyle	*style_array;

    /* null terminate the list in case nothing returned */
    im_values_list[0].attr_name = (char *) NULL;
    im_values_list[0].attr_value = (void *) NULL;

    if ( ol_im_p == NULL ) 
       return;

    if ( (styles = (OlImStyles *) XtMalloc(sizeof(OlImStyles))) == NULL )
       return;

    if ( (style_array = (OlImStyle *)
                        XtMalloc((ol_im_p->im_styles.styles_count + 1) *
                                  sizeof(OlImStyle))) == NULL )
    {
	XtFree((char *) styles);
	return;
    }

    /* at the moment only one attribute defined for IM - input styles */
    styles->styles_count = ol_im_p->im_styles.styles_count;

    if ( styles->styles_count > 0 )
    {
       bcopy(ol_im_p->im_styles.supported_styles, 
             style_array, 
             styles->styles_count*sizeof(OlImStyle));

       styles->supported_styles = style_array;
       im_values_list[0].attr_name = OlNinputStyles;
       im_values_list[0].attr_value = (void *)styles;

       /* null terminate the list */
       im_values_list[1].attr_name = NULL;
       im_values_list[1].attr_value = NULL;
     }
     else
     {
	XtFree((char *) styles);
	XtFree((char *) style_array);
     }
 

     return;
}
