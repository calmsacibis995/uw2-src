/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_BUTTON_BARP_H
#define	_BUTTON_BARP_H
#ident	"@(#)debugger:libol/common/Button_barP.h	1.2"

// toolkit specific members of the Button_bar class
// included by ../../gui.d/common/Button_bar.h

struct Bar_data;	// defined in Button_bar.C

#define	BUTTON_BAR_TOOLKIT_SPECIFICS	\
private:				\
	Bar_data	*list;	/* button table for button bar */ \
	Widget		buttons;	/* flat button widget */

#endif	// _BUTTON_BARP_H
