/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DragOverSP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DragOverSP.h,v $ $Revision: 1.34 $ $Date: 93/03/03 16:25:20 $ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDragOverSP_h
#define _XmDragOverSP_h

#include <X11/Shell.h>
#include <X11/ShellP.h>
#include <Xm/XmP.h>
#include <Xm/DragIconP.h>
#include <Xm/DragOverS.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOExpose(do) \
	((XtClass(do))->core_class.expose) ((Widget)(do), NULL, NULL)

/* 
 * DRAGOVER SHELL
 */
typedef struct 
{
    XtPointer				extension;
} XmDragOverShellClassPart;

/* Full class record declaration */

typedef struct _XmDragOverShellClassRec 
{
    CoreClassPart 		core_class;
    CompositeClassPart 		composite_class;
    ShellClassPart 		shell_class;
    WMShellClassPart	        wm_shell_class;
    VendorShellClassPart 	vendor_shell_class;
    XmDragOverShellClassPart 	dragOver_shell_class;
} XmDragOverShellClassRec;

externalref XmDragOverShellClassRec xmDragOverShellClassRec;

typedef struct _XmBackingRec{
    Position	x, y;
    Pixmap	pixmap;
}XmBackingRec, *XmBacking;

typedef struct _XmDragOverBlendRec{
    XmDragIconObject		sourceIcon;	/* source icon */
    Position			sourceX;	/* source location in blend */
    Position			sourceY;	/* source location in blend */
    XmDragIconObject		mixedIcon;	/* blended icon */
    GC				gc;		/* appropriate depth */
}XmDragOverBlendRec, *XmDragOverBlend;

typedef struct _XmDragOverShellPart{
    Position			hotX;		/* current hotX */
    Position			hotY;		/* current hotY */
    unsigned char		cursorState;	/* current cursor state */
    unsigned char		mode;
    unsigned char		activeMode;

    Position			initialX;	/* initial hotX */
    Position			initialY;	/* initial hotY */

    XmDragIconObject		stateIcon;	/* current state icon */
    XmDragIconObject		opIcon;		/* current operation icon */

    XmDragOverBlendRec		cursorBlend;	/* cursor blending */
    XmDragOverBlendRec		rootBlend;	/* pixmap or window blending */
    Pixel			cursorForeground;
    Pixel			cursorBackground;
    Cursor			ncCursor;	/* noncached cursor */
    Cursor			activeCursor;	/* the current cursor */

    XmBackingRec		backing; 	/* backing store for pixdrag */
    Pixmap			tmpPix;		/* temp storage for pixdrag */
    Pixmap			tmpBit;		/* temp storage for pixdrag */
    Boolean                     isVisible;	/* shell is visible */
}XmDragOverShellPart;

typedef  struct _XmDragOverShellRec{
    CorePart	 	core;
    CompositePart 	composite;
    ShellPart 		shell;
    WMShellPart		wm;
    VendorShellPart	vendor;
    XmDragOverShellPart	drag;
} XmDragOverShellRec;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragOverSP_h */
