/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Set/demos/3.c	3.1" */
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

#include <String.h>
#include <Set.h>
#include <stream.h>

main()
{
        Bag<String> count;
        String word;

        while(cin >> word){
            count.insert(word);
        }
        Bagiter<String> bi(count);
        const Bag_pair<String>* p;
        cout << "COUNT\tWORD\n\n";

        while(p = bi.next()){
            cout << p->count << '\t' << p->value << '\n';
        }
	return 0;
}

Set_or_Bag_hashval Bag<String>::hash(const String& s)
{
        return (Set_or_Bag_hashval)s.hashval();
}
