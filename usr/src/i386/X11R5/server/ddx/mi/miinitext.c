/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)r5server:ddx/mi/miinitext.c	1.7"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: miinitext.c,v 1.15.1.1 92/09/09 15:34:45 rws Exp $ */

#ifdef BEZIER
extern void BezierExtensionInit();
#endif
#ifdef XTESTEXT1
extern void XTestExtension1Init();
#endif
#ifdef SHAPE
extern void ShapeExtensionInit();
#endif
#ifdef MITSHM
extern void ShmExtensionInit();
#endif
#ifdef PEXEXT
extern void PexExtensionInit();
#endif
#ifdef MULTIBUFFER
extern void MultibufferExtensionInit();
#endif
#ifdef XINPUT
extern void XInputExtensionInit();
#endif
#ifdef XTEST
extern void XTestExtensionInit();
#endif
#ifdef MITMISC
extern void MITMiscExtensionInit();
#endif
#ifdef XIDLE
extern void XIdleExtensionInit();
#endif
#ifdef XTRAP
extern void DEC_XTRAPInit();
#endif
#ifdef XYZEXT
extern void XamineYourZerverInit();
#endif
#ifdef BUILTIN
extern void BuiltinExtensionInit();
#endif

extern void siExtensionInit();

/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef BEZIER
    BezierExtensionInit();
#endif
#ifdef XTESTEXT1
    XTestExtension1Init();
#endif
#ifdef SHAPE
    ShapeExtensionInit();
#endif
#ifdef MITSHM
    ShmExtensionInit();
#endif
#ifdef PEXEXT
    PexExtensionInit();
#endif
#ifdef MULTIBUFFER
    MultibufferExtensionInit();
#endif
#ifdef XINPUT
    XInputExtensionInit();
#endif
#ifdef XTEST
    XTestExtensionInit();
#endif
#ifdef MITMISC
    MITMiscExtensionInit();
#endif
#ifdef XIDLE
    XIdleExtensionInit();
#endif
#ifdef XTRAP
    DEC_XTRAPInit();
#endif
#ifdef XYZEXT
    XamineYourZerverInit(argc, argv);
#endif
#ifdef BUILTIN
    BuiltinExtensionInit();
#endif

    siExtensionInit();
}
