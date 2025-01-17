/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ShellEP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ShellEP.h,v $ $Revision: 1.10 $ $Date: 93/03/03 16:32:56 $ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmShellEP_h
#define _XmShellEP_h

#include <Xm/DesktopP.h>

#ifdef __cplusplus
extern "C" {
#endif


#define XmInheritEventHandler		((XtEventHandler)_XtInherit)

#define _XmRAW_MAP 0
#define _XmPOPUP_MAP 1
#define _XmMANAGE_MAP 2


#ifndef XmIsShellExt
#define XmIsShellExt(w)	XtIsSubclass(w, xmShellExtObjectClass)
#endif /* XmIsShellExt */

typedef struct _XmShellExtRec *XmShellExtObject;
typedef struct _XmShellExtClassRec *XmShellExtObjectClass;
externalref WidgetClass xmShellExtObjectClass;


typedef struct _XmShellExtClassPart{
    XtEventHandler	structureNotifyHandler;
    XtPointer		extension;
}XmShellExtClassPart, *XmShellExtClassPartPtr;

typedef struct _XmShellExtClassRec{
    ObjectClassPart		object_class;
    XmExtClassPart		ext_class;
    XmDesktopClassPart 		desktop_class;
    XmShellExtClassPart 	shell_class;
}XmShellExtClassRec;

typedef struct {
    unsigned long	lastConfigureRequest;
    Boolean		useAsyncGeometry;
} XmShellExtPart, *XmShellExtPartPtr;

externalref XmShellExtClassRec 	xmShellExtClassRec;

typedef struct _XmShellExtRec{
    ObjectPart			object;
    XmExtPart			ext;
    XmDesktopPart		desktop;
    XmShellExtPart		shell;
}XmShellExtRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmShellEP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
