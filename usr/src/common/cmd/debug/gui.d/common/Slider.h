/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SLIDER_H
#define	_SLIDER_H
#ident	"@(#)debugger:gui.d/common/Slider.h	1.1"

#include "Component.h"
#include "SliderP.h"

// A slider lets the user select a value from within a specified range

// Framework callbacks:

class Slider : public Component
{
	SLIDER_TOOLKIT_SPECIFICS

public:
		Slider(Component *parent, const char *name, Orientation,
			int min, int max, int initial, int granularity,
			Help_id help_msg = HELP_none);
		~Slider() {};

	int	get_value();
	void	set_value(int);
};

#endif	// _SLIDER_H
