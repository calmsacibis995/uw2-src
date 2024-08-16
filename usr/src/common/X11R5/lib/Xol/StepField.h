/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)textfield:StepField.h	1.3"
#endif

#ifndef _STEPFIELD_H
#define _STEPFIELD_H

#include "Xol/TextField.h"

extern WidgetClass			stepFieldWidgetClass;

typedef struct _StepFieldClassRec *	StepFieldWidgetClass;
typedef struct _StepFieldRec *		StepFieldWidget;

typedef struct OlTextFieldStepped {
	OlSteppedReason		reason;
	Cardinal		count;
}			OlTextFieldStepped,
		      * OlTextFieldSteppedPointer;

#endif
