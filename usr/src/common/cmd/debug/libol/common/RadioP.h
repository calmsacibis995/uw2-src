/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_RADIOP_H
#define	_RADIOP_H
#ident	"@(#)debugger:libol/common/RadioP.h	1.3"

// toolkit specific members of Radio class,
// included by ../../gui.d/common/Radio.h

// Radio list is implemented as a FlatButton widget

class Button_data;

// toolkit specific members:
//	buttons		list of entries for creating flat buttons
//	nbuttons	total number of buttons
//	current		button currently pushed

#define RADIO_TOOLKIT_SPECIFICS		\
private:				\
	Button_data	*buttons; 	\
        int             nbuttons;       \
        int             current;        \
					\
public:					\
	int		get_nbuttons()	{ return nbuttons; }	\
	void		set_current(int i)	{ current = i; }

#endif	// _RADIOP_H
