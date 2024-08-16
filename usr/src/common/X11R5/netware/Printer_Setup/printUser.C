#ident	"@(#)prtsetup2:printUser.C	1.5"
/*----------------------------------------------------------------------------
 *	printUser.c
 */
#include <iostream.h>
#include <pwd.h>
#include <Xm/Xm.h>

#include "printUser.h"

/*----------------------------------------------------------------------------
 *
 */
PrintUser::PrintUser (char* name)
{
	if (d_name = new char[strlen (name) + 1]) {
		strcpy (d_name, name);
	}

	d_resetState = S_NO_STATE;
	d_denyState = S_NO_STATE;
	d_allowState = S_NO_STATE;
}

/*----------------------------------------------------------------------------
 *
 */
PrintUser::~PrintUser ()
{
	if (d_name) {
		delete (d_name);
	}
}

/*----------------------------------------------------------------------------
 *	initialize AllowState data
 */
void
PrintUser::InitState (AllowState state)
{
	switch (state) {
	case S_ALLOW:
		d_resetState = S_ALLOW;
		d_allowState = S_ALLOW;
		d_denyState = S_NO_STATE;
		break;

	case S_DENY:
		d_resetState = S_DENY;
		d_denyState = S_DENY;
		d_allowState = S_NO_STATE;
		break;

	case S_EMPTY_STATE:
		d_resetState = S_EMPTY_STATE;
		d_denyState = S_EMPTY_STATE;
		d_allowState = S_EMPTY_STATE;
		break;

	case S_NO_STATE:
	default:
		d_resetState = S_NO_STATE;
		d_denyState = S_NO_STATE;
		d_allowState = S_NO_STATE;
		break;
	}
}

/*----------------------------------------------------------------------------
 *	Update the _resetState
 */
void
PrintUser::UpdateResetState (AllowState state)
{
	switch (state) {
	case S_NO_STATE:
		if (d_denyState == S_DENY) {
			d_resetState = d_denyState;
		}
		else {
			if (d_allowState == S_ALLOW) {
				d_resetState = d_allowState;
			}
			else {
				d_resetState = S_NO_STATE;
			}
		}
		break;

	case S_DENY:
		d_resetState = d_denyState;
		d_allowState = S_NO_STATE;
		break;

	case S_ALLOW:
		d_resetState = d_allowState;
		d_denyState = S_NO_STATE;
		break;
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PrintUser::ChangeState (AllowState state)
{
	switch (state) {
	case S_DENY:
		d_denyState = S_DENY;
		d_allowState = S_NO_STATE;
		break;

	case S_ALLOW:
		d_allowState = S_ALLOW;
		d_denyState = S_NO_STATE;
		break;

	default:
		break;
	}
}

/*----------------------------------------------------------------------------
 *	Adds a state.
 */
void
PrintUser::AddState (AllowState state)
{
	switch (state) {
	case S_DENY:
		d_denyState = state;
		break;

	case S_ALLOW:
		d_allowState = state;
		break;

	case S_NO_STATE:
		d_allowState = S_NO_STATE;
		d_denyState = S_NO_STATE;
		break;

	default:
		break;
	}
}

