/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/incl/g2ctype.h	3.1" */
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

#ifndef G2CTYPEH
#define G2CTYPEH

#include <String.h>

extern int g2Ptab_ATTLC[];
extern int g2Dtab_ATTLC[];
extern int g2N1tab_ATTLC[];
extern int g2N2tab_ATTLC[];

inline int 
isdigit_ATTLC(int c){
    return (g2Dtab_ATTLC+1)[c];
}
inline int 
isprint_ATTLC(int c){
    return (g2Ptab_ATTLC+1)[c];
}
inline int 
isname1_ATTLC(int c){  // letters, _
    return (g2N1tab_ATTLC+1)[c];
}
inline int 
isname2_ATTLC(int c){  // letters, numbers, *, _,(,)
    return (g2N2tab_ATTLC+1)[c];
}
int 
isname_ATTLC(
    const String& s
);
int 
isint_ATTLC(
    const String& s
);

#endif
