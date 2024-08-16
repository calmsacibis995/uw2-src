/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	BOXESP_H
#define	BOXESP_H
#ident	"@(#)debugger:libol/common/BoxesP.h	1.1"

// toolkit specific members of the Box class and derived classes
// included by ../../gui.d/common/Boxes.h

#define BOX_TOOLKIT_SPECIFICS		\
protected:				\
	Orientation	orientation;

#define PACKBOX_TOOLKIT_SPECIFICS
#define	DIVIDEDBOX_TOOLKIT_SPECIFICS
#define	EXPANSIONBOX_TOOLKIT_SPECIFICS

#endif	// BOXESP_H
