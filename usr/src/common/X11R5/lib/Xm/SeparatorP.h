/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/SeparatorP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile: SeparatorP.h,v $ $Revision: 1.20 $ $Date: 93/12/10 12:03:17 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmSeparatorP_h
#define _XmSeparatorP_h

#include <Xm/Separator.h>
#include <Xm/PrimitiveP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Separator class structure  */

typedef struct _XmSeparatorClassPart
{
   XtPointer extension;   /* Pointer to extension record */
} XmSeparatorClassPart;


/*  Full class record declaration for Separator class  */

typedef struct _XmSeparatorClassRec
{
   CoreClassPart         core_class;
   XmPrimitiveClassPart  primitive_class;
   XmSeparatorClassPart  separator_class;
} XmSeparatorClassRec;

externalref XmSeparatorClassRec xmSeparatorClassRec;


/*  The Separator instance record  */

typedef struct _XmSeparatorPart
{
   Dimension	  margin;
   unsigned char  orientation;
   unsigned char  separator_type;
   GC             separator_GC;
} XmSeparatorPart;


/*  Full instance record declaration  */

typedef struct _XmSeparatorRec
{
   CorePart	    core;
   XmPrimitivePart  primitive;
   XmSeparatorPart  separator;
} XmSeparatorRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSeparatorP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
