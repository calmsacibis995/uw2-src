/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Time/demos/example-1.c	3.1" */
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

#include <Time.h>
#include <stream.h>
#include <String.h>

int error;

int string_objection_handler(const char*){
        cerr << "\nbad string; please try again\n";
        error=1;
        return 1; // recover
}

main(){
	Duration::string_objection.appoint(string_objection_handler);

	Duration d;
	String s;
	
	do{
	    error = 0;
	    cout << "enter the time-of-day: ";
	    cin >> s;
	    d = make_duration(s);
	}while(!error);

	// do something with d...
	return 0;
}
