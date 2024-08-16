/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:motifgif/image.c	1.1"
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2
 */
#include "pictglob.h"

Resize(w,h)
int w,h;
{
    int  ix,iy,ex,ey;
    byte *ximag,*ilptr,*ipptr,*elptr,*epptr;
    static char *rstr = "Resizing Image.  Please wait...";

    /* warning:  this code'll only run machines where int=32-bits */

    if (w==iWIDE && h==iHIGH) {		/* very special case */
        if (expImage != theImage) {
            if (expImage) XDestroyImage(expImage);
            expImage = theImage;
            eWIDE = iWIDE;  eHIGH = iHIGH;
            }
        }

    else {				/* have to do some work */
        /* if it's a big image, this'll take a while.  mention it */
        if (w*h>(500*500)) {
            XDrawImageString(theDisp,mainW,theGC,CENTERX(mfinfo,w/2,rstr),
                  CENTERY(mfinfo,h/2),rstr, strlen(rstr));
            XFlush(theDisp);
            }

	/* first, kill the old expImage, if one exists */
	if (expImage && expImage != theImage) {
            free(expImage->data);  expImage->data = NULL;
            XDestroyImage(expImage);
            }

        /* create expImage of the appropriate size */
        
        eWIDE = w;  eHIGH = h;
        ximag = (byte *) malloc(w*h);
        expImage = XCreateImage(theDisp,theVisual,8,ZPixmap,0,(char *)ximag,
                        eWIDE,eHIGH,8,eWIDE);

        if (!ximag || !expImage) {
            fprintf(stderr,"ERROR: unable to create a %dx%d image\n",w,h);
            exit(0);
            }

        elptr = epptr = (byte *) expImage->data;

        for (ey=0;  ey<eHIGH;  ey++, elptr+=eWIDE) {
            iy = (iHIGH * ey) / eHIGH;
            epptr = elptr;
            ilptr = (byte *) theImage->data + (iy * iWIDE);
            for (ex=0;  ex<eWIDE;  ex++,epptr++) {
                ix = (iWIDE * ex) / eWIDE;
                ipptr = ilptr + ix;
                *epptr = *ipptr;
                }
            }
        }
}
