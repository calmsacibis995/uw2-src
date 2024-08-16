/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/AtomMgr.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: AtomMgr.h,v $ $Revision: 1.13 $ $Date: 93/03/03 16:22:29 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmAtomMgr_h
#define _XmAtomMgr_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XM_ATOM_CACHE

#ifdef XM_ATOM_CACHE
/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Atom XmInternAtom() ;
extern String XmGetAtomName() ;

#else

extern Atom XmInternAtom( 
                        Display *display,
                        String name,
#if NeedWidePrototypes
                        int only_if_exists) ;
#else
                        Boolean only_if_exists) ;
#endif /* NeedWidePrototypes */
extern String XmGetAtomName( 
                        Display *display,
                        Atom atom) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/
#else /* XM_ATOM_CACHE */
#define XmInternAtom(display, name, only_if_exists) \
		XInternAtom(display, name, only_if_exists)
#define XmGetAtomName(display, atom) \
		XGetAtomName(display, atom)
#endif /* XM_ATOM_CACHE */

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#define XmNameToAtom(display, atom) \
        XmGetAtomName(display, atom)

#endif /* _XmAtomMgr_h */
