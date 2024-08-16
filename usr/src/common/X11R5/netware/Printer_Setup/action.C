#ident	"@(#)prtsetup2:action.C	1.4"
/*----------------------------------------------------------------------------
 *	action.c
 */

#include "action.h"

extern "C" int					is_user_admin ();

/*----------------------------------------------------------------------------
 *
 */
Action::Action (action* newAction)
	  : BasicComponent (newAction->label)
{
	_specialSens = newAction->specialSensitivity;
	_chgSensitivityFlg = newAction->chgSensitiveFlg;
	_adminReqFlg = newAction->adminReqFlg;
	_dfltSensitivity = newAction->sensitivity;
	_next = 0;
	_group = newAction->group;
}

/*----------------------------------------------------------------------------
 *
 */
Action::~Action ()
{
}

/*----------------------------------------------------------------------------
 *	This function initializes the Action after the inherited class has been
 *	created. Since there is no widget associated with an action this must
 *	occur after the widget has been created and stored in _w by inherited
 *	classes.
 */
void
Action::InitAction (action* act)
{
	if (act->callback) {
		XtAddCallback (_w,
					   XmNactivateCallback,
					   act->callback,
					   act->callback_data);	
	}
	if (act->dfltAction) {
		XtVaSetValues (_w, XmNshowAsDefault, act->dfltAction, NULL);
	}
	ChangeSensitivity (act->sensitivity);
}

/*----------------------------------------------------------------------------
 *	This function changes the sensitivity of a widget based on the
 *	_chgSensitivityFlg and the _adminReqFlg. It works as follows: If the
 *	action requires admin privileges but the user doesn't have them the
 *	sensitivity is set to False always.  Else if the sensitivity of the
 *	action can be  changed it is set to the newSensitivity.
 */
void
Action::ChangeSensitivity (Boolean newSensitivity)
{
	if ((_adminReqFlg) && !is_user_admin ()) {
		XtVaSetValues (_w, XmNsensitive, False, NULL);
	}
	else {
		if (_chgSensitivityFlg) {
			XtVaSetValues (_w, XmNsensitive, newSensitivity, NULL);
		}
		else {
			XtVaSetValues (_w, XmNsensitive, _dfltSensitivity, NULL);
		}
	}
}

/*----------------------------------------------------------------------------
 *	This function changes the sensitivity either to the default sensitivity
 *	or to ~default sensirivity based on the dfltFlg
 */
void
Action::SetSensitivity (Boolean dfltFlg)
{
	if (dfltFlg) {
		ChangeSensitivity (_dfltSensitivity);
	}
	else {
		ChangeSensitivity (!_dfltSensitivity);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
Action::SpecialSensitivity (short special, Boolean newSensitivity)
{
	if (special & _specialSens) {
		XtVaSetValues (_w, XmNsensitive, newSensitivity, NULL);
	}
}

