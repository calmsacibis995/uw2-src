/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:List/incl/Listio.c	3.3" */
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

#include <List.h>
#include <stream.h>

template <class T>
ostream& operator<<(ostream& oo, const List<T>& ll)
{
	int first = 1;
	oo << "( ";
	Const_listiter<T> l(ll);
	while (!l.at_end()) {
		if (!first)
			oo << ", ";
		first = 0;
		oo << *(l.next());
	}
	oo << " )";
	return oo;
}

template <class T>
ostream& operator<<(ostream& oo, const List_of_p<T>* ll)
{
	return operator<<(oo, *ll);
}

template <class T>
ostream& operator<<(ostream& oo, const List_of_p<T>& ll)
{
	int first = 1;
	oo << "( ";
	Const_list_of_piter<T> l(ll);
	while (!l.at_end()) {
		if (!first)
			oo << ", ";
		first = 0;
		oo << *(l.next());
	}
	oo << " )";
	return oo;
}
