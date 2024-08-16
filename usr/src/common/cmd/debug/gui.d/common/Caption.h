/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_CAPTION_H
#define	_CAPTION_H
#ident	"@(#)debugger:gui.d/common/Caption.h	1.3"

#include "Component.h"
#include "CaptionP.h"
#include "gui_label.h"

// The string passed to the Caption constructor is used to label the caption's child
enum Caption_position
{
	CAP_LEFT,
	CAP_TOP_LEFT,
	CAP_TOP_CENTER
};

class Caption : public Component
{
	CAPTION_TOOLKIT_SPECIFICS

private:
	Component       *child;

public:
			Caption(Component *parent, LabelId caption,
				Caption_position, Help_id help_msg = HELP_none);
			~Caption();

			// accepts one child only
	void		add_component(Component *, Boolean resizable = TRUE);
	void		set_label(const char *label);	// changes the label
};

#endif	// _CAPTION_H
