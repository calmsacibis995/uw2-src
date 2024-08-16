/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TOGGLEP_H
#define	_TOGGLEP_H
#ident	"@(#)debugger:libol/common/ToggleP.h	1.2"

// toolkit specific members of Toggle_button class
// included by ../../gui.d/common/Toggle.h

// OpenLook version of Toggle is implemented as a checkbox Widget
// One of the friend functions is called when the user selects the checkbox

struct Toggle_descriptor;

#define	TOGGLE_BUTTON_TOOLKIT_SPECIFICS		\
private:					\
	Toggle_descriptor	*toggles;	\
        const Toggle_data       *buttons;	\
        int                     nbuttons;	\
						\
public:						\
	Toggle_descriptor	*get_toggles()	{ return toggles; }	\
	const Toggle_data	*get_buttons()	{ return buttons; }	\
	int			get_nbuttons()	{ return nbuttons; }

#endif	// _TOGGLEP_H
