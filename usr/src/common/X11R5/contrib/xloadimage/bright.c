/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)r4xloadimage:bright.c	1.2"
/* bright.c
 *
 * alter an image's brightness by a given percentage
 *
 * jim frost 10.10.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"

void brighten(image, percent, verbose)
     Image        *image;
     unsigned int  percent;
     unsigned int  verbose;
{ int          a;
  unsigned int newrgb;
  float        fperc;

  if (! RGBP(image)) /* we're AT&T */
    return;

  if (verbose) {
    printf("  Brightening colormap by %d%%...", percent);
    fflush(stdout);
  }

  fperc= (float)percent / 100.0;
  for (a= 0; a < image->rgb.used; a++) {
    newrgb= *(image->rgb.red + a) * (int)fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.red + a)= newrgb;
    newrgb= *(image->rgb.green + a) * (int)fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.green + a)= newrgb;
    newrgb= *(image->rgb.blue + a) * (int)fperc;
    if (newrgb > 65535)
      newrgb= 65535;
    *(image->rgb.blue + a)= newrgb;
  }

  if (verbose)
    printf("done\n");
}
