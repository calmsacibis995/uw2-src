/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/mainE.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: mainE.h,v $ $Revision: 1.2.2.2 $ $Date: 1992/04/28 15:25:46 $ */

#if ( defined main_h )
#define extern 
#endif

#ifdef _NO_PROTO
extern void SetSensitive();
extern void ViewError();
extern void ViewWarning();
extern XmString FetchString();
#else
extern void SetSensitive(Widget cascade, String item, Boolean sensitive);

extern void ViewError(ViewPtr this, XmString s1, XmString s2);

extern void ViewWarning(ViewPtr this, XmString s1, XmString s2);

extern XmString FetchString(ViewPtr this, String name);

#endif

#if ( defined main_h )
#define extern 
#endif
