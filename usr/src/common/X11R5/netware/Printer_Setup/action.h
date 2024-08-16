/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:action.h	1.4"
/*----------------------------------------------------------------------------
 *	action.h
 */
#ifndef ACTION_H
#define ACTION_H

#include <Xm/Xm.h>

#include "BasicComponent.h"

/*----------------------------------------------------------------------------
 *
 */
#define SENSITIVITY_NONE		0x00
#define SENSITIVITY_ONE			0x01
#define SENSITIVITY_TWO			0x02
#define SENSITIVITY_THREE		0x04
#define SENSITIVITY_FOUR		0x08

#define BUTTON_NONE				0
#define BUTTON_OK				1
#define BUTTON_CANCEL			2

/*----------------------------------------------------------------------------
 *
 */
typedef struct _action {
	char*						label;
	char*						pix;
	short						group;
	char*						mnemonic;
	char*						accelerator;
	char*						accel_text;
	XtCallbackProc				callback;
	XtPointer					callback_data;
	Boolean						sensitivity;			// Initial sensitivity
	Boolean						chgSensitiveFlg;
	Boolean						adminReqFlg;
	Boolean						dfltAction;
	short						specialSensitivity;
	Boolean						show;
	char*						prompt;
	Widget						w;
	int							which;
} action; 

/*----------------------------------------------------------------------------
 *
 */
class Action : public BasicComponent {
public:
								Action (action* act);
								~Action ();

private:
	Boolean						_chgSensitivityFlg;
	Boolean						_adminReqFlg;
	Boolean						_dfltSensitivity;
	short						_group;
	short						_specialSens;

public:
	Action*						_next;			// Make this Private!!!!!!!!!!

public:
	void						InitAction (action* actionInfo);
	inline short				Group ();
	void						SetSensitivity (Boolean dfltFlag);
	void						SpecialSensitivity (short, Boolean);

private:
	void						ChangeSensitivity (Boolean newSensitivity);
};

/*----------------------------------------------------------------------------
 *
 */
short
Action::Group ()
{
	return (_group);
}

#endif
