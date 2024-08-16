/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TEXT_LINE_H
#define	_TEXT_LINE_H
#ident	"@(#)debugger:gui.d/common/Text_line.h	1.5"

#include "Component.h"
#include "Text_lineP.h"

// Display a single line of text.
// editable specifies whether or not the user may edit it

// Framework callbacks:
// there is one callback function, for when the user types return,
//	creator->function((Text_line *)this, (char *)selected_string)

class Text_line : public Component
{
	TEXT_LINE_TOOLKIT_SPECIFICS

private:
	Callback_ptr    return_cb;

public:
			Text_line(Component *parent, const char *name,
				const char *text, int width, Boolean edit,
				Callback_ptr return_cb = 0,
				Command_sender *creator = 0,
				Help_id help_msg = HELP_none);
			~Text_line();

	Callback_ptr	get_return_cb()		{ return return_cb; }
	char		*get_text();		// returns entire contents
	void		set_editable(Boolean);
	void		set_text(const char *);	// change the displayed text
	void		set_cursor(int);	// change cursor position
	void		clear();		// blank out the display
};

#endif	// _TEXT_LINE_H
