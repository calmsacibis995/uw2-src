/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Vector_h
#define Vector_h
#ident	"@(#)debugger:inc/common/Vector.h	1.2"

#include <stdlib.h>

class Vector {
	size_t		total_bytes;
	size_t		bytes_used;
	void *		vector;
	void		getmemory(size_t);
	void		check();
public:
			Vector();
			Vector(const Vector &);
			~Vector()		{ if ( vector ) free(vector); }
	Vector &	add(void *, size_t);
	Vector &	drop(size_t i)		{ if ( i <= bytes_used )
							bytes_used -= i;
						  return *this; }
	void *		ptr()			{ return vector;	}
	size_t		size()			{ return bytes_used;	}
	Vector &	operator= (const Vector&);
	Vector &	clear()			{ bytes_used = 0; return *this;	}
#ifdef DEBUG
	Vector &	report(char * = 0);
#endif
};

// pool of global scratch Vectors

#define VPOOL_SIZE	2
class Vector_pool {
	Vector	*pool[VPOOL_SIZE];
	int	top;
public:
		Vector_pool(Vector v[VPOOL_SIZE]);
		~Vector_pool() {}
	Vector	*get();
	void	put(Vector *);
};

extern Vector_pool	vec_pool;

#endif /* Vector_h */
