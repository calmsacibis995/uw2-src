/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:List/demos/Student.c	3.2" */
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

#include "Student.h"
#include <List.h>
#include <iostream.h>
 
main()
{
    Student s1("George","A");
    Student s2("Arthur","D");
    Student s3("Chester","C");
    Student s4("Willy","B");
 
    List<Student> Class(s1,s2,s3,s4);
    // Class.print(cout) << "\n";
    cout << Class << "\n";
    Class.sort(name_compare);
    // Class.print(cout) << "\n";
    cout << Class << "\n";
    Class.sort(grade_compare);
    // Class.print(cout) << "\n";
    cout << Class << "\n";
    return 0;
}
