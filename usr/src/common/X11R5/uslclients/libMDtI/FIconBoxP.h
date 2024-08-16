/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef ExmFICONBOXP_H
#define ExmFICONBOXP_H

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FIconBoxP.h	1.4"
#endif

/************************************************************************
 * Description:
 *	This is the flattened IconBox widget's private header file.
 */

#include "FGraphP.h"
#include "FIconBox.h"

/************************************************************************
 * Define structures and names/types needed to support flat widgets
 */
#define IPART(w)		( ((ExmFlatIconBoxWidget)(w))->iconBox )
#define IITEM(i)		( ((ExmFlatIconBoxItem)(i))->iconBox )

/************************************************************************
 * Define Expanded Sub-object Instance Structure
 */
typedef struct {
    XtPointer	object_data;		/* object data			*/
} ExmFlatIconBoxItemPart;

			/* Item's Full Instance record declaration	*/
typedef struct {
    ExmFlatItemPart		flat;
    ExmFlatGraphItemPart	graph;
    ExmFlatIconBoxItemPart	iconBox;
} ExmFlatIconBoxItemRec, *ExmFlatIconBoxItem;

/************************************************************************
 * Define Widget Instance Structure
 */
			/* Define new fields for the instance part	*/
typedef struct {
    XtCallbackProc	trigger_proc;	/* trigger msg proc for DnD	*/
    XtCallbackProc	menu_proc;	/* menu button callback		*/
    ExmFlatDrawItemProc	draw_proc;	/* drawing  routine		*/
    XtCallbackProc	post_select_proc;/* post select callback	*/
    XtCallbackProc	post_unselect_proc;/* post unselect button callback*/
					/* "short" fields next:		*/
    Dimension		hpad;		/* currently: icon pad on RIGHT	*/
    Dimension		vpad;		/* currently: icon pad on BOTTOM */
					/* "byte" field next:		*/
    Boolean		movable;	/* movable icons		*/
    unsigned char	ds_ops;		/* XmNdropSiteOperations	*/
} ExmFlatIconBoxPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full Exmflat Item Record
			 */
typedef struct _ExmFlatIconBoxRec {
    CorePart		core;
    XmPrimitivePart	primitive;
    ExmFlatPart		flat;
    ExmFlatGraphPart	graph;
    ExmFlatIconBoxPart	iconBox;

    ExmFlatIconBoxItemRec	default_item;
} ExmFlatIconBoxRec;

/************************************************************************
 * Define Widget Class Part and Class Rec
 */
				/* Define new fields for the class part	*/
typedef struct {
    char no_class_fields;	/* Makes compiler happy */
} ExmFlatIconBoxClassPart;

				/* Full class record declaration 	*/

typedef struct _ExmFlatIconBoxClassRec {
    CoreClassPart		core_class;
    XmPrimitiveClassPart	primitive_class;
    ExmFlatClassPart		flat_class;
    ExmFlatGraphClassPart	graph_class;
    ExmFlatIconBoxClassPart	iconBox_class;
} ExmFlatIconBoxClassRec;

				/* External class record declaration	*/

extern ExmFlatIconBoxClassRec		exmFlatIconBoxClassRec;

#endif /* ExmFICONBOXP_H */
