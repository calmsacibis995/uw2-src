/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident "@(#)sc:Path/demos/Wild.c	3.5" */
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
 
#include <Path.h>
#include <List.h>
#include <String.h>
#include <iostream.h>
#include <fstream.h>
#include <stdlib.h>
#include <unistd.h>
 
main() {
    // it's hard to make a portable test out of tilde expansion,
    // but this is better than the previous version

    String home (getenv("HOME"));
    String file1 (home + "/1.PathWildTest");
    String file2 (home + "/2.PathWildTest");

    ofstream outFile;
    outFile.open(file1, ios::out);
    if (!outFile) {
        cerr << "cannot open " << file1 << endl;
        exit(1);
    }
    outFile.close();
    outFile.open(file2, ios::out);
    if (!outFile) {
        cerr << "cannot open " << file2 << endl;
        unlink(file1);
        exit(1);
    }
    outFile.close();
 
    Path p ("~/*.PathWildTest");
    Path p_orig = p;

    p.expand_tilde();             // "/my/home/directory/ *.PathWildTest"
 
    List<Path> testfiles;
    p.expand_wildcards(testfiles);
    // "( /my/home/directory/1.PathWildTest /my/home/directory/2.PathWildTest )"

    if (testfiles[0].dirname() != Path(home)) {
        cerr << "tilde expansion incorrect for " << file1 << " says " << testfiles[0].dirname() << endl;
        unlink(file1);  unlink(file2);
        exit(1);
    }
    if (testfiles[1].dirname() != Path(home)) {
        cerr << "tilde expansion incorrect for " << file2 << endl;
        unlink(file1);  unlink(file2);
        exit(1);
    }
    cout << p_orig << " ==> in $HOME: " << testfiles[0].basename()
                                 << " " << testfiles[1].basename() << endl;
    // should produce:
    // ~/ *.PathWildTest ==> in $HOME: 1.PathWildTest 2.PathWildTest
 
    unlink(file1);
    unlink(file2);
 
    return 0;
}
