/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:motifgif/xgif.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: xgif.h,v $ $Revision: 1.3 $ $Date: 92/03/13 15:39:53 $ */
/*
 *  xgif.h  -  header file for xgif, but you probably already knew as much
 */


#define REVDATE   "Rev: 2/13/89"
#define MAXEXPAND 16

/* include files */
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <X11/Xfuncs.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


#ifndef MAIN
#define WHERE extern
#else
#define WHERE
#endif

typedef unsigned char byte;

#define CENTERX(f,x,str) ((x)-XTextWidth(f,str,strlen(str))/2)
#define CENTERY(f,y) ((y)-((f->ascent+f->descent)/2)+f->ascent)


/* X stuff */
WHERE Display       *theDisp;
WHERE int           theScreen, dispcells;
WHERE Colormap      theCmap;
WHERE Window        rootW, mainW;
WHERE GC            theGC;
WHERE unsigned long fcol,bcol;
WHERE Font          mfont;
WHERE XFontStruct   *mfinfo;
WHERE Visual        *theVisual;
WHERE XImage        *theImage, *expImage;

/* global vars */
WHERE int            iWIDE,iHIGH,eWIDE,eHIGH,expand,numcols,strip,nostrip;
WHERE unsigned long  cols[256];
WHERE XColor         defs[256];
WHERE char          *cmd;
