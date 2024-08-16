/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:List/demos/Pair.h	3.1" */
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

#ifndef PAIR_H
#define PAIR_H

#include <String.h>
#include <stream.h>

struct Pair {
    int    count;
    String name;

    Pair() {}
    Pair(const String& s) : name(s), count(1) {}
    Pair(const Pair& pp) : name(pp.name), count(pp.count) {}
    ~Pair() {}

    Pair&  operator=(const Pair& pp) { 
        name = pp.name; 
        count = pp.count;
        return *this; 
    }

    int operator==(const Pair& pp) { 
        if(name == pp.name && count == pp.count) 
            return 1; 
        else return 0; 
    }
};

ostream&
operator<<(ostream& oo, const Pair& p) {
    oo << "Name: " << p.name << " count: " << p.count;
    return oo;
}

#endif
