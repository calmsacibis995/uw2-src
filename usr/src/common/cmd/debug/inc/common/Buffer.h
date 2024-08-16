/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Buffer_h
#define Buffer_h

#ident	"@(#)debugger:inc/common/Buffer.h	1.6"

#include <stdlib.h>

// null terminated string that grows as necessary (never shrinks)

class Buffer
{
	size_t	bytes_used;	// includes null byte
	size_t	total_bytes;
	char	*string;
	void	getmemory(size_t);

public:
		 Buffer()	{ bytes_used = total_bytes = 0;
				  string = 0;
				}
		~Buffer()	{ if (string) free(string); }

	void	add(const char *);
	void	add(char);
	void	clear()		{ bytes_used = 0; }
	size_t	size()		{ return bytes_used; } // includes null byte
	operator char *()	{ return bytes_used ? string : ""; }
};

// pool of global scratch buffers

#define BPOOL_SIZE	5
class Buffer_pool {
	Buffer	*pool[BPOOL_SIZE];
	int	top;
public:
		Buffer_pool(Buffer b[BPOOL_SIZE]);
		~Buffer_pool() {}
	Buffer	*get();
	void	put(Buffer *);
};

extern Buffer_pool	buf_pool;

#endif // Buffer_h
