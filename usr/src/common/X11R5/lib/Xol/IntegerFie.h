/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:IntegerFie.h	1.3"
#endif

#ifndef _INTEGERFIELD_H
#define _INTEGERFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			integerFieldWidgetClass;

typedef struct _IntegerFieldClassRec *	IntegerFieldWidgetClass;
typedef struct _IntegerFieldRec *	IntegerFieldWidget;

typedef struct OlIntegerFieldChanged {
	int			value;
	Boolean			changed;
	OlTextVerifyReason	reason;
}			OlIntegerFieldChanged;

#endif
