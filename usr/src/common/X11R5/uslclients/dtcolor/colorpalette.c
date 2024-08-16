/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colorpalette.c	1.2"
/*
 *
 *
 *    File:        ColorPalette.c
 * 
 *    Project:     DT 3.0
 * 
 *    Description: Controls the Dtstyle Color Palette data
 * 
 * 
 *   (c) Copyright Hewlett-Packard Company, 1990.  
 * 
 * 
 * 
 */
/* $Revision: 1.4 $ */

/* include files                         */

#include <X11/Xlib.h>

#include <Xm/Xm.h>

#include "main.h"
#include "colormain.h"

/* include extern functions              */
#include "colorpalette.h"

/* Internal Variables                    */
extern Pixmap BGPixmap;


/*
 *  Allocate colors for the MAX_NUM_COLOR color sets within a palette, 
 *  if HIGH_COLOR. Will have to do major checking for low color
 *  moitors.  Setting the allocated pixels to pCurrentPalette.
 *
 *  NOTE: for now I am allocating them all, In future we will want to
 *        be more careful about our allocations.
 */
Bool 
#ifdef _NO_PROTO
AllocatePaletteCells( shell )
        Widget shell ;
#else
AllocatePaletteCells(
        Widget shell )
#endif /* _NO_PROTO */
{
	int 	    i;
	int         colorUse;
	PixelSet    pixels[8];
	int         j = 0;
	XColor      colors[MAX_NUM_COLORS * 5];


	_XmGetPixelData (style.screenNum, &colorUse, pixels,
			   &(pCurrentPalette->active),
			   &(pCurrentPalette->inactive),
			   &(pCurrentPalette->primary),
			   &(pCurrentPalette->secondary));

	for (i = 0; i < MAX_NUM_COLORS; i++) {
		pCurrentPalette->color[i].bg.pixel = pixels[i].bg;
		pCurrentPalette->color[i].sc.pixel = pixels[i].sc;
		pCurrentPalette->color[i].fg.pixel = pixels[i].fg;
		pCurrentPalette->color[i].ts.pixel = pixels[i].ts;
		pCurrentPalette->color[i].bs.pixel = pixels[i].bs;

		if (style.dynamicColor == 0) 
			continue;
		pCurrentPalette->color[i].bg.flags = DoRed | DoGreen | DoBlue;
		pCurrentPalette->color[i].fg.flags = DoRed | DoGreen | DoBlue;
		pCurrentPalette->color[i].ts.flags = DoRed | DoGreen | DoBlue;
		pCurrentPalette->color[i].bs.flags = DoRed | DoGreen | DoBlue;
		pCurrentPalette->color[i].sc.flags = DoRed | DoGreen | DoBlue;

		if (i < pCurrentPalette->num_of_colors) {
			colors[j++] =  pCurrentPalette->color[i].bg;
			colors[j++] =  pCurrentPalette->color[i].sc;

			if (FgColor == DYNAMIC)
				colors[j++] =  pCurrentPalette->color[i].fg;

			if (!UsePixmaps) {
				colors[j++] =  pCurrentPalette->color[i].ts;
				colors[j++] =  pCurrentPalette->color[i].bs;
			}
		}
	}

	if (style.dynamicColor)
		XStoreColors(style.display, style.colormap, colors, j );

	return(True);
}


#define DOALL  (DoRed | DoGreen | DoBlue)
/*
 *  ReColorPalette 
 *	changes to RGB values of the already allocated pixels
 *  	for the 8 color buttons.  Each color button uses 5 pixels (at least
 *  	for now.)
 *
 *  	The palette passed has the colors the pixels are going to change to.
 *
 */
int 
#ifdef _NO_PROTO
ReColorPalette()
#else
ReColorPalette( void )
#endif /* _NO_PROTO */
{
	int              i;
	int              j = 0;
	XColor           colors[MAX_NUM_COLORS * 5];

	pCurrentPalette->primary = pOldPalette->primary;
	pCurrentPalette->secondary = pOldPalette->secondary;
	pCurrentPalette->active = pOldPalette->active;
	pCurrentPalette->inactive = pOldPalette->inactive;
	for (i = 0; i < MAX_NUM_COLORS; i++) {

		pCurrentPalette->color[i].bg.pixel = 
		 				pOldPalette->color[i].bg.pixel; 
		if (TypeOfMonitor != B_W) {
			pCurrentPalette->color[i].bg.flags = DOALL; 
			if (i < pCurrentPalette->num_of_colors)
				colors[j++] =  pCurrentPalette->color[i].bg;
		}

		pCurrentPalette->color[i].sc.pixel = 
				pOldPalette->color[i].sc.pixel; 
		if (TypeOfMonitor != B_W) {
			pCurrentPalette->color[i].sc.flags = DOALL; 
			if (i < pCurrentPalette->num_of_colors)
				colors[j++] =  pCurrentPalette->color[i].sc;
		}

		pCurrentPalette->color[i].fg.pixel = 
				pOldPalette->color[i].fg.pixel; 
		if (TypeOfMonitor != B_W) {
			if (FgColor == DYNAMIC) {
				pCurrentPalette->color[i].fg.flags = DOALL; 
				if (i < pCurrentPalette->num_of_colors)
					colors[j++] =  
						pCurrentPalette->color[i].fg;
			}
		}

		pCurrentPalette->color[i].ts.pixel = 
				pOldPalette->color[i].ts.pixel; 
		if (TypeOfMonitor != B_W) {
			if (UsePixmaps == FALSE) {
				pCurrentPalette->color[i].ts.flags = DOALL; 
				if (i < pCurrentPalette->num_of_colors)
					colors[j++] =  pCurrentPalette->color[i].ts;
			}
		}

		pCurrentPalette->color[i].bs.pixel = 
				pOldPalette->color[i].bs.pixel; 
		if (TypeOfMonitor != B_W) {
			if (UsePixmaps == FALSE) {
				pCurrentPalette->color[i].bs.flags = DOALL; 
				if (i < pCurrentPalette->num_of_colors) 
					colors[j++] =  pCurrentPalette->color[i].bs;
			}
		}
	}

	if (TypeOfMonitor != B_W)
		XStoreColors(style.display, style.colormap, colors, j );

	return(True);
}


/*  
 * CheckMonitor  
 *	querry color server for monitor type
 */
void 
#ifdef _NO_PROTO
CheckMonitor( shell )
        Widget shell ;
#else
CheckMonitor(
        Widget shell )
#endif /* _NO_PROTO */
{
	WaitSelection = TRUE;

	XtGetSelectionValue(shell, XA_CUSTOMIZE, XA_TYPE_MONITOR, 
			    show_selection, (XtPointer)GET_TYPE_MONITOR, 
			    CurrentTime);

	XFlush(style.display);

	while (WaitSelection)
		XtAppProcessEvent(XtWidgetToApplicationContext(shell), XtIMAll);
}
