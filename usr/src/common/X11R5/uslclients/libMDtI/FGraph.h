/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FGraph.h	1.5"
#endif

#ifndef ExmFGRAPH_H
#define ExmFGRAPH_H

/************************************************************************
 * Description:
 *	This is the flattened Graph widget's public header file.
 */

#include "Flat.h"	/* include superclass header */

/************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 */

extern WidgetClass			exmFlatGraphWidgetClass;
typedef struct _ExmFlatGraphClassRec *	ExmFlatGraphWidgetClass;
typedef struct _ExmFlatGraphRec *	ExmFlatGraphWidget;

#define XmNgridColumns		(_XmConst char *)"gridColumns"
#define XmCGridColumns		(_XmConst char *)"GridColumns"
#define XmNgridRows		(_XmConst char *)"gridRows"
#define XmCGridRows		(_XmConst char *)"GridRows"
#define XmNgridWidth		(_XmConst char *)"gridWidth"
#define XmCGridWidth		(_XmConst char *)"GridWidth"
#define XmNgridHeight		(_XmConst char *)"gridHeight"
#define XmCGridHeight		(_XmConst char *)"GridHeight"
#define XmNnormalizePosition	(_XmConst char *)"normalizePosition"
#define XmCNormalizePosition	(_XmConst char *)"NormalizePosition"

extern void	ExmFlatRaiseItem(Widget, Cardinal, Boolean);

#endif /* ExmFGRAPH_H */
