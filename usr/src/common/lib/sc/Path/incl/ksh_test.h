/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Path/incl/ksh_test.h	3.1" */
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

#ifndef _KSH_TEST_H
#define _KSH_TEST_H 1

class Ksh_test {
public:
	enum unary{  
		// 'a' is undocumented in the ksh book.  
		// it means "file can be statted".
		a = 'a',
		r = 'r', w = 'w', x = 'x', f = 'f', 
		d = 'd', c = 'c', b = 'b', p = 'p', 
		u = 'u', g = 'g', k = 'k', s = 's', 
		L = 'L', O = 'O', G = 'G', S = 'S' 
	};
	enum binary{ ef = 0, nt = 1, ot = 2 };
	enum id{ effective = 0, real = 1 };
};

int ksh_test(const char *file, Ksh_test::id = Ksh_test::effective);
int ksh_test(Ksh_test::unary, const char *file, Ksh_test::id = Ksh_test::effective);
int ksh_test(const char *file1, Ksh_test::binary, const char *file2, Ksh_test::id = Ksh_test::effective);

#endif /* _KSH_TEST_H */
