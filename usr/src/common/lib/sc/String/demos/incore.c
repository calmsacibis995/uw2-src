/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:String/demos/incore.c	3.1" */
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
#include <stream.h>

main()
{
    String in_String;
    String out_file; 
    out_file.reserve(100);
    char c;

    String old_one[4];
    String new_one[4];

    old_one[0]="Rhodesia";    new_one[0]="Zimbabwe";
    old_one[1]="Alcindor";    new_one[1]="Abdul-Jabbar";
    old_one[2]="Congo";       new_one[2]="Zaire";
    old_one[3]="Bambergers";  new_one[3]="Macys";
 
    while(cin >> in_String) {
        for(int i=0;i<3;i++)
            if(in_String == old_one[i]) 
                in_String = new_one[i];
        out_file += in_String;
        while((c=cin.peek())==' '||c=='\t'||c=='\n') {
            cin.get(c);
            out_file += c;
        }
    }
    cout << out_file;
    return 0;
}
