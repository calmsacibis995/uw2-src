/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:fs/demos/prog7.c	3.1" */
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

#include "prog.h"
#include "fs.h"

// X is a silly class that just remembers a single string.
//
class X {
	char *rep;
public:
        void set(const char *s) { 
		size_t l = strlen(s); 
		rep = new char[l]; 
		memcpy(rep, s, l+1); 
	}
        const char *get() const { 
		return rep; 
	}
};

void f() {
        X *x = new X;
        x->set("hello, world!");
}

main() {
	for (int i = 0; i < 4; i++) {
		fs_mark();
                f();
		fs_since();
		cerr << "---------------------" << endl;
        }
	return 0;
}        
