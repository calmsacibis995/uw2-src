/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef SPACES_H
#define SPACES_H
#ident	"@(#)debugger:libexp/common/libexp.h	1.1"

// Spaces provides a string of blanks to provide indentation
// shared by print_rval and print_type to avoid duplication
// of allocated space
class Spaces
{
	char	*spaces;
	int	maxlen;
	int	null_byte;
public:
		Spaces()	{ spaces = 0;  maxlen = null_byte = 0; }

	char	*get_spaces(int n);
};

extern	Spaces	spaces;

#endif	// SPACES_H
