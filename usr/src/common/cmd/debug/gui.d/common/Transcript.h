/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Transcript_h
#define _Transcript_h

#ident	"@(#)debugger:gui.d/common/Transcript.h	1.2"

#include <stddef.h>


// Class that maintains a buffer of text lines.
// The number of lines is limited.  When the
// buffer grows beyond the limit, the oldest
// lines are discarded.

class Transcript {
	char	*string;
	size_t	*lines;
	size_t	last;
	size_t	bytes_used;
	size_t	total_bytes;
	size_t	nlines;
	void	getmemory(size_t sz);
	size_t 	discard_old();
public:
		Transcript();
		~Transcript();
	void	add(char *);
	char 	*get_string() { return (bytes_used ? string : ""); }
	void	clear();
	size_t	size()  { return bytes_used; }
};

#endif
