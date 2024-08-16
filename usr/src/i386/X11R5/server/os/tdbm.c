/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:os/tdbm.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*===========================================================================*\
|| Copyright 1988,1989,1990,1991 Pittsburgh Powercomputing Corporation (PPc)
|| of Pittsburgh, Pennsylvania as an unpublished work. All Rights Reserved.
||
|| This is unpublished proprietary trade secret source code of Pittsburgh
|| Powercomputing. The copyright above does not evidence any actual or intended
|| publication of this source code.  This source code is supplied under a
|| license.  It may be used, disclosed, and/or copied only as permitted under
|| such license agreement.  Use, copying, and/or disclosure of this source
|| code is strictly prohibited unless otherwise provided in the license
|| agreement.  Any copy must contain the copyright above and this notice must
|| be preserved in all copies of this source code.
\*===========================================================================*/
/*

 $Log:	dbm-txt.c,v $
 * Revision 1.3  91/04/29  10:16:17  ppc
 * split dbm and created libdbm
 * 
 * Revision 1.2  91/03/22  17:06:33  ppc
 * Support for dos version of the host added - ja
 * 
 * Revision 1.1.1.1  91/03/03  15:06:05  ppc
 * X-Station/340 2.1.6 distribution
 *
 * Revision 1.1  91/03/03  15:06:04  ppc
 * Initial revision
 *

*/

/* this file implements only what the X11 server needs */

/* tdbm.c: a textfile oriented functional equivalent to the dbm library. */

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include "misc.h"
#include "tdbm.h"

/*#define DBM_DEBUG/**/

#define uchar	unsigned char
#define ushort	unsigned short
#define ulong	unsigned long

#ifdef SVR4
#define CONST_CHAR_P	(const char *)
#define SIZE_T		(size_t)
#else
#define CONST_CHAR_P	(char *)
#define SIZE_T   /**/
#endif

#define FNAME_EXTENSION	".txt"
#define FNAME_PATH	"/usr/lib/X11/rgb/"
#define FNAME_EXTRASIZE	(strlen(FNAME_PATH)+strlen(FNAME_EXTENSION)+1)

#define MAXSCANCOLORLEN 80
#define MAXCOLORLEN 40
struct rgbdat {
    uchar color[MAXCOLORLEN];
    ushort size;
    ushort r,g,b;
};

struct rgbdat *rgbdata=NULL;
ulong rgb_size,rgb_used;

FILE *tdbm_fp;
char *tdbm_fname;

/*==========================================================================*/

void
case_flatten(cp)
  uchar *cp;
{
    while (*cp) {
	if (isupper(*cp))
	  *cp = tolower(*cp);
	++cp;
    }
}

int
add_rgbdata(color,r,g,b)
  uchar *color;
  unsigned long r,g,b;
{
    ulong newsize;
    struct rgbdat *rgb;

    if (rgb_size == rgb_used) { /* make space */
	rgb_size += 4096 / sizeof(struct rgbdat);
	newsize = sizeof(struct rgbdat) * rgb_size;
	if (rgbdata == NULL)
	  rgbdata=(struct rgbdat *)Xalloc(newsize);
	else
	  rgbdata=(struct rgbdat *)Xrealloc((unsigned char *)rgbdata,newsize);
	if (rgbdata == NULL) {
	    FatalError("tdbm: can't alloc %d bytes\n",newsize);
	    return(-1);
	}
    }

    rgb = &rgbdata[rgb_used];
    strncpy((char *)rgb->color, CONST_CHAR_P color, SIZE_T (MAXCOLORLEN-1));
    rgb->color[MAXCOLORLEN-1] = '\0';
    case_flatten(rgb->color);
    if (strlen(CONST_CHAR_P color) >= MAXCOLORLEN) {
	ErrorF("Color [%s] truncated to [%s]\n",color,rgb->color);
    }
    rgb->size = strlen(CONST_CHAR_P color); /* maintain original size */
    rgb->r = r << 8;
    rgb->g = g << 8;
    rgb->b = b << 8;

    ++rgb_used;
    return(0);
}

#ifdef DBM_DEBUG
list_rgbdata()
{
    struct rgbdat *rgb=rgbdata;
    int ii;

    if (rgb == NULL)
      return;

    ErrorF("rgbdata (%d entries) {\n",rgb_used);
    for (ii=0; ii < rgb_used; ++ii) {
	ErrorF("%04x %04x %04x\t[%s],%d\n",rgb->r,rgb->g,rgb->b,
	       rgb->color,rgb->size);
	++rgb;
    }
    ErrorF("} /* end-of-rgbdata */\n");
}
#endif

/*==========================================================================*/

void
dbmclose()
{
    if (rgbdata != NULL) { /* clear and initialize */
	Xfree((unsigned char *)rgbdata);
    }
    rgbdata = NULL;
    rgb_size=0;
    rgb_used=0;
}

int
dbminit(fname)
  char *fname;
{
    unsigned long r,g,b;
    uchar color[MAXSCANCOLORLEN];
    unsigned long newsize;

    dbmclose(); /* clear old data */

    newsize = sizeof(fname) + FNAME_EXTRASIZE;
    if ((tdbm_fname = (char *)Xalloc(newsize)) == NULL)
      FatalError("tdbm: dbminit can't alloc %d bytes\n",newsize);
    strcpy(tdbm_fname,fname);

#ifdef DBM_DEBUG
    ErrorF("dbminit(%s)\n",tdbm_fname);
#endif

    if((tdbm_fp = fopen(tdbm_fname,"r")) == NULL) {
	if ((fname[0] != '/') && (fname[0] != '.')) {
	    strcpy(tdbm_fname,FNAME_PATH);
	    strcat(tdbm_fname,fname);
	}
	if (strncmp(tdbm_fname,FNAME_EXTENSION,strlen(FNAME_EXTENSION))
	    != 0) {
	    strcat(tdbm_fname,FNAME_EXTENSION);
	}
	tdbm_fp = fopen(tdbm_fname,"r");
    }
    if (tdbm_fp == NULL) {
	ErrorF("dbminit: can't open [%s]\n",tdbm_fname);
	Xfree((unsigned char *)tdbm_fname);
	return(-1);
    }
#ifdef DBM_DEBUG
    ErrorF("Using RGB Database: [%s]\n",tdbm_fname);
#endif
    Xfree((unsigned char *)tdbm_fname);

    while (!feof(tdbm_fp)) {
	if (fscanf(tdbm_fp,"%li %li %li %[^\n]",&r,&g,&b,color) == 4) {
	    if (add_rgbdata(color,r,g,b) == -1)
	      return(-1);
	}
    }
#ifdef DBM_DEBUG
    list_rgbdata();
#endif
    fclose(tdbm_fp);
    return(0);
}

datum
fetch(key)
  datum key;
{
    unsigned long ii;
    struct rgbdat *rgb = rgbdata;
    static datum data;

    data.dptr = NULL;
    data.dsize = 0;

    if (rgb == NULL) {
	return(data);
    }

    for (ii = 0; ii < rgb_used; ++ii, ++rgb) {
	if ((rgb->size == (ushort)key.dsize) &&
	    !strncmp((char *)rgb->color,CONST_CHAR_P key.dptr,SIZE_T key.dsize)) {
	    data.dptr = (char *)&rgb->r;
	    data.dsize = 3*sizeof(ushort);
	    return(data);
	}
    }
#ifdef DBM_DEBUG
    if (data.dptr == NULL) {
	ErrorF("fetch (%d,%s) failed\n",key.dsize,key.dptr);
    }
#endif
    return(data);
}

/*==========================================================================*/

