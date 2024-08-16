/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_STEXT_H
#define	_STEXT_H
#ident	"@(#)debugger:gui.d/common/Stext.h	1.2"

#include "Component.h"
#include "StextP.h"

class Simple_text : public Component
{
	SIMPLE_TEXT_TOOLKIT_SPECIFICS

public:
			Simple_text(Component *parent, const char *text,
				Boolean resize, Help_id help_msg = HELP_none);
			~Simple_text();

	void		set_text(const char *text);	// changes the display
};

#endif	// _STEXT_H
