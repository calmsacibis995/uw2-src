/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/CacheP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: CacheP.h,v $ $Revision: 1.14 $ $Date: 93/03/03 16:22:56 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmCacheP_h
#define _XmCacheP_h

#include <Xm/GadgetP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a few convenience macros */

#define ClassCacheHead(cp) \
	((cp)->cache_head)

#define ClassCacheCopy(cp) \
	((cp)->cache_copy)

#define ClassCacheCompare(cp) \
	((cp)->cache_compare)

#define CacheDataPtr(p) \
	((XtPointer)&((XmGadgetCacheRef*)p)-> data)

#define DataToGadgetCache(p)	((char *)p - sizeof(XmGadgetCache))


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmCacheDelete() ;
extern void _XmCacheCopy() ;
extern XtPointer _XmCachePart() ;

#else

extern void _XmCacheDelete( 
                        XtPointer data) ;
extern void _XmCacheCopy( 
                        XtPointer src,
                        XtPointer dest,
                        size_t size) ;
extern XtPointer _XmCachePart( 
                        XmCacheClassPartPtr cp,
                        XtPointer cpart,
                        size_t size) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCacheP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
