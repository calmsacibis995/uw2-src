/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Path/find.c	3.1" */
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

#include "Pathlib.h"
#include <Search_path.h>

int Search_path::find(const Path & p, Path & ret, Ksh_test::unary m, Ksh_test::id t) const {
	if (p.is_absolute()) {
		if (ksh_test(m, p, t)) {
			ret = p;
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		Listiter<Path> i(*(Search_path*)this);
		while (!i.at_end()) {
			Path tmp = *(i.next());
			tmp /= p;
//cout << "checking access of " << tmp << endl;
			if (ksh_test(m, tmp, t)) {
				ret = tmp;
				return 1;
			}
		}
		return 0;
	}
}

int Search_path::find_all(const Path & p, List<Path> & l, Ksh_test::unary m, Ksh_test::id t) const {
	l.make_empty();
	if (p.is_absolute()) {
		if (ksh_test(m, p, t))
			l.put(p);
	}
	else {
		Listiter<Path> i(*(Search_path*)this);
		while (!i.at_end()) {
			Path tmp = *(i.next());
			tmp /= p;
			if (ksh_test(m, tmp, t))
				l.put(tmp);
		}
	}
	// not really necessary, since put doesn't change current position
	// l.reset();
	return (l.length() > 0);
}

