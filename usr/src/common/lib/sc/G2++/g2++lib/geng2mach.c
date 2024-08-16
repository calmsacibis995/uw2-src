/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/g2++lib/geng2mach.c	3.6" */
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

//  This program computes the size and alignment constants 
//  for the G2 compiler for a given machine/compiler 
//  combination.
//
//  It generates the file "g2mach.h".

#include <unistd.h>
#include <stdio.h>
#include <String.h>
#include "Vblock.h"

#ifdef LSC
#define main _main
#endif

static 
struct xchar {
    char base[1];
    char x;
}xchar;

static
struct xshort {
    char base[1];
    short x;
}xshort;

static
struct xlong {
    char base[1];
    long x;
}xlong;

static
struct xstring {
    char base[1];
    String x;
}xstring;

static
struct xvblock {
    char base[1];
    Vblock<char> x;
} xvblock;

struct xstruct {
    char base;
};

char* g2progname_ATTLC = "aligncomp";
char* g2filename_ATTLC = "g2mach.h";

main(){
    FILE* f;
    
    if( 
	(f=fopen(g2filename_ATTLC, "w")) == NULL 
    ){
	fprintf(stderr, "cannot open '%s'\n", g2filename_ATTLC);
	return(1);
    }
    fprintf(f, "/*ident	\"@(#)G2++:incl/g2mach.h	3.0\"  */\n");

    fprintf(f, "/******************************************************************************\n");
    fprintf(f, "*\n");
    fprintf(f, "* C++ Standard Components, Release 3.0.\n");
    fprintf(f, "*\n");
    fprintf(f, "* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.\n");
    fprintf(f, "* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.\n");
    fprintf(f, "*\n");
    fprintf(f, "* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System\n");
    fprintf(f, "* Laboratories, Inc.  The copyright notice above does not evidence\n");
    fprintf(f, "* any actual or intended publication of such source code.\n");
    fprintf(f, "*\n");
    fprintf(f, "******************************************************************************/\n\n");
    fprintf(f, "//\n");
    fprintf(f, "// G2 machine-dependent parameters\n");
    fprintf(f, "// (this file is generated by the program geng2mach;\n");
    fprintf(f, "// it should NOT be on a distribution tape)\n");
    fprintf(f, "//\n");

//  Protection against multiple inclusion

    fprintf(
	f,
	"#ifndef G2MACHH\n#define G2MACHH\n\n"
    );

//  Sizes

    fprintf(
	f, 
	"const int CHAR_SIZE_ATTLC=%d;\n", 
	sizeof(char)
    );
    fprintf(
	f, 
	"const int SHORT_SIZE_ATTLC=%d;\n", 
	sizeof(short)
    );
    fprintf(
	f, 
	"const int LONG_SIZE_ATTLC=%d;\n", 
	sizeof(long)
    );
    fprintf(
	f, 
	"const int STRING_SIZE_ATTLC=%d;\n", 
	sizeof(String)
    );
    fprintf(
	f, 
	"const int VBLOCK_SIZE_ATTLC=%d;\n", 
	sizeof(Vblock<char>)
    );

//  Alignments

    fprintf(
	f,
	"\n"
    );
    fprintf(
	f, 
	"const int CHAR_ALIGN_ATTLC=%d;\n", 
	(char*)&xchar.x - xchar.base
    );
    fprintf(
	f, 
	"const int SHORT_ALIGN_ATTLC=%d;\n", 
	(char*)&xshort.x - xshort.base
    );
    fprintf(
	f, 
	"const int LONG_ALIGN_ATTLC=%d;\n", 
	(char*)&xlong.x - xlong.base
    );
    fprintf(
	f, 
	"const int STRING_ALIGN_ATTLC=%d;\n", 
	(char*)&xstring.x - xstring.base
    );
    fprintf(
	f, 
	"const int VBLOCK_ALIGN_ATTLC=%d;\n", 
	(char*)&xvblock.x - xvblock.base
    );
    fprintf(
	f, 
	"const int STRUCT_ALIGN_ATTLC=%d;\n", 
	sizeof(xstruct)
    );

//  Protection against multiple inclusion

    fprintf(
	f,
	"\n#endif\n"
    );

    return(0);
}
