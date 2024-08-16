/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:dogs/Square.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: Square.h,v $ $Revision: 1.4 $ $Date: 92/03/13 15:34:28 $ */

/*****************************************************************************
*
*  Square.h - widget public header file
*
******************************************************************************/

#ifndef _Square_h
#define _Square_h

#include <Xm/BulletinB.h>

externalref WidgetClass squareWidgetClass;

typedef struct _SquareClassRec *SquareWidgetClass;
typedef struct _SquareRec *SquareWidget;

extern Widget SquareCreate();
extern int SquareMrmInitialize();

#define IsSquare(w) XtIsSubclass((w), squareWidgetClass)

#define SquareWIDTH 0
#define SquareHEIGHT 1

#define SquareNmajorDimension "majorDimension"
#define SquareCMajorDimension "MajorDimension"

#define SquareNmakeSquare "makeSquare"
#define SquareCMakeSquare "MakeSquare"

#endif /* _Square_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
