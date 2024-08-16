/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef ExmFICONBOX_H
#define ExmFICONBOX_H

#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FIconBox.h	1.7"
#endif


/************************************************************************
 * Description:
 *	This is the flattened IconBox widget's public header file.
 */

#include "FGraph.h"

/************************************************************************
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 */

extern WidgetClass				exmFlatIconBoxWidgetClass;
typedef struct _ExmFlatIconBoxClassRec *	ExmFlatIconBoxWidgetClass;
typedef struct _ExmFlatIconBoxRec *		ExmFlatIconBoxWidget;

#define XmNdrawProc		(_XmConst char *)"drawProc"
#define XmNmenuProc		(_XmConst char *)"menuProc"
#define XmNtriggerMsgProc	(_XmConst char *)"triggerMsgProc"
#define XmNiconPadHoriz		(_XmConst char *)"iconPadHoriz"
#define XmCIconPad		(_XmConst char *)"IconPad"
#define XmNiconPadVert		(_XmConst char *)"iconPadVert"
#define XmNmovableIcons		(_XmConst char *)"movableIcons"
#define XmCMovableIcons		(_XmConst char *)"MovableIcons"
#define XmNobjectData		(_XmConst char *)"objectData"
#define XmCObjectData		(_XmConst char *)"ObjectData"
#define XmNpostSelectProc	(_XmConst char *)"postSelectProc"
#define XmNpostUnselectProc	(_XmConst char *)"postAdjustProc"

/* Possible `reason's for the `reason' field in ExmFIconBoxButtonCD... */
enum {
	ExmB_ADJUST = 'a', ExmB_SELECT = 's', ExmB_MENU = 'm',
	ExmK_DESELECT_ALL = '1',
	ExmK_SELECT = '3', ExmK_MENU = '4', ExmK_ADJUST = '5'
};

typedef struct {
    ExmFlatCallData	item_data;
	/* Note that Motif callback convention is that `reason' and `event'
	 * shall always be the very first two fields in a callback
	 * data structure. Flat code doesn't follow this convention
	 * currently. We may want to define these two fields in
	 * ExmFlatCallData in the future, 9/8/94... */
    int			reason;		/* ExmB_* and ExmK_* etc.	*/
    Widget		menu_widget;	/* A XmNmenuProc can set this field
					 * to the XmMenuShellWidget id and
					 * set `ok' to True if an app requires
					 * the ExmFlatIconBox widget to
					 * perform menu posting... */
    int			count;		/* number of select presses 	*/
					/* "short" fields next:		*/
    Position		x;		/* mouse pointer position	*/
    Position		y;		/* mouse pointer position	*/
					/* "byte" fields next:		*/
    Boolean		ok;		/* ok to do the default action  */
} ExmFIconBoxButtonCD;

typedef struct {
    ExmFlatCallData	item_data;
    Atom *		export_targets;
    Cardinal		num_export_targets;
    Atom *		import_targets;
    Cardinal		num_import_targets;
    Widget		drag_context;
					/* "short" fields next:		*/
					/* "byte" fields next:		*/
    unsigned char	operation;
    WidePosition	x;		/* dest x */
    WidePosition	y;		/* dest y */
} ExmFIconBoxTriggerMsgCD;

#endif /* ExmFICONBOX_H */
