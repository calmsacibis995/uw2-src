/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Set/incl/bagio.c	3.2" */
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

#include <bag.h>
#include <stream.h>

template <class T>
ostream& operator<<(ostream& os,const Bag<T>& b)
{
    Bag_bucketiter_ATTLC<T> bi(b);
    const Bag_bucket_ATTLC<T>* bp = bi.first();
    os << "{";

    while ( bp ) {
	Listiter< Bag_pair<T> > li(((Bag_bucket_ATTLC<T>*)bp)->collision_list);

	while ( !li.at_end() ) {
	    Bag_pair<T>* tp;
	    li.next(tp);
	    os 	<< "("
		<< tp->count
		<< ","
		<< tp->value
		<< ")"
	    ;
	}
	bp = bi.next();
    }
    os << "}";
    return os;
}
