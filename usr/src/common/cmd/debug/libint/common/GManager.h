/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _GManager_h
#define _GManager_h

#ident	"@(#)debugger:libint/common/GManager.h	1.2"

#include "Manager.h"
#include "Message.h"
#include <stddef.h>

class ProcObj;

// Defines the message manager for the graphical interface
// Since gui and debugger may be in different processes, and
// since gui may be dealing with several processes/contexts
// at the same time, needs a lot more state information than
// the cli's message manager

class GUI_Manager : public MessageManager
{
	DBcontext	temp_context;
	DBcontext	dbcontext;		// current ProcObj
	UIcontext	uicontext;		// magic cookie from ui
	int		responding_to_query;
	int		sync_needed;		// gui doing io, wait for it
						// to catch up
	Message		cur_in_msg;
	Message		cur_out_msg;

	Message		sync_m;			// opimization, send same msg
	Message		cmd_complete;		// over and over instead of
						// creating new ones each time

	char		*cmd;			// buffer for input command
	size_t		max_len;		// size of cmd

	int		do_single_cmd();

public:
			GUI_Manager();
			~GUI_Manager()	{ delete cmd; }

	int		send_msg(Msg_id, Severity, ...);
	int		docommand();
	int		query(Msg_id ...);
	void		sync_request();
	void		reset_context(ProcObj *);
};

#endif	// _GManager_h
