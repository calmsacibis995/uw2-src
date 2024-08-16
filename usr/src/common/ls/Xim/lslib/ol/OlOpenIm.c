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
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlOpenIm.c	1.3"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "messages.h"


/* external OlIm functions used by this module */
extern void	OlCloseIm();

/* utilities used by this module */
extern char	*GetTxt();

/*
 * OlOpenIm()   : Open OL Input Method.
 *            In: Pointer to display where to open IM.
 *                Resource database.
 *                Resource name.
 *                Resource class.
 *           Out: Nothing.
 *        Return: Pointer to opened IM or NULL on error.
 *        Errors: see above.
 *     Algorithm: Prepare the OlIm structure, then call XIM and
 *                on success fill the data with XIM info.
 */

OlIm *
OlOpenIm(dpy, rdb, res_name, res_class)
    Display		*dpy;
    XrmDatabase		rdb;
    String		res_name, res_class;
{
    
    OlIm *ol_im_p;
    static OlImStyle *ol_styles_array;
    static XIMStyles *xim_supported_styles_p;



    if ( dpy == NULL ) 
       return((OlIm *) NULL);


    if ( (ol_im_p = (OlIm *)XtCalloc(1, sizeof(OlIm))) == NULL )
       return ((OlIm *) NULL);

    /* set the locale modifiers - this is needed by XIM */

    XSetLocaleModifiers(""); /* not sure this is needed */


    /* call XIM */
    if ( (ol_im_p->imtype = (void *)XOpenIM(dpy, rdb, res_name, res_class)) == NULL )
       goto open_abort;

    /* now check that IM supports at least one input style */
    XGetIMValues(ol_im_p->imtype, XNQueryInputStyle, &xim_supported_styles_p, NULL);


    if ( !xim_supported_styles_p->count_styles )
       goto styles_query_error;
    
    ol_styles_array = ol_im_p->im_styles.supported_styles = 
                      (OlImStyle *) XtMalloc(xim_supported_styles_p->count_styles * 
                      sizeof (OlImStyle));

    if ( ol_styles_array == NULL )
       goto styles_query_error;

    if ( xim_supported_styles_p->supported_styles == NULL )
    {
       ol_im_p->im_styles.styles_count = 1;
       ol_im_p->im_styles.supported_styles[0] |= OlImNeedNothing;
    }
    else
    {
       int i;

       /* convert XIM to OLIM input styles */
       for (i=0; i< (int) xim_supported_styles_p->count_styles; i++)
       {
   
           XIMStyle style;

           style = xim_supported_styles_p->supported_styles[i];
           if ( (style & XIMStatusArea) == style )
              ol_styles_array[i] |= OlImStatusArea;
   
           if ( (style & XIMStatusCallbacks) == style )
              ol_styles_array[i] |= OlImStatusCallbacks;

           /* OLIM has fewer "None"-s and "Nothing"-s, so map */
           if ( ( (style & XIMStatusNothing)  == style )  ||
                ( (style & XIMStatusNone)     == style )  ||
                ( (style & XIMPreeditNothing) == style )  ||
                ( (style & XIMPreeditNone)    == style )  )
              ol_styles_array[i] |= OlImNeedNothing;
   
           if ( (style & XIMPreeditPosition) == style)
              ol_styles_array[i] |= OlImPreEditPosition;

           if ( (style & XIMPreeditArea) == style)
              ol_styles_array[i] |= OlImPreEditArea;

           if ( (style & XIMPreeditCallbacks) == style)
              ol_styles_array[i] |= OlImPreEditCallbacks;
       }
    }
    XtFree((char *) xim_supported_styles_p);

    /* sort out resources */
    if (res_name) 
    {
       int len;

       len = strlen(res_name);
       ol_im_p->appl_name = (String) XtMalloc(len+1);
       if ( ol_im_p->appl_name == NULL ) 
          goto open_abort;
       strncpy(ol_im_p->appl_name, res_name, len);
    }
    
    if (res_class) 
    {
       int len;

       len = strlen(res_class);
       ol_im_p->appl_class = (String) XtMalloc(len+1);
       if ( ol_im_p->appl_class == NULL ) 
          goto open_abort;
       strncpy(ol_im_p->appl_class, res_class, len);
    }


    return(ol_im_p);

open_abort:
    OlCloseIm (ol_im_p);
    return ( (OlIm *) NULL);

styles_query_error:
    XtFree((char *) xim_supported_styles_p);
    XtWarning (GetTxt(CantEnquireStyles));
    return (ol_im_p);
}
