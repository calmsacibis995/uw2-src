/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _COMMAND_H
#define _COMMAND_H
#ident	"@(#)debugger:gui.d/common/Command.h	1.13"

#include "Component.h"
#include "Panes.h"
#include "Vector.h"
#include "Reason.h"

class Message;
class ProcObj;
class Script_dialog;
class Input_dialog;
class Pane_descriptor;

// The Command pane has two components:
//	the transcript area, which displays the event history and I/O from	
//		all programs in the window set, and
//	the command line, which gives the user access to the debugger's
//		command line interface
// There is one command window per window set.

// PAGES_TO_SCROLL * nlines is max no of lines in transcript pane
#define	PAGES_TO_SCROLL	5 

class Command_pane : public Pane
{
	Text_area	*transcript;
	Text_line	*command;
	Vector		command_line;

	Script_dialog	*script_box;
	Input_dialog	*input_box;

	Boolean		has_selection;		// true if text is selected in the
						// transcript pane
	Boolean		in_continuation;	// true if escaped newline typed
						// in command line

public:
			Command_pane(Window_set *, Base_window *, Box *,
				const Pane_descriptor *);
			~Command_pane();

			// access functions
	Text_area	*get_transcript()	{ return transcript; }

			// Component callbacks
	void		script_dialog_cb(Component *, void *);
	void		input_dialog_cb(Component *, void *);
	void		select_cb(Text_area *, int);
	void		copy_selection();
	void		do_command(Component *, const char *);
	void		interrupt_cb(Component *, void *);

			// framework callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *server, Reason_code, void *, ProcObj *);

			// inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

			// functions overridding the ones in Pane base class
	Selection_type	selection_type();
	char		*get_selection();
	void		popup();
	void		popdown();
	int		check_sensitivity(int);
};

#endif	// _COMMAND_H
