/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EXM_FICONBOXI_H_
#define EXM_FICONBOXI_H_

#ifdef NOIDENT
#pragma ident	"@(#)libMDtI:FIconBoxI.h	1.3"
#endif

#include "FIconBoxP.h"

extern void ExmFIconDrawProc(Widget, ExmFlatItem, ExmFlatDrawInfo *);
extern void ExmFIconDrawIcon(Widget, DmGlyphPtr, Drawable, GC, Boolean,
			     Pixmap, int, int);

#endif /* EXM_FICONBOXI_H_ */
