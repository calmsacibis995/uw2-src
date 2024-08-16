/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:imagetypes.c	1.2"
/* imagetypes.c:
 *
 * this contains things which reference the global ImageTypes array
 *
 * jim frost 09.27.89
 *
 * Copyright 1989 Jim Frost.  See included file "copyright.h" for complete
 * copyright information.
 */

#include "copyright.h"
#include "image.h"
#include "imagetypes.h"
#include <errno.h>

extern int errno;

/* load a named image
 */

Image *loadImage(name, verbose)
     char         *name;
     unsigned int  verbose;
{ char   fullname[BUFSIZ];
  Image *image;
  int    a;

  if (findImage(name, fullname) < 0) {
    if (errno == ENOENT)
      printf("%s: image not found\n", name);
    else
      perror(fullname);
    return(NULL);
  }
  for (a= 0; ImageTypes[a].loader; a++)
    if (image= ImageTypes[a].loader(fullname, name, verbose))
      return(image);
  printf((char *)gettxt("xloadimage:1", "%s: unknown or unsupported image type\n"),
	 fullname);
  return(NULL);
}

/* identify what kind of image a named image is
 */

int identifyImage(name)
     char *name;
{ char fullname[BUFSIZ];
  int  a;

  if (findImage(name, fullname) < 0) {
    if (errno == ENOENT)
      printf((char *)gettxt("xloadimage:3", "%s: image not found\n"), name);
    else
      perror(fullname);
    return(1);
  }
  for (a= 0; ImageTypes[a].identifier; a++)
    if (ImageTypes[a].identifier(fullname, name))
      return(0);
  printf((char *)gettxt("xloadimage:1", "%s: unknown or unsupported image type\n"),
	 fullname);
  return(1);
}

/* tell user what image types we support
 */

void supportedImageTypes()
{ int a;

  printf((char *)gettxt("xloadimage:2", "Image types supported:\n"));
  for (a= 0; ImageTypes[a].name; a++)
    printf("  %s\n", ImageTypes[a].name);
}

void goodImage(image, func)
     Image *image;
     char  *func;
{
  if (!image) {
    printf("%s: nil image\n", func);
    exit(0);
  }
  switch (image->type) {
  case IBITMAP:
  case IRGB:
    break;
  default:
    printf("%s: bad destination image\n", func);
    exit(0);
  }
}
