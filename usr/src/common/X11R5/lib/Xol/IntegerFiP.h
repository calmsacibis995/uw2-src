/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:IntegerFiP.h	1.2"
#endif

#ifndef _INTEGERFIELDP_H
#define _INTEGERFIELDP_H

#include "Xol/TextFieldP.h"
#include "Xol/IntegerFie.h"

/*
 * Class structure:
 */

typedef struct _IntegerFieldClassPart {
	XtPointer		extension;
}			IntegerFieldClassPart;

typedef struct _IntegerFieldClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	TextEditClassPart	textedit_class;
	TextFieldClassPart	textfield_class;
	IntegerFieldClassPart	integerfield_class;
}			IntegerFieldClassRec;

extern IntegerFieldClassRec	integerFieldClassRec;

#define INTEGER_C(WC) ((IntegerFieldWidgetClass)(WC))->integerfield_class

/*
 * Instance structure:
 */

typedef struct _IntegerFieldPart {
	/*
	 * Public:
	 */
	int			value;
	int			value_min;
	int			value_max;
	int			value_granularity;
	XtCallbackList		value_changed;

	/*
	 * Private:
	 */
	int			prev_value;
}			IntegerFieldPart;

typedef struct _IntegerFieldRec {
	CorePart		core;
	PrimitivePart		primitive;
	TextEditPart		textedit;
	TextFieldPart		textfield;
	IntegerFieldPart	integerfield;
}			IntegerFieldRec;

#define INTEGER_P(W) ((IntegerFieldWidget)(W))->integerfield

#endif
