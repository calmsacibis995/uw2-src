/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef REASON_H
#define REASON_H
#ident	"@(#)debugger:gui.d/common/Reason.h	1.1"

// The reason code is set by the Window_set or Event_list before
// notifying clients to indicate what changed
enum Reason_code
{
	RC_change_state,	// hit a breakpoint, started running, etc.
	RC_set_current,		// context change within a window set
	RC_set_frame,		// context change within the process
	RC_rename,		// program has been renamed
	RC_delete,		// process died or was released
	RC_animate,		// starting animation
	RC_start_script,
	RC_end_script,
};

#endif // REASON_H
