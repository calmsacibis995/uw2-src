/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:wallpaper/dtsetbg.c	1.2"

/******************************file*header********************************

    Description: xsetbg.c
	This file contains code to set the background color on the 
	root window based on the currently selected color palette.

	return value:
	0 successfully determined color
	1 could not determine background color
*/


#include <Xm/ColorObj.h>

main(int argc, char **argv)
{
    short 	active, inactive, primary, secondary;
    int 		colorUse;
    int 		bg_index = 7;
    PixelSet 	*pPixelData;
    PixelSet 	*bg_pixel_set;
    Pixel 		bg_pixel;
    Widget		w;
    XtAppContext	AppContext;
    int 	retval = 0;
    
    w = XtAppInitialize(&AppContext,/* app_context_return   */
			"SETBG",                /* application_class    */
			(XrmOptionDescList)NULL,/* options              */
			(Cardinal)0,		/* num_options		*/		
			&argc,			/* argc_in_out          */
			argv,                   /* argv_in_out          */
			(String *) NULL,        /* fallback_resources   */
			(ArgList) NULL,         /* args                 */
			(Cardinal) 0            /* num_args             */
			);
    
    if (pPixelData = (PixelSet *)XtMalloc (NUM_COLORS * sizeof(PixelSet))) {
	
	/*
	 *
	 * ASSUMPTION:  If _XmGetPixelData() returns true,
	 * we have a good color server at our disposal.
	 */
	if (_XmGetPixelData(DefaultScreen(XtDisplay(w)), &colorUse, 
			    pPixelData, &active, &inactive, 
			    &primary, &secondary)) {
	    bg_pixel_set = pPixelData + bg_index;
	    bg_pixel     = bg_pixel_set->bg;
	    XSetWindowBackground(XtDisplay(w), 
				 RootWindowOfScreen(XtScreen(w)), bg_pixel);
	    XClearWindow(XtDisplay(w), 
			 RootWindowOfScreen(XtScreen(w)));
	    XSync(XtDisplay(w), True);
	}
	else
	    retval = 1;


    }
    else
	retval = 1;
    exit(retval);
}
