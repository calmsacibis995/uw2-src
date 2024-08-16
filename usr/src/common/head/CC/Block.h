/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Block/incl/Block.h	3.3" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#ifndef BLOCK_H
#define BLOCK_H

#include <new.h>
#include <Blockio.h>

template <class T> class Block {

public:
	Block() : n(0), p(0) {}
	Block(unsigned k) : n(0), p(0) { size(k); }
	Block(const Block<T>& b) { copy(b); }
	~Block();

	Block<T>& operator=(const Block<T>&);

	unsigned size() const { return n; }
	unsigned size(unsigned);

	operator T*() { return p; }
	operator const T*() const { return p; }

	T* end() { return p + n; }
	const T* end() const { return p + n; }

	T& operator[](int i) { return p[i]; }
	const T& operator[](int i) const { return p[i]; }

	int reserve(unsigned k) { return k<n || grow(k); }
	void swap(Block<T>& b);

private:
	T* p;
	unsigned n;

	void move(T*, unsigned);
	void transfer(T*, unsigned);
	void clear(T*, unsigned);
	void copy(const Block<T>&);
	unsigned grow(unsigned);
	static T* default_value();
};

#endif
