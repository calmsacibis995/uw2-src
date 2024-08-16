/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TEXT_LINEP_H
#define	_TEXT_LINEP_H
#ident	"@(#)debugger:libol/common/Text_lineP.h	1.2"

// toolkit specific members of the Text_line class
// included by ../../gui.d/common/Text_line.h

#define TEXT_LINE_TOOLKIT_SPECIFICS	\
private:				\
	Boolean         editable;	\
	char            *string;

#endif	// _TEXT_LINEP_H
