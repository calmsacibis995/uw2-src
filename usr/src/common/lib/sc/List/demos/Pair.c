/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:List/demos/Pair.c	3.1" */
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

#include "Pair.h"
#include <List.h>

// second version from tutorial

List<Pair>&
sort ( List<Pair>& aList )
{
    Listiter<Pair>  unsorted(aList);
    if ( unsorted.step_next() ) {
        Pair p;
        while( unsorted.remove_next(p) ) {
            Listiter<Pair> sorted = unsorted;
            Pair *q;  // pointer into sorted part
            while( sorted.prev(q) && q->name > p.name )
                ;
            if ( q->name < p.name )
                sorted.step_next();   // back up
            else if ( q->name == p.name ) {
                q->count++;
                continue;
            }
            sorted.insert_next(p);
        }
    }
    return aList;
}


main() {
    String	s;
    List<Pair>	myList;

    while (cin >> s)
        myList.put(Pair(s));

    cout << sort(myList) << "\n";
    return 0;
}

