/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_AREA_H
#define	_TEXT_AREA_H
#ident	"@(#)debugger:gui.d/common/Text_area.h	1.4"

#include "Component.h"
#include "Text_areaP.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define TEXT_AREA	Text_area
#else
#define TEXT_AREA
#endif

// A Text_area is a scrollable window of multi-line text
// editable specifies whether or not the user may edit the text

// Framework callbacks:
// there is one callback function for getting or losing the selection
// the callback is invoked as
//	selection:	creator->function((Text_area *)this, 1)
//	deselection:	creator->function((Text_area *)this, 0)

class Text_area : public Component
{
	TEXT_AREA_TOOLKIT_SPECIFICS

protected:
	Callback_ptr	select_cb;
	short		nrows;
	short		npages;

public:
			Text_area(Component *, const char *name,
				int rows, int columns, Boolean editable = FALSE,
				Callback_ptr select_cb = 0,
				Command_sender *creator = 0, Help_id help_msg = HELP_none, int scroll = 0);
			~Text_area();

			// editing functions
	void		copy_selection();	// copy selection to the clipboard
	char		*get_selection();	// return selection string
	char		*get_text();		// returns entire contents
	void		add_text(const char *);	// append to the end
	void		position(int line);	// scroll the given line into view
	void		clear();		// blanks out display

			// access functions
	int		get_current_position();
};

#endif	// _TEXT_AREA_H
