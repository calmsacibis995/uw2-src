/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/incl/Vblock.c	3.2" */
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
unsigned Vblock<T>::size() const {
	return B.size();
}

template <class T>
unsigned Vblock<T>::size(unsigned k) {
	return B.size(k);
}

template <class T>
void* Vblock<T>::elem(int i) {
	return (void*)&B[i];
}

template <class T>
void* Vblock<T>::beginning() {
	return (T*)(*this);
}

template <class T>
void* Vblock<T>::finish() {
	return end();
}

#pragma can_instantiate Block<char>
#pragma can_instantiate Vblock<char>

