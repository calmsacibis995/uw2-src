/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Block/incl/Block.c	3.3" */
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

template <class T>
unsigned Block<T>::size(unsigned k)
{
	if ( k != n ) 			// Move, even when shrinking, since
		move(new T[k], k);	// the unused space can't be reused

	return n;
}

template <class T>
Block<T>::~Block()
{
	if (p) delete[] p;
}

template <class T>
Block<T>& Block<T>::operator=(const Block<T>& b)
{
	if ( this != &b ) {
		if (p) delete[] p;
		copy(b);
	}

	return *this;
}


// Clear k elements starting at v

template <class T>
void Block<T>::clear(T* v, unsigned k)
{
	register T* p = v;
	register T* lim = v + k;
	T* valptr = default_value();

	while ( p < lim )
		*p++ = *valptr;
}

// Make this a copy of b

template <class T>
void Block<T>::copy(const Block<T>& b)
{
	// assert (p is 0 or garbage)
	p = new T[b.n];
	if ( p ) {
		n = b.n;
		transfer(b.p, n);
	} else
		n = 0;
}

// Grow this Block by 1.5 until it can contain at least k+1

template <class T>
unsigned Block<T>::grow(unsigned k)
{
	unsigned nn = n;

	if ( nn == 0 )
		nn++;

	while ( nn <= k )
		nn += (nn >> 1) + 1;

	T* np = new T[nn];
	if ( !np ) {
		nn = k+1;
		np = new T[nn];
	}

	move(np, nn);	// takes care of case when np == 0
	return n;
}

// Transfer len (or fewer) elements into this Block.

template <class T>
void Block<T>::transfer(T* source, unsigned len)
{
	register T* plim;
	register T* pp = p;
	register T* q = source;

	if ( n > len ) {
		plim = p + len;
		clear(plim, n - len);
	} else
		plim = p + n;

	while ( pp < plim )
		*pp++ = *q++;
}

// The contents of this Block now live in memory starting at np
// If np is 0, null out this Block.

template <class T>
void Block<T>::move(T* np, unsigned nn)
{
	T* oldp = p;
	unsigned oldn = n;

	p = np;
	if ( np ) {
		n = nn;
		transfer(oldp, oldn);
	} else
		n = 0;

	if (oldp) delete[] oldp;
}

// Exchange the contents of this Block with another Block

template <class T>
void Block<T>::swap(Block<T>& b)
{
	T* bp = b.p;
	unsigned bn = b.n;
	b.p = p;
	b.n = n;
	p = bp;
	n = bn;
}

template <class T>
T* Block<T>::default_value()
{
	static T default_item;
	return(&default_item);
}

#pragma can_instantiate Block<char>
