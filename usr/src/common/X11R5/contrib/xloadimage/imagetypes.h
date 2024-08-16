/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:imagetypes.h	1.3"
/* imagetypes.h:
 *
 * supported image types and the imagetypes array declaration.  when you
 * add a new image type, only the makefile and this header need to be
 * changed.
 *
 * jim frost 10.15.89
 */

Image *facesLoad();
Image *pbmLoad();
Image *sunRasterLoad();
Image *gifLoad();
Image *xbitmapLoad();
Image *xpixmapLoad();

int facesIdent();
int pbmIdent();
int sunRasterIdent();
int gifIdent();
int xbitmapIdent();
int xpixmapIdent();

/* some of these are order-dependent
 */

struct {
  int    (*identifier)(); /* print out image info if this kind of image */
  Image *(*loader)();     /* load image if this kind of image */
  char  *name;            /* name of this image format */
} ImageTypes[] = {
  sunRasterIdent, sunRasterLoad, "Sun Rasterfile",
  pbmIdent,       pbmLoad,       "Portable Bit Map (PBM)",
  facesIdent,     facesLoad,     "Faces Project",
  gifIdent,       gifLoad,       "GIF Image",
  xpixmapIdent,   xpixmapLoad,   "X Pixmap",
  xbitmapIdent,   xbitmapLoad,   "X Bitmap",
  NULL,           NULL,          NULL
};

