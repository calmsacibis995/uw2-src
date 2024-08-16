/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:printUser.h	1.4"
/*----------------------------------------------------------------------------
 *	printUser.h
 */
#ifndef PRINTUSER_H
#define PRINTUSER_H

typedef enum {
								S_ALLOW,
								S_DENY,
								S_NO_STATE,
								S_MIXED,
								S_EMPTY_STATE
} AllowState;

/*----------------------------------------------------------------------------
 *
 */
class PrintUser {
public:								// Constructors/Destructors
								PrintUser (char* name);
								~PrintUser ();

private:							// Private data
	char*						d_name;
	AllowState 					d_resetState;
	AllowState 					d_denyState;
	AllowState					d_allowState;	

public:								// Public interface methods
	void						InitAllowed (Boolean state);
	void						InitState (AllowState state);
	void						UpdateResetState (AllowState state);
	void						ChangeState (AllowState state);
	void						AddState (AllowState state);

public:								// Public inline interface methods
	inline char*				Name ();
	inline AllowState			DenyState ();
	inline AllowState			Allow_State ();
	inline AllowState			ResetState ();
	inline void					ResetState (AllowState state);
};

/*----------------------------------------------------------------------------
 *
 */
char*
PrintUser::Name ()
{
	return (d_name);
}

AllowState
PrintUser::DenyState ()
{
	return (d_denyState);
}

AllowState
PrintUser::Allow_State ()
{
	return (d_allowState);
}

AllowState
PrintUser::ResetState ()
{
	return (d_resetState);
}

void
PrintUser::ResetState (AllowState state)
{
	d_resetState = state;	
}

#endif
